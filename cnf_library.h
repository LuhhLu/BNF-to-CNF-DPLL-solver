#ifndef SUDOKU_CNF_LIBRARY_H
#define SUDOKU_CNF_LIBRARY_H

#include <stdbool.h>

// 
typedef struct Node {
    char op;  // Logical operator (e.g., 'v', '^', '!', etc.)
    char* var;  // Full variable name for complex variables (e.g., "n5_r3_c2")
    struct Node *left;
    struct Node *right;
} Node;

// 
Node* createNode(char op);
Node* createNodeFromVariable(char* var);
Node* parseExpression(char* expr, int start, int end);
Node* removeBiconditional(Node* root);
Node* removeImplication(Node* root);
Node* applyDeMorgan(Node* root);
Node* removeDoubleNegation(Node* root);
Node* distributeOrOverAnd(Node* root);
void storeCNF(Node* root, char cnfExpressions[][100], int* index);
bool isDuplicate(char cnfExpressions[][100], int index, char* expr);
void treeToString(Node* root, char* buffer);
void removeSpaces(char* expr);
int compareStrings(const void* a, const void* b);


#endif //SUDOKU_CNF_LIBRARY_H
