#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Include the provided I/O headers and our new trie header
#include "io.h"
#include "trie.h"

int main(int argc, char *argv[]) {
    // 1. Initialize variables to hold the routing table data
    uint32_t prefix;
    int prefixLength;
    int outInterface;

    // 2. Open the files. 
    // We will use the simple routing table and a simple lookup test file for now.
    int attempt = initializeIO("routing_table_simple.txt", "prueba0.txt");
    if (attempt != OK) {
        printIOExplanationError(attempt);
        return 1; // Exit program with error
    }

    // 3. Initialize the root of the Trie
    // bitPosition starts at 0 (leftmost bit)
    struct node *root = createNode(0);
    root->prefix = 0;           // 0.0.0.0
    root->prefixLength = 0;     // /0
    root->outInterface = 0;     // Default route interface (adjust if needed)

    printf("Building initial binary trie...\n");

    // 4. Read the routing table line by line and build the tree
    int routes_added = 0;
    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertPrefixSimpleTrie(root, prefix, prefixLength, outInterface);
        routes_added++;
    }

    printf("Successfully added %d routes to the trie.\n", routes_added);

    // --- FUTURE PHASES WILL GO HERE ---
    // patriciaCompress(root);
    // levelCompress(root);
    // lookup loop
    // ----------------------------------

    // 5. Cleanup
    // freeTrie(root); // We will implement this function later!
    freeIO();
    
    printf("Phase 2 complete! Exiting safely.\n");

    return 0;
}
