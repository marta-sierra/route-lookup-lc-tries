#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// Node structure definition
struct node {
    // Forwarding Info
    uint32_t prefix; 
    int prefixLength;
    int bitPosition;  // Next bit to evaluate (index 0 to 31)
    int outInterface; // We will use -1 to indicate it is NULL / has no interface

    // Children
    int k;                   // Branching factor: 2^k children
    struct node **children;  // Dynamic array of pointers to children
};

// Phase 1 function declarations
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);

#endif // TRIE_H
