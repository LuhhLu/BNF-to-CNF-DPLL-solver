#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "cnf_library.h"
#include "dpll_solver.h"

#define SIZE 9
#define SATISFIABLE 1
#define UNSATISFIABLE (-1)

int verbose = 0;  // Verbose mode flag
char *bnf_file = NULL;  // BNF file name (optional)
int sudoku_board[SIZE][SIZE] = {0};  // Sudoku board initialized to 0 (unset)
int bnf = -1;  // Extra credit flag

void parse_arguments(int argc, char *argv[]);
void parse_sudoku_inputs(int argc, char *argv[], int start_index);
void print_sudoku_board();
int generate_bnf_clauses(char bnfClauses[10000][100]);
void generate_cnf_clauses();
void generate_at_least_one_digit_clauses(char cnf[][100], int *index);
void generate_unique_row_clauses(char cnf[][100], int *index);
void generate_unique_column_clauses(char cnf[][100], int *index);
void generate_unique_block_clauses(char cnf[][100], int *index);
void parse_bnf_file(const char *filename);

void parse_arguments(int argc, char *argv[]) {
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
            i++;
        } else if (strcmp(argv[i], "-bnf") == 0) {
            bnf = 1;
            if (i + 1 < argc && strchr(argv[i + 1], '=') == NULL) {
                bnf_file = argv[i + 1];
                i += 2;
            } else {
                i++;
                break;
            }
        } else {
            break;
        }
    }

    if (i < argc) {
        parse_sudoku_inputs(argc, argv, i);
    } else if (!bnf_file) {
        fprintf(stderr, "Error: No BNF file or Sudoku inputs provided\n");
        exit(EXIT_FAILURE);
    }
}

void parse_sudoku_inputs(int argc, char *argv[], int start_index) {
    for (int i = start_index; i < argc; i++) {
        int row, col, val;

        if (sscanf(argv[i], "%1d%1d=%1d", &row, &col, &val) != 3) {
            fprintf(stderr, "Error: Invalid input format '%s'\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        if (row < 1 || row > 9 || col < 1 || col > 9 || val < 1 || val > 9) {
            fprintf(stderr, "Error: Values out of range in '%s'\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        if (sudoku_board[row - 1][col - 1] != 0) {
            fprintf(stderr, "Error: Duplicate assignment at position %d%d\n", row, col);
            exit(EXIT_FAILURE);
        }

        sudoku_board[row - 1][col - 1] = val;
    }
}


void parse_bnf_file(const char *bnf_file) {
    FILE *file = NULL;
    char line[100];
    char cnfExpressions[20000][100];
    char bnfClauses[20000][100];  // Define the array to store BNF clauses if generated
    int index = 0, numClauses = 0;

    // If bnf_file is provided, read from the file
    if (bnf_file) {
        file = fopen(bnf_file, "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open BNF file '%s'\n", bnf_file);
            exit(EXIT_FAILURE);
        }

        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;  // Remove newline
            removeSpaces(line);

            if (verbose) {
                printf("BNF clause: %s\n", line);
            }

            // Process the BNF clause
            Node* root = parseExpression(line, 0, strlen(line) - 1);
            root = removeBiconditional(root);
            root = removeImplication(root);
            root = applyDeMorgan(root);
            root = removeDoubleNegation(root);
            root = distributeOrOverAnd(root);

            int previousIndex = index;
            storeCNF(root, cnfExpressions, &index);

            if (verbose) {
                printf("Converted CNF clauses:\n");
                for (int i = previousIndex; i < index; i++) {
                    printf("%s\n", cnfExpressions[i]);
                }
                printf("\n");
            }
        }
        fclose(file);
    } else {
        // No file provided, generate BNF clauses dynamically
        numClauses = generate_bnf_clauses(bnfClauses);  // Generate clauses

        // Process the generated BNF clauses
        for (int i = 0; i < numClauses; i++) {
            strncpy(line, bnfClauses[i], sizeof(line));  // Copy the clause
            removeSpaces(line);
            if (verbose) {
                printf("BNF clause: %s\n", line);
            }

            // Process the BNF clause
            Node* root = parseExpression(line, 0, strlen(line) - 1);
            root = removeBiconditional(root);
            root = removeImplication(root);
            root = applyDeMorgan(root);
            root = removeDoubleNegation(root);
            root = distributeOrOverAnd(root);

            int previousIndex = index;
            storeCNF(root, cnfExpressions, &index);

            if (verbose) {
                printf("Converted CNF clauses:\n");
                for (int i = previousIndex; i < index; i++) {
                    printf("%s\n", cnfExpressions[i]);
                }
                printf("\n");
            }
        }
    }

    // Remove duplicates and sort CNF clauses (as before)
    char uniqueCNF[20000][100];
    int uniqueIndex = 0;
    for (int i = 0; i < index; i++) {
        if (!isDuplicate(uniqueCNF, uniqueIndex, cnfExpressions[i])) {
            strcpy(uniqueCNF[uniqueIndex++], cnfExpressions[i]);
        }
    }

    qsort(uniqueCNF, uniqueIndex, sizeof(uniqueCNF[0]), compareStrings);

    if (verbose) {
        printf("ALL CNF clauses:\n");
        for (int i = 0; i < uniqueIndex; i++) {
            printf("%s\n", uniqueCNF[i]);
        }
    }

    // Continue processing the CNF clauses
    struct Clause *root = readClauseSetFromInput(uniqueCNF, uniqueIndex, bnf_file ? 1 : -1);

    int result = dpll(root);
    if(verbose){
        printf(result == SATISFIABLE ? "SATISFIABLE\n" : "UNSATISFIABLE\n");
    }

    writeSolutionToOutput(root, valuation, bnf_file ? 1 : -1);
}


void print_sudoku_board() {
    printf("Initial Sudoku Board:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", sudoku_board[i][j]);
        }
        printf("\n");
    }
}



int generate_bnf_clauses(char bnfClauses[10000][100]) {
    int index = 0;

    // Store BNF clauses for initial values
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            if (sudoku_board[row][col] != 0) {
                snprintf(bnfClauses[index++], 100, "n%d_r%d_c%d", sudoku_board[row][col], row + 1, col + 1);
            }
        }
    }

    // Cell constraint: each cell must contain exactly one number
    for (int row = 1; row <= SIZE; row++) {
        for (int col = 1; col <= SIZE; col++) {
            // At least one number in each cell
            char clause[500] = "(";
            for (int num = 1; num <= SIZE; num++) {
                char part[50];
                snprintf(part, 50, "n%d_r%d_c%d", num, row, col);
                strcat(clause, part);
                if (num < SIZE) strcat(clause, " v ");
            }
            strcat(clause, ")");
            snprintf(bnfClauses[index++], 500, "%s", clause);

            // No more than one number in each cell (pairwise exclusion)
            for (int num1 = 1; num1 <= SIZE; num1++) {
                for (int num2 = num1 + 1; num2 <= SIZE; num2++) {
                    snprintf(bnfClauses[index++], 100, "(!n%d_r%d_c%d v !n%d_r%d_c%d)", num1, row, col, num2, row, col);
                }
            }
        }
    }

    // Row constraint: each number must appear exactly once in each row
    for (int num = 1; num <= SIZE; num++) {
        for (int row = 1; row <= SIZE; row++) {
            // At least one number in the row
            char clause[500] = "(";
            for (int col = 1; col <= SIZE; col++) {
                char part[50];
                snprintf(part, 50, "n%d_r%d_c%d", num, row, col);
                strcat(clause, part);
                if (col < SIZE) strcat(clause, " v ");
            }
            strcat(clause, ")");
            snprintf(bnfClauses[index++], 500, "%s", clause);

            // No more than one number in each row (pairwise exclusion)
            for (int col1 = 1; col1 <= SIZE; col1++) {
                for (int col2 = col1 + 1; col2 <= SIZE; col2++) {
                    snprintf(bnfClauses[index++], 100, "(!n%d_r%d_c%d v !n%d_r%d_c%d)", num, row, col1, num, row, col2);
                }
            }
        }
    }

    // Column constraint: each number must appear exactly once in each column
    for (int num = 1; num <= SIZE; num++) {
        for (int col = 1; col <= SIZE; col++) {
            // At least one number in the column
            char clause[500] = "(";
            for (int row = 1; row <= SIZE; row++) {
                char part[50];
                snprintf(part, 50, "n%d_r%d_c%d", num, row, col);
                strcat(clause, part);
                if (row < SIZE) strcat(clause, " v ");
            }
            strcat(clause, ")");
            snprintf(bnfClauses[index++], 500, "%s", clause);

            // No more than one number in each column (pairwise exclusion)
            for (int row1 = 1; row1 <= SIZE; row1++) {
                for (int row2 = row1 + 1; row2 <= SIZE; row2++) {
                    snprintf(bnfClauses[index++], 100, "(!n%d_r%d_c%d v !n%d_r%d_c%d)", num, row1, col, num, row2, col);
                }
            }
        }
    }

    // Block constraint: each number must appear exactly once in each block
    for (int num = 1; num <= SIZE; num++) {
        for (int blockRow = 0; blockRow < 3; blockRow++) {
            for (int blockCol = 0; blockCol < 3; blockCol++) {
                // At least one number in the block
                char clause[500] = "(";
                for (int i = 1; i <= 3; i++) {
                    for (int j = 1; j <= 3; j++) {
                        int row = blockRow * 3 + i;
                        int col = blockCol * 3 + j;
                        char part[50];
                        snprintf(part, 50, "n%d_r%d_c%d", num, row, col);
                        strcat(clause, part);
                        if (!(i == 3 && j == 3)) strcat(clause, " v ");
                    }
                }
                strcat(clause, ")");
                snprintf(bnfClauses[index++], 500, "%s", clause);

                // No more than one number in each block (pairwise exclusion)
                for (int i1 = 1; i1 <= 3; i1++) {
                    for (int j1 = 1; j1 <= 3; j1++) {
                        int row1 = blockRow * 3 + i1;
                        int col1 = blockCol * 3 + j1;
                        for (int i2 = i1; i2 <= 3; i2++) {
                            for (int j2 = (i2 == i1 ? j1 + 1 : 1); j2 <= 3; j2++) {
                                int row2 = blockRow * 3 + i2;
                                int col2 = blockCol * 3 + j2;
                                snprintf(bnfClauses[index++], 100, "(!n%d_r%d_c%d v !n%d_r%d_c%d)", num, row1, col1, num, row2, col2);
                            }
                        }
                    }
                }
            }
        }
    }

    return index;
}


void generate_cnf_clauses() {
    char cnfClauses[10000][100];
    int index = 0;

    // Generate CNF clauses for initial known values
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            if (sudoku_board[row][col] != 0) {
                // Store in the format n{value}_r{row+1}_c{col+1}
                snprintf(cnfClauses[index++], 100, "n%d_r%d_c%d", sudoku_board[row][col], row + 1, col + 1);
            }
        }
    }

    // Generate Sudoku constraints in CNF form
    generate_at_least_one_digit_clauses(cnfClauses, &index);
    generate_unique_row_clauses(cnfClauses, &index);
    generate_unique_column_clauses(cnfClauses, &index);
    generate_unique_block_clauses(cnfClauses, &index);

    if (verbose) {
        printf("Generated CNF Clauses:\n");
        for (int i = 0; i < index; i++) {
            printf("%s\n", cnfClauses[i]);
        }
    }

    struct Clause *root = readClauseSetFromInput(cnfClauses, index, bnf);

    int result = dpll(root);
    if (result == SATISFIABLE) {
        if(verbose){
            printf("SATISFIABLE\n");
        }
        writeSolutionToOutput(root, valuation, bnf);
    } else {
        if(verbose){
            printf("UNSATISFIABLE\n");
        }
    }

}

// Ensure each cell has at least one digit
void generate_at_least_one_digit_clauses(char cnf[][100], int *index) {
    for (int row = 1; row <= SIZE; row++) {
        for (int col = 1; col <= SIZE; col++) {
            char clause[100] = "";
            for (int val = 1; val <= SIZE; val++) {
                char literal[16];
                snprintf(literal, sizeof(literal), "n%d_r%d_c%d ", val, row, col);
                strcat(clause, literal);
            }
            strcpy(cnf[(*index)++], clause);
        }
    }
}

// Ensure each number appears at most once per row
void generate_unique_row_clauses(char cnf[][100], int *index) {
    for (int row = 1; row <= SIZE; row++) {
        for (int val = 1; val <= SIZE; val++) {
            for (int col1 = 1; col1 <= SIZE; col1++) {
                for (int col2 = col1 + 1; col2 <= SIZE; col2++) {
                    snprintf(cnf[(*index)++], 100, "!n%d_r%d_c%d !n%d_r%d_c%d", val, row, col1, val, row, col2);
                }
            }
        }
    }
}

// Ensure each number appears at most once per column
void generate_unique_column_clauses(char cnf[][100], int *index) {
    for (int col = 1; col <= SIZE; col++) {
        for (int val = 1; val <= SIZE; val++) {
            for (int row1 = 1; row1 <= SIZE; row1++) {
                for (int row2 = row1 + 1; row2 <= SIZE; row2++) {
                    snprintf(cnf[(*index)++], 100, "!n%d_r%d_c%d !n%d_r%d_c%d", val, row1, col, val, row2, col);
                }
            }
        }
    }
}

// Ensure each number appears at most once per 3x3 block
void generate_unique_block_clauses(char cnf[][100], int *index) {
    for (int val = 1; val <= SIZE; val++) {
        for (int block_row = 0; block_row < 3; block_row++) {
            for (int block_col = 0; block_col < 3; block_col++) {
                for (int cell1 = 0; cell1 < 9; cell1++) {
                    for (int cell2 = cell1 + 1; cell2 < 9; cell2++) {
                        int row1 = block_row * 3 + cell1 / 3 + 1;
                        int col1 = block_col * 3 + cell1 % 3 + 1;
                        int row2 = block_row * 3 + cell2 / 3 + 1;
                        int col2 = block_col * 3 + cell2 % 3 + 1;
                        snprintf(cnf[(*index)++], 100, "!n%d_r%d_c%d !n%d_r%d_c%d", val, row1, col1, val, row2, col2);
                    }
                }
            }
        }
    }
}


int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);

    if (bnf_file) {
        parse_bnf_file(bnf_file);
    } else {
        if (verbose) {
            print_sudoku_board();
        }
        if (bnf == 1){
            parse_bnf_file(bnf_file);

        }else{
            generate_cnf_clauses();
        }
    }

    return 0;
}



