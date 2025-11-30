#include "sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int sim2_initialize() {
    set_nets = (set_net_t*)calloc(num_nets, sizeof(set_net_t));
    for (size_t i = 0; i < num_nets; i++) {
        size_t n = nets[i].net;
        uint8_t v = nets[i].value;
        // initialize set_nets[i]
        set_nets[i].net = n;
        set_nets[i].value = v;
        set_nets[i].is_updated = (i < num_inputs);
        set_nets[i].num_f_net_set = 1;
        // initialize f_net_set
        set_nets[i].f_net_set = (fault_net_t*)calloc(1, sizeof(fault_net_t));
        fault_net_t* p = &set_nets[i].f_net_set[0];
        p->fault_net = n;
        p->sa0 = (v != 0);
        p->sa1 = (v == 0);
    }

    gnmap = (gate_net_map_t*)calloc(num_gates, sizeof(gate_net_map_t));
    for (size_t i = 0; i < num_gates; i++) {
        gate_t g = gates[i];
        size_t g_no = i;
        size_t g_num_nets = g.num_nets;

        size_t* gp = (size_t*)calloc(g_num_nets, sizeof(size_t));
        for (size_t j = 0; j < g_num_nets; j++) {
            size_t g_net = g.nets[j];
            for (size_t k = 0; k < num_nets; k++)
                if (g_net == set_nets[k].net)
                    gp[j] = k;
        }
        gnmap[i].gate_idx = g_no;
        gnmap[i].num_nets = g_num_nets;
        gnmap[i].nets_idxs = gp;
    }

    // net num starts from 1 to 2 * (size + 1)
    // [1:num_nets] is stuck-at-0 [num_nets+1:2*num_nets] is stuck-at-1
    hash_net_set = (uint8_t*)calloc(2*(num_nets + 1), sizeof(uint8_t));

#ifdef DEBUG
    printf("==========sim2_initialization=========\n");
    for (size_t i = 0; i < num_nets; i++) {
        printf("set_nets[%ld]: net: %ld, value: %d, is_updated: %d, num_f_net_set: %ld\n", 
            i, set_nets[i].net, set_nets[i].value, set_nets[i].is_updated, set_nets[i].num_f_net_set);
        for (size_t j = 0; j < set_nets[i].num_f_net_set; j++) {
            printf("\tf_net_set[%ld]: fault_net = %ld, sa0 = %d, sa1 = %d\n",
            j, set_nets[i].f_net_set[j].fault_net, set_nets[i].f_net_set[j].sa0, set_nets[i].f_net_set[j].sa1);
        }
    }

    for (size_t i = 0; i < num_gates; i++) {
        printf("gnmap[%ld]: gate_no: %ld, num_nets: %ld, ", i, gnmap[i].gate_idx, gnmap[i].num_nets);
        for (size_t j = 0; j < gnmap[i].num_nets; j++)
            printf("%ld ", gnmap[i].nets_idxs[j]);
        printf("\n");
    }
#endif
    return 0;
}

// in set1 or in set2
int union_set(size_t set1_size, fault_net_t* set1, size_t set2_size, fault_net_t* set2, size_t* set_size, fault_net_t** set) {
    // hash set
    memset(hash_net_set, 0, (2*(num_nets + 1)) * sizeof(uint8_t));
    for (size_t i = 0; i < set1_size; i++) {
        size_t hash_idx = set1[i].fault_net + set1[i].sa1 * num_nets;
        hash_net_set[hash_idx] = 1;
    } 

    size_t new_set_size = set1_size;
    fault_net_t* new_set = (fault_net_t*)calloc(set1_size, sizeof(fault_net_t));
    memcpy(new_set, set1, set1_size * sizeof(fault_net_t));

    for (size_t i = 0; i < set2_size; i++) {
        fault_net_t f_net = set2[i];
        size_t hash_idx = f_net.fault_net + f_net.sa1 * num_nets;
        if (!hash_net_set[hash_idx]) {
            new_set = realloc(new_set, (new_set_size + 1)*sizeof(fault_net_t));
            new_set[new_set_size++] = f_net;
        }
    }

    *set_size = new_set_size;
    *set = new_set;
    return 0;
}

// both in se1 and set2
int intersection_set(size_t set1_size, fault_net_t* set1, size_t set2_size, fault_net_t* set2, size_t* set_size, fault_net_t** set) {
    // hash set
    memset(hash_net_set, 0, (2*(num_nets + 1)) * sizeof(uint8_t));
    for (size_t i = 0; i < set1_size; i++) {
        size_t hash_idx = set1[i].fault_net + set1[i].sa1 * num_nets; 
        hash_net_set[hash_idx] = 1;
    }

    size_t new_set_size = 0;
    fault_net_t* new_set = NULL;
    for (size_t i = 0; i < set2_size; i++) {
        fault_net_t f_net = set2[i];
        size_t hash_idx = f_net.fault_net + f_net.sa1 * num_nets;
        if (hash_net_set[hash_idx]) {
            new_set = (fault_net_t*)realloc(new_set, (new_set_size + 1) * sizeof(fault_net_t));
            new_set[new_set_size++] = f_net;
        }
    }

    *set_size = new_set_size;
    *set = new_set;
    return 0;
}

// in set1 but not in set2
int exclusion_set(size_t set1_size, fault_net_t* set1, size_t set2_size, fault_net_t* set2, size_t* set_size, fault_net_t** set) {
    // hash set
    memset(hash_net_set, 0, (2*(num_nets + 1)) * sizeof(uint8_t));
    for (size_t i = 0; i < set2_size; i++) {
        size_t hash_idx = set2[i].fault_net + set2[i].sa1 * num_nets; 
        hash_net_set[hash_idx] = 1;
    }

    size_t new_set_size = 0;
    fault_net_t* new_set = NULL;
    for (size_t i = 0; i < set1_size; i++) {
        fault_net_t f_net = set1[i];
        size_t hash_idx = f_net.fault_net + f_net.sa1 * num_nets;
        if (!hash_net_set[hash_idx]) {
            new_set = (fault_net_t*)realloc(new_set, (new_set_size + 1) * sizeof(fault_net_t));
            new_set[new_set_size++] = f_net;
        }
    }

    *set_size = new_set_size;
    *set = new_set;
    return 0;
}

// One input net, and one output net
// gnmap[gate_idx].num_nets must be 2
int merge_set_buf_inv(size_t gate_idx) {
    size_t in_idx = gnmap[gate_idx].nets_idxs[0], out_idx = gnmap[gate_idx].nets_idxs[1];
    set_net_t *in_p = &set_nets[in_idx], *out_p = &set_nets[out_idx];
    size_t num_new_set = 0;
    fault_net_t* new_set = NULL;

    // union input_set with output_set
    union_set(in_p->num_f_net_set, in_p->f_net_set, out_p->num_f_net_set, out_p->f_net_set, &num_new_set, &new_set);

    // release the oldput_net_idx's fault_net_t* f_net_set firstly
    free(out_p->f_net_set);
    out_p->f_net_set = new_set;
    out_p->num_f_net_set = num_new_set;
    // set this net has been updated
    out_p->is_updated = 1;
    return 0;
}

// two input nets, and one output net
// gnmap[gate_idx].num_nets must be 3
int merge_set_and_or_nand_nor(size_t gate_idx) {
    gatetype_t gtype = gates[gate_idx].type;
    size_t in1_idx = gnmap[gate_idx].nets_idxs[0], in2_idx = gnmap[gate_idx].nets_idxs[1], out_idx = gnmap[gate_idx].nets_idxs[2];
    set_net_t *in1_p = &set_nets[in1_idx], *in2_p = &set_nets[in2_idx], *out_p = &set_nets[out_idx];
    uint8_t in1_value = in1_p->value, in2_value = in2_p->value;
    size_t num_temp_set = 0, temp_new_set = 0;
    fault_net_t* temp_set = NULL, *new_set = NULL;

    if (gtype == AND || gtype == NAND) {
        // a = 1, b = 0 -> Lnew = Lb - La (in b but not in a)
        if (in1_value && !in2_value)
            exclusion_set(in2_p->num_f_net_set, in2_p->f_net_set, in1_p->num_f_net_set, in1_p->f_net_set, &num_temp_set, &temp_set);
        // a = 0, b = 1 -> Lnew = La - Lb (in a but not in b)
        else if (!in1_value && in2_value)
            exclusion_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set);
        // a = 1, b = 1 -> Lnew = La union Lb (in a or in b)
        else if (in1_value && in2_value)
            union_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set);
        // a = 0, b = 0 -> Lnew = La intersect Lb (both in a and b)
        else
            intersection_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set);
    }
    else if (gtype == OR || gtype == NOR) {
        // a = 1, b = 0 -> Lnew = La - Lb (in a but not in b)
        if (in1_value && !in2_value)
            exclusion_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set);
        // a = 0, b = 1 -> Lnew = Lb - La (in b but not in a)
        else if (!in1_value && in2_value)
            exclusion_set(in2_p->num_f_net_set, in2_p->f_net_set, in1_p->num_f_net_set, in1_p->f_net_set, &num_temp_set, &temp_set);
        // a = 1, b = 1 -> Lnew = La intersect Lb (both in a and in b)
        else if (in1_value && in2_value)
            intersection_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set);
        // a = 0, b = 0 -> Lnew = La union Lb (in a or b)
        else
            union_set(in1_p->num_f_net_set, in1_p->f_net_set, in2_p->num_f_net_set, in2_p->f_net_set, &num_temp_set, &temp_set); 
    }

    // union with the output fault set
    union_set(num_temp_set, temp_set, out_p->num_f_net_set, out_p->f_net_set, &temp_new_set, &new_set);
    // don't forget to free temp_f_net_set
    free(temp_set); temp_set = NULL;

    // release the oldput_net_idx's fault_net_t* f_net_set firstly
    free(out_p->f_net_set);    
    out_p->f_net_set = new_set;
    out_p->num_f_net_set = temp_new_set;
    // set this net has been updated
    out_p->is_updated = 1;
    return 0;
}

int deductive_simulate(size_t gate_idx) {
    gate_t g = gates[gate_idx];
    size_t g_num_sets = g.num_nets;
    gate_net_map_t gnmap_g = gnmap[gate_idx];

    // if the net has been updated, then skip this gate/output nets
    size_t output_net_idx = gnmap_g.nets_idxs[g_num_sets - 1];
    if (set_nets[output_net_idx].is_updated) return 1;

    // count how many input nets of gate are updated
    size_t count = 0;
    for (size_t i = 0; i < g_num_sets - 1; i++) {
        size_t net_idx = gnmap_g.nets_idxs[i];
        if (set_nets[net_idx].is_updated) count++;
    }
    // if some input nets haven't updated, then skip
    if (count != g_num_sets - 1) return 1;

    switch (g.type) {
        case BUF:
        case INV:
            if (!merge_set_buf_inv(gate_idx)) return 0;
            break;
        case AND:
        case OR:
        case NAND:
        case NOR:
            if (!merge_set_and_or_nand_nor(gate_idx)) return 0;
            break;
        default: 
            return 1;
    }
    return 1;
}

int sim2_run() {
    size_t total_num_updated = num_nets - num_inputs;
    size_t num_current_updated = 0; 
    while (num_current_updated < total_num_updated)
        for (size_t i = 0; i < num_gates; i++)
            if (!deductive_simulate(i))
                num_current_updated++;

#ifdef DEBUG
    printf("***************Display Output Net Fault lists*****************\n");
    for (size_t i = num_inputs; i < num_inputs + num_outputs; i++) {
        set_net_t output_net = set_nets[i];
        size_t num_fault_net = output_net.num_f_net_set;
        printf("output_net %ld:\n", output_net.net);
        for (size_t j = 0; j < num_fault_net; j++) {
            fault_net_t fault_net = output_net.f_net_set[j];
            printf("\tfault net %ld ", fault_net.fault_net);
            if (fault_net.sa0) printf("stuck at 0\n");
            if (fault_net.sa1) printf("stuck at 1\n");
        }
    }
#endif
    return 0;
}

// merge to result_union
int sim2_postrun() {
    fault_net_t *temp_set = NULL;
    size_t temp_size = 0;

    // collect output faults after the sim2_run()
    for (size_t i = num_inputs; i < num_inputs + num_outputs; i++) {
        size_t size_output_set = set_nets[i].num_f_net_set;
        fault_net_t *output_set = set_nets[i].f_net_set;

        if (!temp_set) {
            temp_set = (fault_net_t*)calloc(size_output_set, sizeof(fault_net_t));
            memcpy(temp_set, output_set, size_output_set * sizeof(fault_net_t));
            temp_size = size_output_set;
        } else {
            size_t new_size = 0;
            fault_net_t *new_set = NULL;
            union_set(temp_size, temp_set, size_output_set, output_set, &new_size, &new_set);
            free(temp_set);
            temp_set = new_set;
            temp_size = new_size;
        }
    }

    // merge to result union
    if (result_union == NULL) {
        result_union = temp_set;
        size_union = temp_size;
    } else {
        size_t new_size = 0;
        fault_net_t *new_union = NULL;
        union_set(size_union, result_union, temp_size, temp_set, &new_size, &new_union);
        free(result_union);
        result_union = new_union;
        size_union = new_size;
        free(temp_set);
    }

    return 0;
}

int sim2_wrap() {
    // Insertion sorting 
    for (size_t i = 1; i < size_union; ++i) {
        fault_net_t key = result_union[i];
        size_t j = i;
        while (j > 0 && result_union[j - 1].fault_net > key.fault_net) {
            result_union[j] = result_union[j - 1];
            j--;
        }
        result_union[j] = key;
    }

    printf("\n===== Deductive Fault Simulator Final Summary =====\n");
    printf("%ld total fault nets found\n", size_union);
    for (size_t i = 0; i < size_union; i++) {
        if (result_union[i].sa0)
            printf("net %ld stuck at 0\n", result_union[i].fault_net);
        if (result_union[i].sa1)
            printf("net %ld stuck at 1\n", result_union[i].fault_net);
    }

    return 0;
}

// only updated nets, gnmap, set_nets, and hash_net_set after iteration
int cleanup() {
    if (nets) {free(nets); nets = NULL;}
    if (hash_net_set) {free(hash_net_set); hash_net_set = NULL;}
    if (gnmap) {
        for (size_t i = 0; i < num_gates; i++) { free(gnmap[i].nets_idxs); gnmap[i].nets_idxs = NULL;}
        free(gnmap); gnmap = NULL;
    }
    if (set_nets) {
        for (size_t i = 0; i < num_nets; i++) { free(set_nets[i].f_net_set); set_nets[i].f_net_set = NULL;}
        free(set_nets); set_nets = NULL;
    }
    return 0;
}

// clear all
int cleanup_final() {
    cleanup();
    if (gates) {
        for (size_t i = 0; i < num_gates; i++) { free(gates[i].nets); gates[i].nets = NULL;}
        free(gates); gates = NULL;
    }
    if (inputs) {free(inputs); inputs = NULL;}
    if (outputs) {free(outputs); outputs = NULL;}
    if (result_union) {free(result_union); result_union = NULL;}
    num_inputs = 0, num_outputs = 0, num_gates = 0, num_nets = 0; size_union = 0;
    return 0;
}
