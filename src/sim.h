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
} gate_net_map_t;           // for sim3 as well

// For sim3
typedef enum {
    ZERO,                   // bit 0
    ONE,                    // bit 1 
    D,                      // D     
    DBAR,                   // D' 
    X                       // X    
} five_val_t;

typedef enum {
    TV_0,
    TV_1,
    TV_X
} three_val_t;

typedef struct {
    size_t net;
    five_val_t nvalue;
} nnet_t;

// For sim1, sim2, sim3
extern size_t num_inputs, num_outputs, num_gates, num_nets;
extern size_t *inputs, *outputs;
extern gate_t *gates;
extern net_t  *nets;

// For sim2
extern size_t size_union;           // size of result set
extern set_net_t *set_nets;         // num_nets
extern uint8_t *hash_net_set;       // hashed net set
extern fault_net_t *result_union;   // result set

// For sim2 and sim3
extern gate_net_map_t *gnmap;       // gates map to nets idx
// For sim3
extern nnet_t *nnets;               // new net lists

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

// For sim3
int sim3_initialize(size_t fault_net, const char* fault);
int sim3_run();
int sim3_wrap();
int sim3_cleanup();

#endif
