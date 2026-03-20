#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h> 

#include "io.h"
#include "trie.h"

int main(int argc, char *argv[]) {
    uint32_t prefix;
    int prefixLength;
    int outInterface;

    // Command line arguments check
    if (argc != 3) {
        printf("Usage: %s <routing_table.txt> <test_packets.txt>\n", argv[0]);
        return 1;
    }

    int attempt = initializeIO(argv[1], argv[2]);
    if (attempt != OK) {
        printIOExplanationError(attempt);
        return 1;
    }

    struct node *root = createNode(0);
    root->prefix = 0;           
    root->prefixLength = 0;     
    root->outInterface = 0;     

    // Phase 1: Build the standard binary tree
    int routes_added = 0;
    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertPrefixSimpleTrie(root, prefix, prefixLength, outInterface);
        routes_added++;
    }

    // Phase 2: Patricia Compression
    int nodes_before = countNodes(root);
    patriciaCompress(&root);
    printf("\n>>> ESTRUCTURA DESPUÉS DE PATRICIA COMPRESS <<<\n");
    printTrieStructure(root, 0);
    printf("===============================================\n"); 

    // Phase 3: Level compress
    levelCompress(root);
    printf("\n>>> ESTRUCTURA DESPUÉS DE LEVELS COMPRESS <<<\n");
    printTrieStructure(root, 0);
    printf("===============================================\n");
    
    int nodes_after = countNodes(root);

    // Phase 4: Build tables
    // Phase 4.1.: Prefix Table
    prefix_vector = (struct prefix_vector_entry*)malloc(COUNT INTERNAL NODES LEVEL COMPRESSED TRIE  * sizeof(struct prefix_vector_entry));
    buildprefixTable(pass what needed);
    // Phase 4.2: Delete single children
    deleteSingleChildren(root)
    *hacer funcion en trie.c que al encontrarse un hijo unico reemplaza el padre con este
    
    // Phase 4.3: recorrer bread first 
    vamos a rellenar el campo temp_id del nodo empezando por 0 en root y en breadfirst cada vez 
    vas sumando uno. tambien cuando pases por cada nodo a oarte de lods ids tienes que calcular tambein 
    para todos los nodos sus temp_skip. si es un leave ponlo a 0, si es interno una formula que sera el minimo valor de prefixlegnth de sus hijos
    menos el valor de prefixlegnth de ese propio nodo - k de ese nodo mas 1
    tambien cunado pases por cada nodo si es un hijo ponle k=0


main_vector = (struct main_vector_entry*)malloc(COUNT TOTAL NODES LEVEL COMPRESSED TRIE  * sizeof(struct main_vector_entry));
    base_vector = (struct base_vector_entry*)malloc(COUNT TOTAL LEAVES OF LEVEL COMPRESSES TRIE * sizeof(struct base_vector_entry));
vamos a hacer otro breadfirst por el arbol llenando ambas de estas dos estrcutruas a la vez
tenemos un mainidx iniciado a cero, y vamos a rellenar la tabla main vector ordenadamente de uno en uno 
con este mainidx y simultaneamente el basevector que tendra otro baseidx iniciado a 0.

mainidx ira coincidiendo con temp_idx de los nodos pero no hace falta que checkees
entonces por cada nodo en el trie ponemos su valor de branch que es k del nodo, su valor skip que sera el 
temp skip que ya hemos calculado para ese nodo y el pointer hay dos casos: si tenemos hijos, 
le ponemos como pointer el temp_id del primer hijo si no tenemos hijso ponemos ahi el base idx

añadiremos en el basevector cuando no tengamos hijos una entrada en el baseidx con la ip de ese nodo, el nexthop que sera la outputinterface
y prefix pointer el temp prefix pointer del nodo



    //Phase 5: Lookups and Performance Metrics
    int total_lookups = 0;
    int total_memory_accesses = 0;
    double total_packet_processing_time = 0.0;
    uint32_t ip;
    
    while (readInputPacketFileLine(&ip) == OK) {
        int accesses_this_lookup = 0;
        struct timespec initialTime, finalTime;
        double searchingTime;
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &initialTime);
        int bestInterface = lookUp(root, ip, &accesses_this_lookup);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finalTime);
        
        printOutputLine(ip, bestInterface, &initialTime, &finalTime, &searchingTime, accesses_this_lookup);
        
        total_lookups++;
        total_memory_accesses += accesses_this_lookup;
        total_packet_processing_time += searchingTime;
    }

    double avg_memory_accesses = 0;
    double avg_processing_time = 0;
    if (total_lookups > 0) {
        avg_memory_accesses = (double)total_memory_accesses / total_lookups;
        avg_processing_time = total_packet_processing_time / total_lookups;
    }

    printSummary(nodes_after, total_lookups, avg_memory_accesses, avg_processing_time); */

    // Mandatory memory cleanup in C
    free(next_hop_table);
    free(prefix_vector);
    free(base_vector);
    freeIO();
    
    return 0;
}
