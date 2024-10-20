#include "cnf_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Create a new node
Node* createNode(char op) {
    Node* node = (Node*)malloc(sizeof(Node) + sizeof(Node*) * 2);
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->op = op;
    node->var = NULL;
    node->left = NULL;
    node->right = NULL;
    return node;
}

Node* createNodeFromVariable(char* var) {
    Node* node = createNode('\0');
    node->var = (char*)malloc(strlen(var) + 1);
    strcpy(node->var, var);
    return node;
}

// Deep copy a node
Node* copyNode(Node* root) {
    if (!root) return NULL;

    Node* newNode = createNode(root->op);

    if (root->var) {
        newNode->var = (char*)malloc(strlen(root->var) + 1);
        strcpy(newNode->var, root->var);
    }

    newNode->left = copyNode(root->left);
    newNode->right = copyNode(root->right);

    return newNode;
}

// Remove all spaces from the expression
void removeSpaces(char* expr) {
    char* i = expr;
    char* j = expr;
    while (*j) {
        if (*j != ' ') *i++ = *j;
        j++;
    }
    *i = '\0';
}

Node* parseExpression(char* expr, int start, int end) {
    // Strip outer parentheses
    while (expr[start] == '(' && expr[end] == ')') {
        int count = 0;
        for (int i = start; i <= end; i++) {
            if (expr[i] == '(') count++;
            if (expr[i] == ')') count--;
            if (count == 0 && i != end) break;
            if (i == end && count == 0) {
                start++;
                end--;
            }
        }
    }

    // Handle biconditional (<=>)
    for (int i = end, count = 0; i >= start; i--) {
        if (expr[i] == ')') count++;
        if (expr[i] == '(') count--;
        if (count == 0 && expr[i] == '>' && expr[i - 1] == '=' && expr[i - 2] == '<') {
            Node* node = createNode('<');
            node->left = parseExpression(expr, start, i - 3);
            node->right = parseExpression(expr, i + 1, end);
            return node;
        }
    }

    // Handle implication (=>)
    for (int i = end, count = 0; i >= start; i--) {
        if (expr[i] == ')') count++;
        if (expr[i] == '(') count--;
        if (count == 0 && expr[i] == '>' && expr[i - 1] == '=') {
            Node* node = createNode('>');
            node->left = parseExpression(expr, start, i - 2);
            node->right = parseExpression(expr, i + 1, end);
            return node;
        }
    }

    // Handle disjunction (v)
    for (int i = end, count = 0; i >= start; i--) {
        if (expr[i] == ')') count++;
        if (expr[i] == '(') count--;
        if (count == 0 && expr[i] == 'v') {
            Node* node = createNode('v');
            node->left = parseExpression(expr, start, i - 1);
            node->right = parseExpression(expr, i + 1, end);
            return node;
        }
    }

    // Handle conjunction (^)
    for (int i = end, count = 0; i >= start; i--) {
        if (expr[i] == ')') count++;
        if (expr[i] == '(') count--;
        if (count == 0 && expr[i] == '^') {
            Node* node = createNode('^');
            node->left = parseExpression(expr, start, i - 1);
            node->right = parseExpression(expr, i + 1, end);
            return node;
        }
    }

    // Handle negation (!)
    if (expr[start] == '!') {
        if (expr[start + 1] == '(') {
            int count = 0;
            for (int i = start + 1; i <= end; i++) {
                if (expr[i] == '(') count++;
                if (expr[i] == ')') count--;
                if (count == 0) {
                    Node* node = createNode('!');
                    node->left = parseExpression(expr, start + 1, i);
                    return node;
                }
            }
        } else {
            Node* node = createNode('!');
            node->left = parseExpression(expr, start + 1, end);
            return node;
        }
    }

    // Handle complex variables (n%d_r%d_c%d)
    if (expr[start] == 'n') {
        char var[100];
        int i = start, varIndex = 0;
        while (i <= end && expr[i] != ' ') {
            var[varIndex++] = expr[i++];
        }
        var[varIndex] = '\0';

        return createNodeFromVariable(var);
    }

    // Handle simple variables (A, B, C, D)
    if (start == end) {
        return createNode(expr[start]);
    }

    return NULL; // Return NULL if no match
}


// Remove biconditional nodes by converting them to equivalent implications
Node* removeBiconditional(Node* root) {
    if (!root) return NULL;

    root->left = removeBiconditional(root->left);
    root->right = removeBiconditional(root->right);

    if (root->op == '<') {
        Node* leftImp = createNode('>');
        leftImp->left = copyNode(root->left);
        leftImp->right = copyNode(root->right);

        Node* rightImp = createNode('>');
        rightImp->left = copyNode(root->right);
        rightImp->right = copyNode(root->left);

        Node* andNode = createNode('^');
        andNode->left = leftImp;
        andNode->right = rightImp;

        free(root);
        return andNode;
    }

    return root;
}

// Remove implication nodes by converting them to disjunctions
Node* removeImplication(Node* root) {
    if (!root) return NULL;

    root->left = removeImplication(root->left);
    root->right = removeImplication(root->right);

    if (root->op == '>') {
        Node* notNode = createNode('!');
        notNode->left = copyNode(root->left);

        Node* orNode = createNode('v');
        orNode->left = notNode;
        orNode->right = copyNode(root->right);

        free(root);
        return orNode;
    }

    return root;
}

// Apply De Morgan's laws to negate conjunctions and disjunctions
Node* applyDeMorgan(Node* root) {
    if (!root) return NULL;

    root->left = applyDeMorgan(root->left);
    root->right = applyDeMorgan(root->right);

    if (root->op == '!' && (root->left->op == 'v' || root->left->op == '^')) {
        Node* child = root->left;
        Node* newLeft = createNode('!');
        newLeft->left = copyNode(child->left);

        Node* newRight = createNode('!');
        newRight->left = copyNode(child->right);

        Node* newRoot = createNode(child->op == 'v' ? '^' : 'v');
        newRoot->left = applyDeMorgan(newLeft);
        newRoot->right = applyDeMorgan(newRight);

        free(root);
        free(child);
        return newRoot;
    }

    return root;
}

// Remove double negation nodes
Node* removeDoubleNegation(Node* root) {
    if (!root) return NULL;

    root->left = removeDoubleNegation(root->left);
    root->right = removeDoubleNegation(root->right);

    if (root->op == '!' && root->left && root->left->op == '!') {
        Node* newRoot = root->left->left;
        free(root->left);
        free(root);
        return newRoot;
    }

    return root;
}

Node* distributeOrOverAnd(Node* root) {
    if (root == NULL) return NULL;

    root->left = distributeOrOverAnd(root->left);
    root->right = distributeOrOverAnd(root->right);

    if (root->op == 'v') {
        if (root->left != NULL && root->left->op == '^') {
            Node* leftAnd = root->left;
            Node* newLeft = createNode('v');
            newLeft->left = copyNode(leftAnd->left);
            newLeft->right = copyNode(root->right);

            Node* newRight = createNode('v');
            newRight->left = copyNode(leftAnd->right);
            newRight->right = copyNode(root->right);

            Node* newRoot = createNode('^');
            newRoot->left = distributeOrOverAnd(newLeft);
            newRoot->right = distributeOrOverAnd(newRight);

            free(root);
            free(leftAnd);

            return newRoot;
        } else if (root->right != NULL && root->right->op == '^') {
            Node* rightAnd = root->right;
            Node* newLeft = createNode('v');
            newLeft->left = copyNode(root->left);
            newLeft->right = copyNode(rightAnd->left);

            Node* newRight = createNode('v');
            newRight->left = copyNode(root->left);
            newRight->right = copyNode(rightAnd->right);

            Node* newRoot = createNode('^');
            newRoot->left = distributeOrOverAnd(newLeft);
            newRoot->right = distributeOrOverAnd(newRight);

            // 释放当前节点和合取节点
            free(root);
            free(rightAnd);

            return newRoot;
        }
    }

    if (root->op == 'v') {
        if ((root->left && root->right) &&
            ((root->left->op == '!' && root->right->op == root->left->left->op) ||
             (root->right->op == '!' && root->left->op == root->right->left->op))) {
            free(root);
            return NULL;
        }
    }

    if (root->op == 'v' && (root->left == NULL || root->right == NULL)) {
        free(root);
        return NULL;
    }

    return root;
}



void treeToString(Node* root, char* buffer) {
    if (!root) return;

    if (root->op == '!') {  // Handle negation
        strcat(buffer, "!");
        treeToString(root->left, buffer);
    }
    else if (root->left || root->right) {  // Handle binary operations
        treeToString(root->left, buffer);
        int len = strlen(buffer);

        // Add operator or space for variable
        buffer[len] = (root->op == 'v') ? ' ' : root->op;
        buffer[len + 1] = '\0';

        treeToString(root->right, buffer);
    }
    else {  // Handle leaf nodes (either complex or simple vars)
        if (root->op == '\0') {
            strcat(buffer, root->var);  // Append full variable string for complex vars
        } else {
            int len = strlen(buffer);
            buffer[len] = root->op;  // Append single char var
            buffer[len + 1] = '\0';
        }
    }
}

void storeCNF(Node* root, char cnfExpressions[][100], int* index) {
    if (!root) return;

    if (root->op == '^') {  // Recursively store conjunctions
        storeCNF(root->left, cnfExpressions, index);
        storeCNF(root->right, cnfExpressions, index);
    } else {  // Convert and store the expression
        char buffer[100] = "";
        treeToString(root, buffer);
        strcpy(cnfExpressions[*index], buffer);
        (*index)++;
    }
}

bool isDuplicate(char cnfExpressions[][100], int index, char* expr) {
    for (int i = 0; i < index; i++) {
        if (strcmp(cnfExpressions[i], expr) == 0) return true;
    }
    return false;
}

int compareStrings(const void* a, const void* b) {
    return strlen((char*)a) - strlen((char*)b);  // Sort by string length
}
