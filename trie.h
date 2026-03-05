#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>

// Definición de la estructura del nodo
struct node {
    // Forwarding Info
    uint32_t prefix; 
    int prefixLength;
    int bitPosition; // Siguiente bit a evaluar (índice 0 a 31)
    int outInterface; // Usaremos -1 para indicar que es NULL / no tiene interfaz

    // Children
    int k; // Ramificación: 2^k hijos
    struct node **children;  // Array dinámico de punteros a hijos
};

// Declaraciones de las funciones de la Fase 1
struct node* createNode(int bitPos);
void insertPrefixSimpleTrie(struct node *root, uint32_t ip, int prefixLength, int outInterface);

#endif // TRIE_H
