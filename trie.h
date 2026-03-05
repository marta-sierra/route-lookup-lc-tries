#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// Node structure definition
struct node {
    // Forwarding Info
    uint32_t prefix; 
    int prefixLength;
    int bitPosition;  // Next bit to evaluate (index 0 to 31)
    int outInterface; // We use -1 to indicate it is an intermediate node (no valid route)

    // Children
    int k;                   // Branching factor: 2^k children
    struct node **children;  // Dynamic array of pointers to children
};

// Phase 1 function declarations (Tree Building)
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);

// Phase 3 function declarations (Lookup)
uint32_t getBits(uint32_t ip, int bitPosition, int k);
int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength);
int lookUp(struct node *root, uint32_t ip, int *memory_accesses);

// Phase 4 function declarations (Patricia Compression & Utils)
void patriciaCompress(struct node **node_ptr);
int countNodes(struct node *root);

#endif // TRIE_H