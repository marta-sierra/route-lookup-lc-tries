#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h> // Para los struct timespec del profesor

#include "io.h"
#include "trie.h"

int main(int argc, char *argv[]) {
    uint32_t prefix;
    int prefixLength;
    int outInterface;

    // Abrimos los archivos (ajusta los nombres si usas otros TXT)
    int attempt = initializeIO("routing_table_simple.txt", "prueba3.txt");
    if (attempt != OK) {
        printIOExplanationError(attempt);
        return 1;
    }

    struct node *root = createNode(0);
    root->prefix = 0;           
    root->prefixLength = 0;     
    root->outInterface = 0;     

    // Fase 1 y 2: Construir el árbol
    int routes_added = 0;
    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertPrefixSimpleTrie(root, prefix, prefixLength, outInterface);
        routes_added++;
    }

    // Fase 3: Búsqueda y Escritura de resultados
    int total_lookups = 0;
    int total_memory_accesses = 0;
    double total_packet_processing_time = 0.0;
    uint32_t ip;
    
    // Cambiamos al nombre correcto de la función del profesor: readInputPacketFileLine
    while (readInputPacketFileLine(&ip) == OK) {
        int accesses_this_lookup = 0;
        struct timespec initialTime, finalTime;
        double searchingTime;
        
        // El profesor pide medir el tiempo EXACTAMENTE alrededor del lookup
        clock_gettime(CLOCK_MONOTONIC_RAW, &initialTime);
        int bestInterface = lookUp(root, ip, &accesses_this_lookup);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finalTime);
        
        // Llamamos a la función de imprimir del profesor para que nos cree el archivo .out
        printOutputLine(ip, bestInterface, &initialTime, &finalTime, &searchingTime, accesses_this_lookup);
        
        // Actualizamos métricas para el resumen final
        total_lookups++;
        total_memory_accesses += accesses_this_lookup;
        total_packet_processing_time += searchingTime;
    }

    // Calcular las medias que pide el profesor
    double avg_memory_accesses = 0;
    double avg_processing_time = 0;
    if (total_lookups > 0) {
        avg_memory_accesses = (double)total_memory_accesses / total_lookups;
        avg_processing_time = total_packet_processing_time / total_lookups;
    }

    // Imprimir el resumen del profesor (ponemos 0 de momento en el número de nodos)
    printSummary(0, total_lookups, avg_memory_accesses, avg_processing_time);

    freeIO();
    return 0;
}
