#include "sim.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// helper functions
int str2gatetype(const char* t, gatetype_t* g) {
    if      (!strcmp(t,"BUF"))  {*g = BUF; return 0;}
    else if (!strcmp(t,"INV"))  {*g = INV; return 0;}
    else if (!strcmp(t,"AND"))  {*g = AND; return 0;}
    else if (!strcmp(t,"OR"))   {*g = OR; return 0;}
    else if (!strcmp(t,"NAND")) {*g = NAND; return 0;}
    else if (!strcmp(t,"NOR"))  {*g = NOR; return 0;}
    return 1;
}

int gateComputesNet(gate_t g) {
    // if output net of gate has been computed and had a result, skip
    for (size_t i = 0; i < num_nets; i++)
        if (g.nets[g.num_nets-1] == nets[i].net && nets[i].has_value)
            return 1;

    #ifdef DEBUG
        const char* str;
        switch (g.type) {
        case BUF:  str = "BUF";  break;
        case INV:  str = "INV";  break;
        case AND:  str = "AND";  break;
        case OR:   str = "OR";   break;
        case NAND: str = "NAND"; break;
        case NOR:  str = "NOR";  break;
        default: break;
        }
        printf("gate %s, %ld\n", str, g.nets[0]);
    #endif
    // count how many input nets of gate are known
    uint8_t xy[2];
    uint8_t count = 0;
    for (size_t i = 0; i < g.num_nets - 1; i++) {
        size_t i_net = g.nets[i];
        for (size_t j = 0; j < num_nets; j++) {
            net_t j_net = nets[j];
            if (i_net == j_net.net && j_net.has_value) {
                xy[i] = j_net.value;
                count++;
            }
        }  
    }
    // if some input nets are unknown, skip
    if (count != g.num_nets - 1) return 1;

    // calculate the output value corresponding to the gate type
    size_t net = g.nets[g.num_nets - 1];
    uint8_t val;
    switch (g.type) {
    case BUF:  val = xy[0];            break;
    case INV:  val = ~xy[0];           break;
    case AND:  val = xy[0] & xy[1];    break;
    case OR:   val = xy[0] | xy[1];    break;
    case NAND: val = ~(xy[0] & xy[1]); break;
    case NOR:  val = ~(xy[0] | xy[1]); break;
    default:   return 1;
    }
    // mark the output net
    for (size_t i = 0; i < num_nets; i++)
        if (nets[i].net == net) {
            nets[i].value = val & 0x01;
            nets[i].has_value = 1;
        }
    #ifdef DEBUG
        printf("    net %ld has value %d\n", net, val & 0x01);
    #endif
    return 0;
}

int build(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) return 1;

    char line_buffer[BUFFER_SIZE];
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        char* tok = strtok(line_buffer, " \t\r\n");
        if (!tok) continue;
        #ifdef DEBUG
            printf("%s ", tok);
        #endif
        if (!strcmp(tok, "INPUT")) {
            char* p = strtok(NULL, " \t\r\n");
            while (p) {
                int v = atoi(p);
                if (v == -1) break;
                inputs = (size_t*)realloc(inputs, (num_inputs + 1) * sizeof(size_t));
                inputs[num_inputs++] = v;
                p = strtok(NULL, " \t\r\n");
                #ifdef DEBUG
                    printf("%ld ", inputs[num_inputs-1]);
                #endif
            }
        } else if (!strcmp(tok, "OUTPUT")) {
            char* p = strtok(NULL, " \t\r\n");
            while (p) {
                int v = atoi(p);
                if (v == -1) break;
                outputs = (size_t*)realloc(outputs, (num_outputs + 1) * sizeof(size_t));
                outputs[num_outputs++] = v;
                p = strtok(NULL, " \t\r\n");
                #ifdef DEBUG
                    printf("%ld ", outputs[num_outputs-1]);
                #endif
            }
        } else {
            gatetype_t gatetype;
            if(str2gatetype(tok, &gatetype)) return 1;
            gates = (gate_t*)realloc(gates, (num_gates + 1) * sizeof(gate_t));
            gate_t* gp = &gates[num_gates++];
            // realloc() request the input pointer must be a NULL or the returned pointer of malloc/realloc
            // otherwise gp could be a dirty pointer, which will cause core dumped
            memset(gp, 0, sizeof(gate_t));
            gp->type = gatetype;

            char* p = strtok(NULL, " \t\r\n");            
            while (p) {
                gp->nets = (size_t*)realloc(gp->nets, (gp->num_nets + 1) * sizeof(size_t));
                gp->nets[gp->num_nets++] = atoi(p);
                p = strtok(NULL, " \t\r\n");
                #ifdef DEBUG
                    printf("%ld ", gp->nets[(gp->num_nets)-1]);
                #endif
            }
        }
        #ifdef DEBUG
            printf("\n");
        #endif
    }
    fclose(fp);
    return 0;
}


int sim1_initialize(const char* signal, uint8_t en_rand) {
    num_nets = num_inputs + num_outputs;
    nets = (net_t*)calloc(num_nets, sizeof(net_t));
    for (size_t i = 0; i < num_inputs; i++) nets[i].net = inputs[i];
    for (size_t i = 0; i < num_outputs; i++) nets[i + num_inputs].net = outputs[i];
    for (size_t i = 0; i < num_gates; i++) {
        for (size_t j = 0; j < gates[i].num_nets; j++) {
            size_t cnet = gates[i].nets[j];
            int match = 0;
            for (size_t k = 0; k < num_nets; k++)
                if (cnet == nets[k].net) match = 1;
            if (!match) {
                nets = (net_t*)realloc(nets, (num_nets + 1) * sizeof(net_t));
                // clear up those allocated memory (calloc don't need clear)
                memset(&nets[num_nets], 0, sizeof(net_t));
                nets[num_nets++].net = cnet;
            }
        }
    }

    if (en_rand) {
        for (size_t i = 0; i < num_inputs; i++) {
            uint8_t val = rand() & 1;
            nets[i].value = val;
            nets[i].has_value = 1;
        }
    } else {
        size_t input_len = strlen(signal);
        if (input_len != num_inputs) return 1;
        for (size_t i = 0; i < input_len; i++) {
            char ch = signal[i];
            if (ch != '0' && ch != '1') return 1;
            nets[i].value = ch - '0';
            nets[i].has_value = 1;
        }
    }

    #ifdef DEBUG
        printf("=======================================\n");
        for (size_t i = 0; i < num_nets; i++)
            printf("net: %ld, value: %d, has_value: %d\n", nets[i].net, nets[i].value, nets[i].has_value);
    #endif
    return 0;
}


int sim1_run() {
    size_t computed_gates = 0;
    while (computed_gates < num_gates)
        for (size_t i = 0; i < num_gates; i++)
            if (!gateComputesNet(gates[i])) computed_gates++;
    #ifdef DEBUG
        printf("****************************************\n");
        for (size_t i = 0; i < num_nets; i++)
            printf("net: %ld, value: %d, has_value: %d\n", nets[i].net, nets[i].value, nets[i].has_value);
    #endif

    return 0;
}

int sim1_wrap() {
    printf("\n=====Combinational Logic Simulator Wrap up=====\n");
    for (size_t i = 0; i < num_outputs; i++) {
        char cc = '0' + nets[i + num_inputs].value;
        printf("%c", cc);
    }
    printf("\n");
    
    return 0;
}
