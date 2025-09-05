#ifndef _INC
#define _INC

#include <stddef.h>
#include <stdint.h> 

typedef enum {
    BUF,
    INV,
    AND,
    OR,
    NAND,
    NOR
} gatetype_t;

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

int build(const char* path);
int initialize(const char* signal);
int run(char** res);
int cleanup(char** res);

#endif
