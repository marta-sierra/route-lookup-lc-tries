#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

// Global variables for the external tables
struct prefix_vector_entry *prefix_vector = NULL;
struct base_vector_entry *base_vector = NULL;
struct main_vector_entry *main_vector = NULL;

static int prefix_vector_count = 0;
static int base_vector_count = 0;
static int main_vector_count = 0;

// =======================================================
// --- PHASE 1: TREE BUILDING ---
// =======================================================
struct node* createNode(int bitPos) {
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
    newNode->prefix = 0;
    newNode->prefixLength = bitPos;
    newNode->bitPosition = bitPos;
    // outInterface == 0 means "no route" (MISS / default route handling).
    newNode->outInterface = 0;
    newNode->k = 1;
    newNode->children = (struct node**)calloc(2, sizeof(struct node*));
    newNode->temp_id = -1;
    newNode->temp_skip = 0;
    newNode->temp_prefix_index = -1;
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
    current->prefix = ip;
    current->prefixLength = prefixLength;
    current->bitPosition = prefixLength;
}


// =======================================================
// --- PHASE ? LOOKUP FUNCTIONS ---
// =======================================================
uint32_t getBits(uint32_t ip, int bitPosition, int k) { 
    int shift_amount = 32 - (bitPosition + k); 
    uint32_t mask = (k == 32) ? 0xFFFFFFFFu : ((1u << k) - 1u);
    return (ip >> shift_amount) & mask; 
}

int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength) { 
    if (prefixLength == 0) return 1; 
    int shift_amount = 32 - prefixLength; 
    return (ip >> shift_amount) == (prefix >> shift_amount); 
}
// LOOKUP ALGORITHM
int lookUp(struct node *root, uint32_t ip, int *memory_accesses) {
    int mem = 0;
    int current_main_idx = root ? root->temp_id : 0;
    if (current_main_idx < 0) current_main_idx = 0;

    int bit_pos = root ? root->prefixLength : 0;

    while (1) {
        mem++; // main_vector access
        struct main_vector_entry mv = main_vector[current_main_idx];

        if (mv.branch == 0) {
            mem++; // base_vector access
            struct base_vector_entry bv = base_vector[mv.pointer];

            if (checkPrefixMatch(ip, bv.ip_string, bit_pos) == 1) {
                *memory_accesses = mem;
                return bv.next_hop;
            }

            int pref_idx = bv.prefix_pointer;
            while (pref_idx != -1) {
                mem++; // prefix_vector access
                struct prefix_vector_entry pv = prefix_vector[pref_idx];
                if (checkPrefixMatch(ip, bv.ip_string, pv.length) == 1) {
                    *memory_accesses = mem;
                    return pv.next_hop;
                }
                pref_idx = pv.prefix_pointer;
            }

            *memory_accesses = mem;
            return 0;
        }

        // Internal: extract "branch" bits at (bit_pos + skip).
        bit_pos += mv.skip;
        uint32_t idx = getBits(ip, bit_pos, mv.branch);
        bit_pos += mv.branch;
        current_main_idx = mv.pointer + (int)idx;
    }

    /* vamos a empezar por el mainvector que tiene en la primera entrada la root.
    llevamos una cuenta de que bit estamos mirando.
    usando la branch y el skip obtenemos que bits hay que extraer (le sumamos el skip y cogemos desde ahi hasta ahi + branch)
    nos movemos a mainvector[pointer + el valor de estos bits extraidos]
    excepto que en este nodo branch fuera cero, entonces no hay comparacion, si no que nos movemoss 
    a basevector[pointer]  .
    nos vamos moviendo por el mainvector hasta que lleguemos a una leave que sera este caso en el que llegamos al base vector 


    una vez en el basevector vamos a coger la ip ahi guardada. sera ipref. 
    hacemos comprobacion hasta el numero de bits que haya que mirar (lo hemos guardado del calculo en mainvector)
 si hay match podemos reutrn ese next_hop 
si no hay match cogemos el prefix pointer. si es -1 no hay ruta devolvemos 0
si no nos vamos a la tabla de prefix_vector usando ese prefix poitner que tiene guardado la longitud de cada prefijo
comparamos el ip que buscamos con el ip ref extraidos los bits que nos diga esta longituf
cuando haya match daremos next hop, cuando no hay amatch cogemos el siguiente pointer ahi guardado
seguimos asi moviendonos por la prefix table hasta que o hagamos match o veamos un poitner -1 que 
que entonces devolvemos 0


    return 0; // Default route */
}

// =======================================================
// --- PHASE ? COMPRESSION & UTILS ---
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
    if (node->outInterface != 0 || node->bitPosition == 0) {
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

static int isLeafNode(struct node *node) {
    if (node == NULL) return 1;
    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children && node->children[i] != NULL) return 0;
    }
    return 1;
}

static int countLeaves(struct node *node) {
    if (node == NULL) return 0;
    if (isLeafNode(node)) return 1;
    int num_children = 1 << node->k;
    int count = 0;
    for (int i = 0; i < num_children; i++) {
        if (node->children[i] != NULL) count += countLeaves(node->children[i]);
    }
    return count;
}

static int countPrefixEntries(struct node *node) {
    if (node == NULL) return 0;
    if (isLeafNode(node)) return 0;

    int count = (node->outInterface != 0) ? 1 : 0;
    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children[i] != NULL) count += countPrefixEntries(node->children[i]);
    }
    return count;
}

static void buildprefixTableRec(struct node *node, int aux, int *prefix_idx) {
    if (node == NULL) return;

    if (isLeafNode(node)) {
        node->temp_prefix_index = aux;
        return;
    }

    if (node->outInterface != 0) {
        prefix_vector[*prefix_idx].length = node->prefixLength;
        prefix_vector[*prefix_idx].next_hop = node->outInterface;
        prefix_vector[*prefix_idx].prefix_pointer = aux;
        aux = *prefix_idx;
        (*prefix_idx)++;
    }

    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children[i] == NULL) {
            fprintf(stderr, "buildprefixTable: missing child at index %d (internal node)\n", i);
            exit(1);
        }
        buildprefixTableRec(node->children[i], aux, prefix_idx);
    }
}

void buildprefixTable(struct node *root) {
    prefix_vector_count = countPrefixEntries(root);
    prefix_vector = (struct prefix_vector_entry*)malloc((size_t)prefix_vector_count * sizeof(struct prefix_vector_entry));
    if (prefix_vector_count > 0 && prefix_vector == NULL) {
        fprintf(stderr, "buildprefixTable: out of memory\n");
        exit(1);
    }

    int prefix_idx = 0;
    buildprefixTableRec(root, -1, &prefix_idx);
}

void deleteSingleChildren(struct node **root_ptr) {
    if (root_ptr == NULL || *root_ptr == NULL) return;

    struct node *node = *root_ptr;

    // Post-order recursion.
    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children && node->children[i] != NULL) {
            deleteSingleChildren(&node->children[i]);
        }
    }

    // Count non-null children.
    int non_null = 0;
    struct node *only_child = NULL;
    for (int i = 0; i < num_children; i++) {
        if (node->children && node->children[i] != NULL) {
            non_null++;
            only_child = node->children[i];
        }
    }

    // Bypass single-child nodes that do not store a route.
    if (non_null == 1 && node->outInterface == 0 && node->bitPosition != 0) {
        free(node->children);
        *root_ptr = only_child;
        free(node);
    }
}

void buildMainBaseTables(struct node *root) {
    main_vector_count = countNodes(root);
    base_vector_count = countLeaves(root);

    main_vector = (struct main_vector_entry*)malloc((size_t)main_vector_count * sizeof(struct main_vector_entry));
    base_vector = (struct base_vector_entry*)malloc((size_t)base_vector_count * sizeof(struct base_vector_entry));
    if ((main_vector_count > 0 && main_vector == NULL) || (base_vector_count > 0 && base_vector == NULL)) {
        fprintf(stderr, "buildMainBaseTables: out of memory\n");
        exit(1);
    }

    struct node **queue = (struct node**)malloc((size_t)main_vector_count * sizeof(struct node*));
    if (queue == NULL) {
        fprintf(stderr, "buildMainBaseTables: out of memory (queue)\n");
        exit(1);
    }

    int head = 0, tail = 0;
    int next_id = 0;
    int base_idx = 0;

    root->temp_id = next_id++;
    queue[tail++] = root;

    while (head < tail) {
        struct node *node = queue[head++];
        int cur_id = node->temp_id;

        if (isLeafNode(node)) {
            node->temp_skip = 0;
            main_vector[cur_id].branch = 0;
            main_vector[cur_id].skip = 0;
            main_vector[cur_id].pointer = base_idx;

            base_vector[base_idx].ip_string = node->prefix;
            base_vector[base_idx].next_hop = node->outInterface;
            base_vector[base_idx].prefix_pointer = node->temp_prefix_index;
            base_idx++;
            continue;
        }

        // Internal: compute temp_skip from the minimum child prefixLength.
        int num_children = 1 << node->k;
        int min_child_pl = 33; // larger than any valid prefixLength (0..32)
        for (int i = 0; i < num_children; i++) {
            if (node->children[i] == NULL) {
                fprintf(stderr, "buildMainBaseTables: missing child at index %d (internal node)\n", i);
                exit(1);
            }
            if (node->children[i]->prefixLength < min_child_pl) {
                min_child_pl = node->children[i]->prefixLength;
            }
        }
        node->temp_skip = min_child_pl - node->prefixLength - node->k + 1;

        // Enqueue children in increasing index order: pointer + extractedBits works.
        int first_child_id = -1;
        for (int i = 0; i < num_children; i++) {
            struct node *child = node->children[i];
            child->temp_id = next_id;
            if (i == 0) first_child_id = next_id;
            next_id++;
            queue[tail++] = child;
        }

        main_vector[cur_id].branch = node->k;
        main_vector[cur_id].skip = node->temp_skip;
        main_vector[cur_id].pointer = first_child_id;
    }

    free(queue);
}

// Función auxiliar para imprimir el IP en formato A.B.C.D
void printIP(uint32_t ip) {
    printf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

// Función recursiva para imprimir la estructura del árbol
void printTrieStructure(struct node *node, int depth) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node(pl=%d, k=%d, out=%d)\n", node->prefixLength, node->k, node->outInterface);

    if (node->children == NULL) return;
    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children[i] != NULL) {
            printTrieStructure(node->children[i], depth + 1);
        }
    }
}