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

    // Phase 1 and 2: Build the standard binary tree
    int routes_added = 0;
    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertPrefixSimpleTrie(root, prefix, prefixLength, outInterface);
        routes_added++;
    }

    // --- NEW: Extract information and build the external tables BEFORE compressing ---
    buildTables(root, -1);
    
    // Phase 4 and 5: Compression (Patricia + LC-Trie)
    int nodes_before = countNodes(root);
    patriciaCompress(&root);

    printf("\n>>> ESTRUCTURA DESPUÉS DE PATRICIA COMPRESS <<<\n");
    printTrieStructure(root, 0);
    printf("===============================================\n"); 

    levelCompress(root);
    printf("\n>>> ESTRUCTURA DESPUÉS DE LEVELS COMPRESS <<<\n");
    printTrieStructure(root, 0);
    printf("===============================================\n");
    
    int nodes_after = countNodes(root);

    // PAPER TABLE BUILDING
    // 1. Allocate dynamic memory for the 3 Paper Vectors (SIZES INCREASED TO PREVENT SEGFAULT)
    prefix_vector = (struct prefix_vector_entry*)malloc(COUNT INTERNAL NODES LEVEL COMPRESSED TRIE  * sizeof(struct prefix_vector_entry));
    buildprefixTable(pass what needed)
    
    
    
    
    base_vector = (struct base_vector_entry*)malloc(500000 * sizeof(struct base_vector_entry));

    // Phase 3: Lookups and Performance Metrics
    /* int total_lookups = 0;
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
