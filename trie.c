#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

// Creates a new node in memory, initialized by default as an empty binary node
struct node* createNode(int bitPos) {
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
    if (newNode == NULL) {
        printf("Memory error when creating node\n");
        exit(1);
    }
    
    newNode->prefix = 0;
    newNode->prefixLength = 0;
    newNode->bitPosition = bitPos;
    newNode->outInterface = -1; // -1 means this node has no valid output route
    newNode->k = 1;             // k=1 means 2^1 = 2 children (standard binary)
    
    // calloc allocates memory for the 2 children and automatically initializes them to NULL
    newNode->children = (struct node**)calloc(2, sizeof(struct node*)); 
    if (newNode->children == NULL) {
        printf("Memory error when creating children array\n");
        exit(1);
    }
    
    return newNode;
}

// Inserts a prefix into the standard binary tree (uncompressed)
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface) {
    struct node *current = root;
    
    // Read the IP from left (bit 0) to right until reaching the prefix length
    for (int i = 0; i < prefixLength; i++) {
        
        // Extract the bit at position 'i'. 
        // Since 'i' goes from 0 to 31 (left to right), the shift amount is (31 - i)
        int bit = (ip >> (31 - i)) & 1;
        
        // If the path does not exist, create the intermediate node
        if (current->children[bit] == NULL) {
            //
