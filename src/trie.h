#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// 2. Prefix Vector
struct prefix_vector_entry {
    int length;
    int next_hop;
    int prefix_pointer; // -1 means there is no shorter prefix
};
extern struct prefix_vector_entry *prefix_vector;
//extern int prefix_vector_count;

// 3. Base Vector
struct base_vector_entry {
    uint32_t ip_string;
    int next_hop;
    int prefix_pointer; // -1 means there is no shorter prefix
};
extern struct base_vector_entry *base_vector;
//extern int base_vector_count;

struct main_vector_entry {
    int branch;
    int skip;
    int pointer; 
};
extern struct main_vector_entry *main_vector;

// 4. LC-Trie Node (Modified for the paper architecture)
struct node {
    uint32_t prefix;     
    int prefixLength;    
    int outInterface;
    int k; // 2^k hijos, mirar k bits
    struct node **children;
    // Depth/bit position of this node in the original binary trie (0..32).
    // Inserted nodes set this to the current prefix length.
    int bitPosition;
    int temp_id; 
    int temp_skip;
    int temp_prefix_index; // inicializar todos a menos 1
};

// Phase 1-3 Declarations
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);
uint32_t getBits(uint32_t ip, int bitPosition, int k);
int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength);

// Paper Architecture Table Declarations
void buildprefixTable(struct node *root);
void deleteSingleChildren(struct node **root_ptr);
void buildMainBaseTables(struct node *root);
int lookUp(struct node *root, uint32_t ip, int *memory_accesses);

// Phase 4-5 Declarations (Compression)
void patriciaCompress(struct node **node_ptr);
int countNodes(struct node *root);
void levelCompress(struct node *node);
int checkFull(struct node *node, int depth, int expected_bit);
struct node* getDescendantAtDepth(struct node *node, int bit_pattern, int depth);

// borrar
void printTrieStructure(struct node *node, int depth);

#endif // TRIE_H

