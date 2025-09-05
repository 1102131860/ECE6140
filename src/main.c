#include <stdio.h>
#include <string.h>
#include "sim1.h"

#define NUM_PARAMETERS 4

const char* file_path;
const char* signals;
char* results;

int main(int argc, char** argv)
{
    if (argc != NUM_PARAMETERS + 1) {
        fprintf(stderr, "Please check the command format: %s -f <file_path> -i <input_signals>\n", argv[0]);
        return -1;
    }

    for (size_t i = 1; i < NUM_PARAMETERS + 1; i++) {
        if (!strcmp(argv[i], "-f")) file_path = argv[i+1];
        if (!strcmp(argv[i], "-i")) signals = argv[i+1];
    }

    if (build(file_path)) {
        fprintf(stderr, "Cannot open file (%s)\n", file_path);
        cleanup(&results);
        return -1;
    }

    if (initialize(signals)) {
        fprintf(stderr, "Input siganls (%s) dismatches the circuit\n", signals);
        cleanup(&results);
        return -1;
    }

    if (run(&results)) {
        fprintf(stderr, "Fail to find reseut\n");
        cleanup(&results);
        return -1;
    }

    printf("%s\n", results);
    cleanup(&results);
    
    return 0;
}
