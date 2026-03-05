#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// Definición de la estructura del nodo
struct node {
    uint32_t prefix; 
    int prefixLength;
    int bitPosition; 
    int outInterface;

    int k; 
    struct node **children; 
};

// Declaraciones de las funciones de la Fase 1
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);

// Declaraciones de las funciones de la Fase 3
uint32_t getBits(uint32_t ip, int bitPosition, int k);
int checkPrefixMatch(uint32_t ip, uint32_t prefix, int prefixLength);
int lookUp(struct node *root, uint32_t ip, int *memory_accesses);

#endif // TRIE_H
// comitet 