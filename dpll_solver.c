#include "dpll_solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SATISFIABLE 1
#define UNSATISFIABLE -1
#define UNCERTAIN 0

int dpll(struct Clause* root);
struct Clause* readClauseSetFromInput(char cnf[][100], int numClauses, int bnf);
void removeClause(struct Clause* root);
void writeSolutionToOutput(struct Clause* root, int* valuation, int bnf);

int clauseNumber, variableNumber;
int* valuation;
int hard_case = 0;

// Create and return an empty Clause
struct Clause* createClause() {
    struct Clause* instance = malloc(sizeof(struct Clause));
    instance->head = NULL;
    instance->next = NULL;
    return instance;
}

void freeClause(struct Clause *clause) {
    if (clause == NULL) return;

    // Free all literals in the clause
    struct Literal *currentLiteral = clause->head;
    while (currentLiteral != NULL) {
        struct Literal *nextLiteral = currentLiteral->next;
        free(currentLiteral); // Free the current literal
        currentLiteral = nextLiteral; // Move to the next literal
    }

    // Free the clause itself
    free(clause);
}

void printClauseSet(struct Clause * root){
    struct Clause* itr = root;
    while (itr != NULL){
        struct Literal * l = itr->head;
        while (l != NULL){
            printf("%d ", l->index);
            l = l->next;
        }
        printf("\n");
        itr = itr->next;
    }
}



// Create and return an empty Literal
struct Literal* createLiteral() {
    struct Literal* instance = malloc(sizeof(struct Literal));
    instance->next = NULL;
    instance->index = 0;
    return instance;
}

// finds a unit clause and return its literal index for unit-propagation step
int findUnitClause(struct Clause * root){
    struct Clause * itr = root;
    while (itr != NULL){
        if (itr->head == NULL) {
            if (verbose) printf("Contradiction: Empty clause found\n");
            continue;
        }
        if(itr->head->next == NULL){
            return itr->head->index;
        }
        itr = itr->next;
    }
    // no unit clause found, return 0
    return 0;
}

// signal function
int sign(int num){
    return (num > 0) - (num < 0);
}

// finds a pure literal by iterating through all clauses
int findPureLiteral(struct Clause * root){
    // create a lookup table to keep track of literal pureness
    int * literalLookup = (int*) calloc(variableNumber + 1, sizeof(int));
    struct Clause * itr = root;
    while (itr != NULL){
        struct Literal * l = itr->head;
        while (l != NULL){
            int seen = literalLookup[abs(l->index)];
            if (seen == 0) literalLookup[abs(l->index)] = sign(l->index);
            else if (seen == -1 && sign(l->index) == 1) literalLookup[abs(l->index)] = 2;
            else if (seen == 1 && sign(l->index) == -1) literalLookup[abs(l->index)] = 2;
            l = l->next;
        }
        itr = itr->next;
    }

    // iterate over the lookup table to send the first pure literal found
    int i;
    for (i = 1; i < variableNumber + 1; i++) {
        if (literalLookup[i] == -1 || literalLookup[i] == 1) return i * literalLookup[i];
    }
    // no pure literal found, return 0
    return 0;
}

int unitPropagation(struct Clause **root){
    int unitLiteralIndex = findUnitClause(*root);
    if (verbose && unitLiteralIndex != 0 && hard_case!=1) printf("Easy case: Unit literal %d\n", abs(unitLiteralIndex));
    hard_case = 0;
    if (unitLiteralIndex == 0) return 0;

    valuation[abs(unitLiteralIndex)] = unitLiteralIndex > 0 ? 1 : 0;

    struct Clause *itr = *root;
    struct Clause *prev = NULL;
    while (itr != NULL){
        struct Literal *currentL = itr->head;
        struct Literal *previousL = NULL;

        int clauseRemoved = 0;
        while (currentL != NULL){
            if (currentL->index == unitLiteralIndex) {
                if (prev == NULL){
                    *root = itr->next;
                    freeClause(itr);
                    itr = *root;
                } else {
                    prev->next = itr->next;
                    freeClause(itr);
                    itr = prev->next;
                }
                clauseRemoved = 1;
                break;
            } else if (currentL->index == -unitLiteralIndex) {
                if (currentL == itr->head) {
                    itr->head = currentL->next;
                    free(currentL);
                    currentL = itr->head;
                } else {
                    previousL->next = currentL->next;
                    free(currentL);
                    currentL = previousL->next;
                }
                continue;
            }
            previousL = currentL;
            currentL = currentL->next;
        }
        if (!clauseRemoved) {
            prev = itr;
            itr = itr->next;
        }
    }
    return 1;
}



// implements pure literal elimination algorithm
int pureLiteralElimination(struct Clause * root){
    int pureLiteralIndex = findPureLiteral(root);
    if (verbose && pureLiteralIndex != 0) printf("Easy case: Pure literal found %d\n", pureLiteralIndex);
    if (pureLiteralIndex == 0) return 0;

    // set the valuation for that literal
    if (verbose) printf("Setting literal %d to %s\n", abs(pureLiteralIndex), pureLiteralIndex > 0 ? "true" : "false");
    valuation[abs(pureLiteralIndex)] = pureLiteralIndex > 0 ? 1 : 0;

    // iterate over the clause set to remove clauses containing the pure literal
    struct Clause * itr = root;
    struct Clause * prev;
    while (itr != NULL){
        struct Literal * l = itr->head;
        while (l != NULL){
            if (l->index == pureLiteralIndex) {
                // pure literal found, remove whole clause and re-adjust pointers

                if (itr == root){
                    // the root has to change if we are removing the first clause
                    *root = *(root->next);
                    itr = NULL;
                } else {
                    prev->next = itr->next;
                    itr = prev;
                }
                break;
            }
            l = l->next;
        }
        prev = itr;
        itr = itr == NULL ? root : itr->next;
    }
    return 1;
}



// checks if all the remaining clauses contain non-conflicting literals
// i.e. for each literal remaining in the clause set, either positive or
// negative index should be present. If that's the case, satisfiability is solved.
int areAllClausesUnit(struct Clause * root){
//    printf("reached\n");
    int * literalLookup = (int*) calloc(variableNumber + 1, sizeof(int));

    struct Clause* itr = root;
    while (itr != NULL){
        struct Literal * l = itr->head;
        while (l != NULL){
            int seen = literalLookup[abs(l->index)];
            if (seen == 0) literalLookup[abs(l->index)] = sign(l->index);
                // if we previously have seen this literal with the opposite sign, return false
            else if (seen == -1 && sign(l->index) == 1) return 0;
            else if (seen == 1 && sign(l->index) == -1) return 0;
            l = l->next;
        }
        itr = itr->next;
    }

//    printf("reached 2\n");

    // if we reached here, that means the clause set contains no conflicting literals
    // iterate over the clause set one last time to decide their valuation
    itr = root;
    while (itr != NULL){
        struct Literal * l = itr->head;
        while (l != NULL){
            valuation[abs(l->index)] = l->index > 0 ? 1 : 0;
            l = l->next;
        }
        itr = itr->next;
    }

    // return true to terminate dpll
    return 1;
}

// returns if the clause set contains and empty clause with no literal within
int containsEmptyClause(struct Clause * root){
    struct Clause* itr = root;
    while (itr != NULL){
        // if the head pointer is null, no literals
        if(itr->head == NULL) return 1;
        itr = itr->next;
    }
    return 0;
}

// checks if the current state of the clause set represents a solution
int checkSolution(struct Clause * root){
    if (containsEmptyClause(root)) return UNSATISFIABLE;
    if (areAllClausesUnit(root)) return SATISFIABLE;
    return UNCERTAIN;
}

// returns a random literal index to perform branching
int chooseLiteral(struct Clause * root){
    // just return the first literal, it doesn't change the outcome
    // but it maybe better to use a smarter approach for speed
    // (e.g. choose the literal with most frequency)
    return root->head->index;
}

// deep clones a clause constructing a new clause and literal structs
struct Clause * cloneClause(struct Clause * origin){
    struct Clause * cloneClause = createClause();
    struct Literal * iteratorLiteral = origin->head;
    struct Literal * previousLiteral = NULL;

    // iterate over the clause to clone literals as well
    while (iteratorLiteral != NULL){
        struct Literal * literalClone = createLiteral();
        literalClone->index = iteratorLiteral->index;
        if (cloneClause->head == NULL) {
            cloneClause->head = literalClone;
        }
        if (previousLiteral != NULL) {
            previousLiteral->next = literalClone;
        }
        previousLiteral = literalClone;
        iteratorLiteral = iteratorLiteral->next;
    }
    return cloneClause;
}

struct Clause * branch(struct Clause * root, int literalIndex){
    if (verbose) printf("Hard case: Guessing %d = %s\n", abs(literalIndex), literalIndex > 0 ? "true" : "false");
    hard_case = 1;

    valuation[abs(literalIndex)] = literalIndex > 0 ? 1 : 0;

    struct Clause * newClone = NULL,
            * currentClause = NULL,
            * previousClause = NULL,
            * iterator = root;

    // deep clone each clause one by one
    while (iterator != NULL){
        struct Clause * clone = cloneClause(iterator);
        if (newClone == NULL) {
            newClone = clone;
        }
        if (previousClause != NULL) {
            previousClause->next = clone;
        }
        previousClause = clone;
        iterator = iterator->next;
    }

    struct Clause * addedClause = createClause();
    struct Literal * addedLiteral = createLiteral();
    addedLiteral->index = literalIndex;
    addedClause->head = addedLiteral;

    addedClause->next = newClone;
    return addedClause;
}

void removeLiteral(struct Literal * literal){
    while (literal != NULL) {
        struct Literal * next = literal->next;
        free(literal);
        literal = next;
    }
}

void removeClause(struct Clause * root){
    while (root != NULL) {
        struct Clause * next = root->next;
        if (root->head != NULL) removeLiteral(root->head);
        free(root);
        root = next;
    }
}

// DPLL algorithm with recursive backtracking
int dpll(struct Clause * root){

    int solution = checkSolution(root);
    if (solution != UNCERTAIN){

        return solution;
    }

    while(1){
        solution = checkSolution(root);
        if (solution != UNCERTAIN){

            return solution;
        }
        if (!unitPropagation(&root)) break;
    }

    while(1){
        int solution = checkSolution(root);
        if (solution != UNCERTAIN) {

            return solution;
        }
        if (!pureLiteralElimination(root)) break;
    }

    int literalIndex = chooseLiteral(root);

    if (dpll(branch(root, literalIndex)) == SATISFIABLE) return SATISFIABLE;

    if (verbose) printf("Contradiction: Backtracking and trying literal %d = %s\n", abs(literalIndex),literalIndex > 0 ? "false" : "true");
    return dpll(branch(root, -literalIndex));
}

struct Clause * readClauseSetFromInput(char cnf[][100], int numClauses, int bnf) {
    struct Clause *root = NULL, *currentClause = NULL, *previousClause = NULL;
    struct Literal *currentLiteral = NULL, *previousLiteral = NULL;

    if(bnf == 1){
        variableNumber = 26;
    }else{
        variableNumber = 729;
    };

    if (valuation == NULL) {
        valuation = (int*) calloc(variableNumber + 1, sizeof(int));
        for (int i = 0; i <= variableNumber; i++) {
            valuation[i] = -1;
        }
    }

    for (int i = 0; i < numClauses; i++) {
        currentClause = createClause();
        if (root == NULL) {
            root = currentClause;
        }
        if (previousClause != NULL) {
            previousClause->next = currentClause;
        }

        char *token = strtok(cnf[i], " ");
        while (token != NULL) {
            int isNegated = 0;

            // Check for negation (!)
            if (token[0] == '!') {
                isNegated = 1;
                token++;  // Skip the '!' character
            }

            int literalIndex = 0;

            if (bnf == 1) {
                // Handling ABCD tokens
                if (strlen(token) != 1 || token[0] < 'A' || token[0] > 'Z') {
                    fprintf(stderr, "Error: Invalid token format '%s'\n", token);
                    exit(EXIT_FAILURE);
                }
                literalIndex = token[0] - 'A' + 1;
            } else {
                // Extract val, row, col from the token (for n{val}_r{row}_c{col} format)
                int val, row, col;
                if (sscanf(token, "n%d_r%d_c%d", &val, &row, &col) != 3) {
                    fprintf(stderr, "Error: Invalid token format '%s'\n", token);
                    exit(EXIT_FAILURE);
                }

                // Calculate the literal index based on val, row, col
                literalIndex = (val - 1) + (row - 1) * 9 + (col - 1) * 81 + 1;
            }

            // Apply negation if necessary
            if (isNegated) {
                literalIndex = -literalIndex;
            }

            // Create and add the literal to the clause
            currentLiteral = createLiteral();
            currentLiteral->index = literalIndex;

            valuation[abs(literalIndex)] = 0;

            if (currentClause->head == NULL) {
                currentClause->head = currentLiteral;
            } else if (previousLiteral != NULL) {
                previousLiteral->next = currentLiteral;
            }

            previousLiteral = currentLiteral;
            token = strtok(NULL, " ");
        }

        previousClause = currentClause;
    }

    return root;
}

void writeSolutionToOutput(struct Clause * root, int * valuation, int bnf) {
    if (bnf == 1) {
        printf("Solution:\n");

        for (int i = 1; i <= variableNumber; i++) {
            if (valuation[i] != -1) {
                // Convert the index (1-26) to a letter (A-Z)
                char letter = 'A' + (i - 1);

                // Output the letter and its assignment (True/False)
                printf("%c = %s\n", letter, valuation[i] == 1 ? "True" : "False");
            }
        }
    }else{
        printf("Solution:\n");
        int sudoku_board[9][9] = {0};  // Initialize an empty Sudoku board

        for (int i = 1; i <= variableNumber; i++) {
            if (valuation[i] != -1) { // If the variable has been assigned
                // Reverse the index to val, row, col
                int val = (i - 1) % 9 + 1;
                int row = ((i - 1) / 9) % 9 + 1;
                int col = (i - 1) / 81 + 1;

                // Print the solution for each variable
                if(verbose){
                    printf("n%d_r%d_c%d = %s\n", val, row, col, valuation[i] == 1 ? "True" : "False");
                }

                // Fill the Sudoku board if the variable is true
                if (valuation[i] == 1) {
                    sudoku_board[row - 1][col - 1] = val;
                }
            }
        }

        // Now print the filled Sudoku board
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                printf("%d ", sudoku_board[i][j]);
            }
            printf("\n");
        }
    }
}