#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// 1. Next-Hop Table (Array of integers)
extern int *next_hop_table;
extern int next_hop_count;

// 2. Prefix Vector
struct prefix_vector_entry {
    uint32_t prefix;
    int prefixLength;
    int next_hop_index;
    int prefix_vector_index; // -1 means there is no shorter prefix
};
extern struct prefix_vector_entry *prefix_vector;
extern int prefix_vector_count;

// 3. Base Vector
struct base_vector_entry {
    uint32_t ip_string;
    int next_hop_index;
    int prefix_vector_index; // -1 means there is no shorter prefix
};
extern struct base_vector_entry *base_vector;
extern int base_vector_count;

// 4. LC-Trie Node (Modified for the paper architecture)
struct node {
    uint32_t prefix;     // (Only used during Phase 1 tree building)
    int prefixLength;    // (Only used during Phase 1 tree building)
    int outInterface;    // (Only used during Phase 1 tree building)

    int bitPosition; 
    int k; 
    struct node **children; 
    
    // --- NEW: Pointer from the leaf to the Base Vector ---
    int base_vector_index; // -1 if it is an internal node
    int prefix_vector_index;
};

// Phase 1-3 Declarations
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);
uint32_t getBits(uint32_t ip, int bitPosition, int k);
int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength);

// Paper Architecture Table Declarations
int addNextHop(int outInterface);
void buildTables(struct node *node, int current_prefix_idx);
int lookUp(struct node *root, uint32_t ip, int *memory_accesses);

// Phase 4-5 Declarations (Compression)
void patriciaCompress(struct node **node_ptr);
int countNodes(struct node *root);
void levelCompress(struct node *node);
int checkFull(struct node *node, int depth, int expected_bit);
struct node* getDescendantAtDepth(struct node *node, int bit_pattern, int depth);


#endif // TRIE_H