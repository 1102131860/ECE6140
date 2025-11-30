#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

// Global variables definition
size_t num_inputs = 0, num_outputs = 0, num_gates = 0, num_nets = 0;
size_t *inputs = NULL, *outputs = NULL;
gate_t *gates = NULL;
net_t *nets = NULL;
nnet_t *nnets = NULL;
gate_net_map_t *gnmap = NULL;
// Need to declaim but not used in podem (this only for sim2)
set_net_t *set_nets = NULL;
uint8_t *hash_net_set = NULL;
fault_net_t *result_union = NULL;
size_t size_union = 0;

// Local variables definition
const char *file_path = NULL, *fault = NULL;
size_t fault_net = 0;

int main(int argc, char** argv)
{
    for (size_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f")) file_path = argv[i+1];
        if (!strcmp(argv[i], "-s")) fault = argv[i+1];
        if (!strcmp(argv[i], "-n")) fault_net = atoi(argv[i+1]);
    }

    if (!file_path || !fault) {
        fprintf(stderr, "Usage: ./sim3 -f <file> -n <net> -s <sa0/sa1>\n");
        return -1;
    }

    // Load Circuit firstly
    if (build(file_path)) {
        fprintf(stderr, "Cannot open file (%s)\n", file_path);
        sim3_cleanup();
        return -1;
    }

    // Initialize sim3
    if (sim3_initialize(fault_net, fault)) {
        fprintf(stderr, "Simulation initialization failed\n");
        sim3_cleanup();
        return -1;
    }

    // Run sim3
    sim3_run();
    // Wrap up sim3
    sim3_wrap();
    // Clean up sim3
    sim3_cleanup();

    return 0;
}
