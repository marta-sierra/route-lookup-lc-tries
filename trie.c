#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

// Global variables for the external tables
int *next_hop_table = NULL;
int next_hop_count = 0;
struct prefix_vector_entry *prefix_vector = NULL;
int prefix_vector_count = 0;
struct base_vector_entry *base_vector = NULL;
int base_vector_count = 0;

// =======================================================
// --- PHASE 1: TREE BUILDING ---
// =======================================================
struct node* createNode(int bitPos) {
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
    newNode->prefix = 0;
    newNode->prefixLength = 0;
    newNode->bitPosition = bitPos;
    newNode->outInterface = -1; 
    newNode->k = 1;             
    newNode->base_vector_index = -1; 
    newNode->prefix_vector_index = -1;
    newNode->children = (struct node**)calloc(2, sizeof(struct node*)); 
    return newNode;
}

void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface) {
    struct node *current = root;
    for (int i = 0; i < prefixLength; i++) {
        int bit = (ip >> (31 - i)) & 1;
        if (current->children[bit] == NULL) {
            current->children[bit] = createNode(i + 1);
        }
        current = current->children[bit];
    }
    current->outInterface = outInterface;
    current->bitPosition = prefixLength;
    current->prefix = ip;
    current->prefixLength = prefixLength;
}

// =======================================================
// --- PHASE 2.5: BUILD PAPER TABLES ---
// =======================================================

// Helper to avoid repeating interfaces in the next-hop table
int addNextHop(int outInterface) {
    for(int i = 0; i < next_hop_count; i++) {
        if(next_hop_table[i] == outInterface) return i;
    }
    next_hop_table[next_hop_count] = outInterface;
    return next_hop_count++;
}

// Traverses the initial tree and populates Base Vector, Prefix Vector, and Next Hop Table
void buildTables(struct node *node, int current_prefix_idx) {
    if (node == NULL) return;
    int my_prefix_idx = current_prefix_idx;

    if (node->outInterface != -1) {
        int nh_idx = addNextHop(node->outInterface);

        // If it is a leaf, the information goes to the Base Vector
        if (node->children[0] == NULL && node->children[1] == NULL) {
            base_vector[base_vector_count].ip_string = node->prefix;
            base_vector[base_vector_count].next_hop_index = nh_idx;
            base_vector[base_vector_count].prefix_vector_index = current_prefix_idx;
            
            node->base_vector_index = base_vector_count;
            base_vector_count++;
        } 
        // If it is an intermediate node, the information goes to the Prefix Vector
        else {
            prefix_vector[prefix_vector_count].prefix = node->prefix;
            prefix_vector[prefix_vector_count].prefixLength = node->prefixLength;
            prefix_vector[prefix_vector_count].next_hop_index = nh_idx;
            prefix_vector[prefix_vector_count].prefix_vector_index = current_prefix_idx;
            
            my_prefix_idx = prefix_vector_count;
            prefix_vector_count++;
            
            node->base_vector_index = -1; // Internal nodes do not point to the Base Vector
        }
    }
    node->prefix_vector_index = my_prefix_idx;
    // Recursive call for all existing children
    if (node->children != NULL) {
        int num_children = 1 << node->k;
        for (int i = 0; i < num_children; i++) {
            buildTables(node->children[i], my_prefix_idx);
        }
    }
}

// =======================================================
// --- PHASE 3: LOOKUP FUNCTIONS ---
// =======================================================
uint32_t getBits(uint32_t ip, int bitPosition, int k) { 
    int shift_amount = 32 - (bitPosition + k); 
    uint32_t mask = (1 << k) - 1; 
    return (ip >> shift_amount) & mask; 
}

int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength) { 
    if (prefixLength == 0) return 1; 
    int shift_amount = 32 - prefixLength; 
    return (ip >> shift_amount) == (prefix >> shift_amount); 
}
// STRICT PAPER LOOKUP ALGORITHM
int lookUp(struct node *root, uint32_t ip, int *memory_accesses) {
    struct node *current = root;
    *memory_accesses = 0;
    
    // 1. Trie Search (Fast-Forward)
    while (current != NULL) {
        (*memory_accesses)++;
        if (current->children == NULL) break; // Llegamos a una hoja
        int index = getBits(ip, current->bitPosition, current->k);
        if (current->children[index] == NULL) break; // ¡Nos caemos del árbol en un nodo interno!
        current = current->children[index]; 
    }
    
    if (current == NULL) return 0;

    int pref_idx = -1;

    // 2. Comprobamos dónde hemos parado
    if (current->base_vector_index != -1) {
        // Hemos llegado a una hoja. Miramos el Base Vector.
        (*memory_accesses)++;
        struct base_vector_entry bv = base_vector[current->base_vector_index];

        if (checkPrefixMatch(ip, bv.ip_string, current->prefixLength) == 1) {
            // Éxito exacto
            (*memory_accesses)++;
            return next_hop_table[bv.next_hop_index];
        }
        // Si falla, preparamos el índice para mirar el Prefix Vector
        pref_idx = bv.prefix_vector_index;
    } else {
        // Nos hemos caído en un nodo interno. 
        // Pasamos directamente al Prefix Vector asociado a esta rama.
        pref_idx = current->prefix_vector_index;
    }

    // 3. Prefix Vector Lookup (Backtracking)
    while (pref_idx != -1) {
        (*memory_accesses)++;
        struct prefix_vector_entry pv = prefix_vector[pref_idx];
        
        if (checkPrefixMatch(ip, pv.prefix, pv.prefixLength) == 1) {
            (*memory_accesses)++;
            return next_hop_table[pv.next_hop_index];
        }
        pref_idx = pv.prefix_vector_index;
    }
    
    return 0; // Default route
}

// =======================================================
// --- PHASE 4 & 5: COMPRESSION & UTILS ---
// =======================================================
int countNodes(struct node *root) {
    if (root == NULL) return 0;
    int count = 1; 
    int num_children = 1 << root->k; 
    for (int i = 0; i < num_children; i++) {
        if (root->children[i] != NULL) count += countNodes(root->children[i]);
    }
    return count;
}

void patriciaCompress(struct node **node_ptr) {
    if (*node_ptr == NULL) return;
    struct node *node = *node_ptr;

    // Base Case: Leaf
    if (node->children[0] == NULL && node->children[1] == NULL) return;
    
    // Bifurcation: Cannot compress
    if (node->children[0] != NULL && node->children[1] != NULL) {
        patriciaCompress(&(node->children[0]));
        patriciaCompress(&(node->children[1]));
        return;
    }

    struct node *child = (node->children[0] != NULL) ? node->children[0] : node->children[1];

    // If it holds a valid route or is the root, keep it and evaluate child
    if (node->outInterface != -1 || node->bitPosition == 0) {
        if (node->children[0] != NULL) patriciaCompress(&(node->children[0]));
        else patriciaCompress(&(node->children[1]));
    } else {
        // Bypass and free the useless intermediate node
        *node_ptr = child;
        free(node->children);
        free(node);
        patriciaCompress(node_ptr);
    }
}

int checkFull(struct node *node, int depth, int expected_bit) {
    if (node == NULL) return 0;
    
    // THE BUG FIX: The bits MUST be contiguous! If Patricia skipped bits, abort LC-Trie here.
    if (node->bitPosition != expected_bit) return 0; 
    
    if (depth == 0) return 1;
    if (node->k != 1) return 0; 
    
    // Check both children expecting the strictly next bit
    return checkFull(node->children[0], depth - 1, expected_bit + 1) && 
           checkFull(node->children[1], depth - 1, expected_bit + 1);
}

struct node* getDescendantAtDepth(struct node *node, int bit_pattern, int depth) {
    struct node *current = node;
    for (int i = depth - 1; i >= 0; i--) {
        if (current == NULL) return NULL;
        int bit = (bit_pattern >> i) & 1;
        current = current->children[bit];
    }
    return current;
}
void levelCompress(struct node *node) {
    if (node == NULL) return;

    int k = 1;
    if (node->k == 1) {
        while ((node->bitPosition + k) <= 32) {
            // Pass the current bitPosition to ensure strict continuity
            if (checkFull(node, k, node->bitPosition)) k++;
            else break;
        }
        k--; 
    }

    if (k > 1) {
        int num_new_children = 1 << k; 
        struct node **new_children_array = (struct node**)calloc(num_new_children, sizeof(struct node*));
        for (int i = 0; i < num_new_children; i++) {
            new_children_array[i] = getDescendantAtDepth(node, i, k);
        }
        node->k = k;
        free(node->children); // Free the old array of pointers
        node->children = new_children_array;
    }

    int current_num_children = 1 << node->k;
    for (int i = 0; i < current_num_children; i++) {
        levelCompress(node->children[i]);
    }
}