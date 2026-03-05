#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

// =======================================================
// --- PHASE 1: TREE BUILDING ---
// =======================================================

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
            // The new node will evaluate the next bit, so its position is i+1
            current->children[bit] = createNode(i + 1);
        }
        
        // Move down to the child
        current = current->children[bit];
    }
    
    // Upon exiting the loop, we have reached the end of the prefix.
    // Store the routing information in this final node.
    current->outInterface = outInterface;
    current->bitPosition = prefixLength;
    current->prefix = ip;
    current->prefixLength = prefixLength;
}

// =======================================================
// --- PHASE 3: LOOKUP FUNCTIONS ---
// =======================================================

// Helper function: Extracts 'k' bits starting from 'bitPosition' going downwards. 
uint32_t getBits(uint32_t ip, int bitPosition, int k) { 
    int shift_amount = 32 - (bitPosition + k); 
    uint32_t mask = (1 << k) - 1; 
    uint32_t extracted_bits = (ip >> shift_amount) & mask; 
    return extracted_bits; 
}

// Helper function: Compares the top prefixLength bits of the IP and the Prefix
int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength) { 
    if (prefixLength == 0) { 
        return 1; // 1 means 'true' in C
    }
    
    int shift_amount = 32 - prefixLength; 
    uint32_t ip_shifted = ip >> shift_amount; 
    uint32_t prefix_shifted = prefix >> shift_amount; 
    
    if (ip_shifted == prefix_shifted) { 
        return 1; 
    } else { 
        return 0; // 0 means 'false'
    }
}

// LOOKUP FUNCTION: Traverses the trie and updates memory accesses
int lookUp(struct node *root, uint32_t ip, int *memory_accesses) {
    struct node *current = root;
    int best_match_interface = root->outInterface;
    *memory_accesses = 0;
    
    while (current != NULL) {
        (*memory_accesses)++;
        
        // Check if this node holds a valid route 
        if (current->outInterface != -1) { 
            // Verify the prefix matches (necessary because Patricia skips bits) 
            if (checkPrefixMatch(ip, current->prefix, current->prefixLength) == 1) {
                best_match_interface = current->outInterface;
            }
        }
            
        // Extract the required bits to determine which branch to take
        int index = getBits(ip, current->bitPosition, current->k);
        
        // Jump to the child. 
        // Note: If the child is NULL (we are at a leaf), the while loop breaks at the top.
        current = current->children[index]; 
    }
    
    return best_match_interface;
}

// =======================================================
// --- PHASE 4: COMPRESSION & UTILS ---
// =======================================================

// Helper function: Counts the total number of non-null nodes in the tree
int countNodes(struct node *root) {
    if (root == NULL) return 0;
    
    int count = 1; // Count the current node
    
    // Sum the nodes from all existing branches
    // In Patricia/Standard Trie, k=1 (2 children)
    int num_children = 1 << root->k; 
    for (int i = 0; i < num_children; i++) {
        if (root->children[i] != NULL) {
            count += countNodes(root->children[i]);
        }
    }
    return count;
}

// PATRICIA COMPRESSION (Safe version using Double Pointers)
void patriciaCompress(struct node **node_ptr) {
    if (*node_ptr == NULL) return;
    struct node *node = *node_ptr;

    // 1. BASE CASE: It is a leaf, cannot be compressed further
    if (node->children[0] == NULL && node->children[1] == NULL) {
        return;
    }

    // 2. BIFURCATION: It has two children, so it's a necessary routing point. 
    // We cannot compress it, so we just evaluate its children.
    if (node->children[0] != NULL && node->children[1] != NULL) {
        patriciaCompress(&(node->children[0]));
        patriciaCompress(&(node->children[1]));
        return;
    }

    // 3. IT ONLY HAS ONE CHILD
    struct node *child = (node->children[0] != NULL) ? node->children[0] : node->children[1];

    // If the current node holds a valid route (outInterface != -1) OR is the root (bitPosition == 0), 
    // we CANNOT compress it. We just move on to evaluate its child.
    if (node->outInterface != -1 || node->bitPosition == 0) {
        if (node->children[0] != NULL) {
            patriciaCompress(&(node->children[0]));
        } else {
            patriciaCompress(&(node->children[1]));
        }
    } 
    // IT IS A USELESS NODE (1 child and no route): WE COMPRESS!
    else {
        // Point the parent's pointer directly to the grandchild
        *node_ptr = child;
        
        // Free the memory of the useless node we just bypassed
        free(node->children);
        free(node);
        
        // Recursive call on the node that took its place (in case it is also useless)
        patriciaCompress(node_ptr);
    }
}