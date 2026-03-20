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
    newNode->prefix = 
    newNode->prefixLength = ;
    newNode->outInterface = -1; 
    newNode->k = 1;             
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
    current->prefix = ip;
    current->prefixLength = prefixLength;
}


// =======================================================
// --- PHASE ? LOOKUP FUNCTIONS ---
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
// LOOKUP ALGORITHM
int lookUp(struct node *root, uint32_t ip, int *memory_accesses) {
    struct node *current = root;
    *memory_accesses = 0;
    vamos a empezar por el mainvector que tiene en la primera entrada la root.
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



    return 0; // Default route
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

prefixidx int variable global = 0
buildPrefixTable(){
    recursive function. initially called from root with aux -1
    if leaf:
        node.temp_prefix_index = aux
    else:
        if outinterface != 0
            prefix_entry_vector[prefixidx] = { length -> prefixlength del nodo, 
            next_hop -> output interface dle nodo, prefix_pointer -> aux};
            aux <- prefixidx
            prefixidx += 1
        for all children:
            buildprefixtable(childnode, aux)


}

// Función auxiliar para imprimir el IP en formato A.B.C.D
void printIP(uint32_t ip) {
    printf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

// Función recursiva para imprimir la estructura del árbol
void printTrieStructure(struct node *node, int depth) {
    ...

    // Recorrer hijos según el valor de k (stride)
    int num_children = 1 << node->k;
    for (int i = 0; i < num_children; i++) {
        if (node->children[i] != NULL) {
            // Si el hijo es el mismo que el anterior (en Patricia no pasa, pero en LC-Trie sí)
            // solo lo imprimimos una vez para no saturar la consola
            if (i > 0 && node->children[i] == node->children[i-1]) continue;
            
            printTrieStructure(node->children[i], depth + 1);
        }
    }
}