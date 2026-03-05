#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h> // Used for struct timespec to measure execution time

#include "io.h"
#include "trie.h"

int main(int argc, char *argv[]) {
    uint32_t prefix;
    int prefixLength;
    int outInterface;

    if (argc != 3) {
        printf("Uso: %s <routing_table.txt> <paquetes_prueba.txt>\n", argv[0]);
        return 1;
    }
    // Open the input and output files (adjust the text file names if necessary)
    int attempt = initializeIO(argv[1], argv[2]);
    if (attempt != OK) {
        printIOExplanationError(attempt);
        return 1;
    }

    // Initialize the Root node
    struct node *root = createNode(0);
    root->prefix = 0;           
    root->prefixLength = 0;     
    root->outInterface = 0;     

    // Phase 1 and 2: Build the standard binary trie
    int routes_added = 0;
    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertPrefixSimpleTrie(root, prefix, prefixLength, outInterface);
        routes_added++;
    }

    // Phase 4: Patricia Compression
    int nodes_before = countNodes(root);
    
    // We pass the MEMORY ADDRESS of root (double pointer) to the compression algorithm
    patriciaCompress(&root); 
    
    int nodes_after = countNodes(root);

    // Phase 3: Lookup and Metrics
    int total_lookups = 0;
    int total_memory_accesses = 0;
    double total_packet_processing_time = 0.0;
    uint32_t ip;
    
    // Read the IPs we want to search for
    while (readInputPacketFileLine(&ip) == OK) {
        int accesses_this_lookup = 0;
        struct timespec initialTime, finalTime;
        double searchingTime;
        
        // Measure time strictly around the lookup function
        clock_gettime(CLOCK_MONOTONIC_RAW, &initialTime);
        int bestInterface = lookUp(root, ip, &accesses_this_lookup);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finalTime);
        
        // Call the professor's print function to generate the .out file line by line
        printOutputLine(ip, bestInterface, &initialTime, &finalTime, &searchingTime, accesses_this_lookup);
        
        // Update global metrics for the final summary
        total_lookups++;
        total_memory_accesses += accesses_this_lookup;
        total_packet_processing_time += searchingTime;
    }

    // Calculate the averages requested by the professor
    double avg_memory_accesses = 0;
    double avg_processing_time = 0;
    if (total_lookups > 0) {
        avg_memory_accesses = (double)total_memory_accesses / total_lookups;
        avg_processing_time = total_packet_processing_time / total_lookups;
    }

    // Print the final summary using the ACTUAL number of nodes after compression
    printSummary(nodes_after, total_lookups, avg_memory_accesses, avg_processing_time);

    // Clean up IO streams
    freeIO();
    
    // Note: To prevent memory leaks, a freeTrie(root) function should eventually be added here
    return 0;
}