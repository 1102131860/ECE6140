#ifndef _INC
#define _INC

#include <stddef.h>
#include <stdint.h> 

#define RAND_SEED 42
#define BUFFER_SIZE 1024

typedef enum {
    BUF,
    INV,
    AND,
    OR,
    NAND,
    NOR
} gatetype_t;

// For sim1
typedef struct {
    gatetype_t type;
    size_t num_nets;
    size_t* nets;
} gate_t;

typedef struct {
    size_t net;
    uint8_t value;
    uint8_t has_value;
} net_t;

// For sim2
typedef struct {
    size_t fault_net;
    uint8_t sa0;
    uint8_t sa1;
} fault_net_t;

typedef struct {
    size_t net;
    uint8_t value;
    uint8_t is_updated;
    size_t num_f_net_set;
    fault_net_t* f_net_set;
} set_net_t;

typedef struct {
    size_t gate_idx;        // the idx of gate_t* gates 
    size_t num_nets;        // how many nets are connected to this gate
    size_t* nets_idxs;      // the idx of set_net_t* set_nets;
} gate_net_map_t;

// For sim1
extern size_t num_inputs, num_outputs, num_gates, num_nets;
extern size_t *inputs, *outputs;
extern gate_t *gates;
extern net_t *nets;
// For sim2
extern gate_net_map_t *gnmap;       // num_gates
extern set_net_t *set_nets;         // num_nets
extern uint8_t* hash_net_set;       // hashed net set
extern fault_net_t* result_union;   // result set
extern size_t size_union;           // size of result set

// For sim1
int build(const char* path);
int sim1_initialize(const char* signal, uint8_t en_rand);
int sim1_run();
int sim1_wrap();
// For sim2
int sim2_initialize();
int sim2_run();
int sim2_postrun();
int sim2_wrap();
int cleanup();
int cleanup_final();

#endif
