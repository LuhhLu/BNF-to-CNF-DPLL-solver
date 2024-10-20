//
// Created by Morningstar on 2024/10/19.
//

#ifndef SUDOKU_DPLL_SOLVER_H
#define SUDOKU_DPLL_SOLVER_H

// Define the Clause and Literal structures
struct Literal {
    struct Literal * next;
    int index;
};

struct Clause {
    struct Literal * head;
    struct Clause * next;
};

extern int *valuation;
extern int verbose;

// Declare DPLL functions
int dpll(struct Clause * root);
struct Clause * readClauseSetFromInput(char cnf[][100], int numClauses, int bnf);
void removeClause(struct Clause * root);
void writeSolutionToOutput(struct Clause * root, int * valuation, int bnf);

#endif //SUDOKU_DPLL_SOLVER_H
