#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

// Global variables definition
size_t num_inputs = 0, num_outputs = 0, num_gates = 0, num_nets = 0;
size_t *inputs = NULL, *outputs = NULL;
gate_t *gates = NULL;
net_t *nets = NULL;
set_net_t *set_nets = NULL;
gate_net_map_t *gnmap = NULL;
uint8_t *hash_net_set = NULL;
fault_net_t *result_union = NULL;
size_t size_union = 0;

// Local variables definition
const char *file_path, *signals;
const char *mode_str, *rand_case_str;
int mode = 0, rand_case = 5;

int main(int argc, char** argv)
{
    for (size_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f")) file_path = argv[i+1];
        if (!strcmp(argv[i], "-i")) signals = argv[i+1];
        if (!strcmp(argv[i], "-m")) {
            mode_str = argv[i+1];
            int m = atoi(mode_str);
            if (m == 0 || m == 1)
                mode = m;
            else
                fprintf(stderr, "Please enter a integer 0 or 1 (-m)\n");
        }
        if (!strcmp(argv[i], "-n")) {
            rand_case_str = argv[i+1];
            int rc = atoi(rand_case_str);
            if (rc > 0)
                rand_case = rc;
            else
                fprintf(stderr, "Please enter a positive integer (-n)\n"); 
        }
    }

    // Load Circuit firstly
    if (build(file_path)) {
        fprintf(stderr, "Cannot open file (%s)\n", file_path);
        cleanup_final();
        return -1;
    }

    // Given the input signal, then generate the corresponding output signal
    if (mode == 0) {
        if (sim1_initialize(signals, 0)) {
            fprintf(stderr, "Input siganls (%s) dismatches the circuit\n", signals);
            cleanup_final();
            return -1;
        }
        sim1_run();
        sim2_initialize();
        sim2_run();
        sim2_postrun();
        sim2_wrap();
        cleanup_final();
    } 

    // Self randomly generate input signal
    if (mode == 1) {
        // set rand seed only once
        srand(RAND_SEED);
        for (size_t i = 0; i < rand_case; i++) {
            printf("\n========== Random Test Case #%ld ==========\n", i + 1);
            sim1_initialize(NULL, 1);
            sim1_run();
            sim2_initialize();
            sim2_run();
            sim2_postrun();
            // don't forget to cleanup after each iteration
            cleanup();
        }

        // show the final result
        sim2_wrap();
        cleanup_final();
    }
    
    return 0;
}
