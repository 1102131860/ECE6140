#include "sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ======================
// Globals for PODEM
// ======================
static size_t  fault_net_num  = 0;          // net number where fault is located
static size_t  fault_net_idx  = (size_t)-1; // index into nnets
static uint8_t fault_sa       = 0;          // 0 for sa0, 1 for sa1
static int     podem_success  = 0;

// ----------------------
// Helper: map five-value <-> (good, faulty)
// ----------------------
static void decode_five(five_val_t v, three_val_t *g, three_val_t *f) {
    switch (v) {
    case ZERO: *g = TV_0; *f = TV_0; break;
    case ONE:  *g = TV_1; *f = TV_1; break;
    case D:    *g = TV_1; *f = TV_0; break; // good=1, faulty=0
    case DBAR: *g = TV_0; *f = TV_1; break; // good=0, faulty=1
    case X:
    default:   *g = TV_X; *f = TV_X; break;
    }
}

static five_val_t encode_five(three_val_t g, three_val_t f) {
    if (g == TV_0 && f == TV_0) return ZERO;
    if (g == TV_1 && f == TV_1) return ONE;
    if (g == TV_1 && f == TV_0) return D;
    if (g == TV_0 && f == TV_1) return DBAR;
    // any combination with X or inconsistent -> X
    return X;
}

// three-valued gate operation on a,b in {0,1,X}
static three_val_t gate_op_scalar(gatetype_t type, three_val_t a, three_val_t b) {
    int ca = (a == TV_1);
    int cb = (b == TV_1);
    int za = (a == TV_0);
    int zb = (b == TV_0);

    switch (type) {
    case AND:
    case NAND:                                  // flip in the imply()
        if (za || zb) return TV_0;              // any 0 -> 0
        if (a == TV_1 && b == TV_1) return TV_1; 
        return TV_X;                            // otherwise unknown
    case OR:
    case NOR:                                   // flip in the imply()
        if (ca || cb) return TV_1;              // any 1 -> 1
        if (a == TV_0 && b == TV_0) return TV_0;
        return TV_X;
    default:
        return TV_X;
    }
}

static three_val_t buf_op(three_val_t a) {
    return a;
}

static three_val_t inv_op(three_val_t a) {
    if (a == TV_0) return TV_1;
    if (a == TV_1) return TV_0;
    return TV_X;
}

// ----------------------
// Circuit / PODEM helpers
// ----------------------
static int is_primary_input_idx(size_t idx) {
    // In sim3_initialize()，PIs stores in nnets[0 .. num_inputs-1]
    return (idx < num_inputs);
}

static size_t find_net_idx(size_t net) {
    for (size_t i = 0; i < num_nets; ++i)
        if (nnets[i].net == net) return i;
    return (size_t)-1;
}

// Inject stuck-at fault：only inject faulty value to fault net
static void inject_fault() {
    if (fault_net_idx == (size_t)-1) return;
    if (fault_sa == 0)
        nnets[fault_net_idx].nvalue = D;        // D' = good 0, faulty 1
    else
        nnets[fault_net_idx].nvalue = DBAR;     // D  = good 1, faulty 0
}

// Start from assigning PI，use D-calculation to imply
static void imply() {
    // Apply the fault at the beginning
    inject_fault();

    int changed = 1;
    while (changed) {
        changed = 0;

        for (size_t gi = 0; gi < num_gates; ++gi) {
            gate_t g = gates[gi];
            gate_net_map_t map = gnmap[gi];
            size_t n_in = map.num_nets - 1;
            size_t out_idx = map.nets_idxs[map.num_nets - 1];

            three_val_t g_in0 = TV_X, f_in0 = TV_X;
            three_val_t g_in1 = TV_X, f_in1 = TV_X;

            if (n_in >= 1)
                decode_five(nnets[map.nets_idxs[0]].nvalue, &g_in0, &f_in0);
            if (n_in == 2)
                decode_five(nnets[map.nets_idxs[1]].nvalue, &g_in1, &f_in1);

            three_val_t g_out = TV_X, f_out = TV_X;

            switch (g.type) {
            case BUF:
                g_out = buf_op(g_in0);
                f_out = buf_op(f_in0);
                break;
            case INV:
                g_out = inv_op(g_in0);
                f_out = inv_op(f_in0);
                break;
            case AND:
            case OR:
            case NAND:
            case NOR: {
                three_val_t core_g = gate_op_scalar(g.type, g_in0, g_in1);
                three_val_t core_f = gate_op_scalar(g.type, f_in0, f_in1);
                if (g.type == AND || g.type == OR) {
                    g_out = core_g;
                    f_out = core_f;
                } else { // NAND / NOR
                    g_out = inv_op(core_g);
                    f_out = inv_op(core_f);
                }
                break;
            }
            default:
                g_out = TV_X;
                f_out = TV_X;
                break;
            }

            // --- FORCE FAULT NODE ---
            five_val_t new_v;
            if (out_idx == fault_net_idx) {
                // GOOD value kept from imply
                // But FAULT value forced to stuck-at value
                if (fault_sa == 0)
                    f_out = TV_0;
                else
                    f_out = TV_1;
                new_v = encode_five(g_out, f_out);
            } else {
                // normal node
                new_v = encode_five(g_out, f_out);
            }

            if (new_v != nnets[out_idx].nvalue) {
                nnets[out_idx].nvalue = new_v;
                changed = 1;
#ifdef DEBUG
                printf("  Gate %ld type %d updates net %ld to value %d\n", 
                    gi, g.type, nnets[out_idx].net, new_v);
#endif
            }
        }
    }
#ifdef DEBUG
    printf("After imply():\n");
    for (size_t i = 0; i < num_nets; ++i) {
        five_val_t v = nnets[i].nvalue;
        printf("  net %ld: value %d\n", nnets[i].net, v);
    }
#endif
}

static int is_fault_activated() {
    if (fault_net_idx == (size_t)-1) return 0;
    five_val_t v = nnets[fault_net_idx].nvalue;
    if (fault_sa == 0)
        return (v == D);     // 1/0
    else
        return (v == DBAR);  // 0/1
}

static int is_fault_propagated() {
    // Any primary output is D/ D' means fault is propagated
    for (size_t i = 0; i < num_outputs; ++i) {
        size_t net_no = outputs[i];
        size_t idx = find_net_idx(net_no);
        if (idx == (size_t)-1) continue;
        five_val_t v = nnets[idx].nvalue;
        if (v == D || v == DBAR) return 1;
    }
    return 0;
}

// D-frontier gate: output is X, at least one input is D/ D'
static int find_d_frontier_gate(size_t *gate_idx) {
    for (size_t gi = 0; gi < num_gates; ++gi) {
        gate_net_map_t map = gnmap[gi];
        size_t out_idx = map.nets_idxs[map.num_nets - 1];
        five_val_t out_v = nnets[out_idx].nvalue;
        if (out_v != X) continue;

        int has_d = 0;
        for (size_t i = 0; i < map.num_nets - 1; ++i) {
            five_val_t in_v = nnets[map.nets_idxs[i]].nvalue;
            if (in_v == D || in_v == DBAR) {
                has_d = 1;
                break;
            }
        }
        if (has_d) {
            *gate_idx = gi;
            return 1;
        }
    }
#ifdef DEBUG
    printf("No D-frontier gate found.\n");
#endif
    return 0;
}

// generate objective: (t_net_idx, t_val)
static int get_objective(size_t *t_net_idx, five_val_t *t_val) {
    // If fault hasn't yet been activated: target = make fault net good = ~sa
    if (!is_fault_activated()) {
        if (fault_net_idx == (size_t)-1) return 0;
        *t_net_idx = fault_net_idx;
        *t_val = (fault_sa == 0) ? ONE : ZERO; // good value is oppsite to the stuck value
        return 1;
    }

    // if fault is activated but not yet propagated: pick a D-frontier gate
    size_t g_idx;
    if (!find_d_frontier_gate(&g_idx))
        return 0; // not found D-frontier gate

    gate_t g = gates[g_idx];
    gate_net_map_t map = gnmap[g_idx];

    // non-controlling value
    uint8_t nc_val = 0;
    switch (g.type) {
    case AND:
    case NAND:
        nc_val = 1; // controlling 0
        break;
    case OR:
    case NOR:
        nc_val = 0; // controlling 1
        break;
    case BUF:
    case INV:
    default:
        nc_val = 0;
        break;
    }

    // find first X input net
    for (size_t i = 0; i < map.num_nets - 1; ++i) {
        size_t idx = map.nets_idxs[i];
        if (nnets[idx].nvalue == X) {
            *t_net_idx = idx;
            *t_val = (nc_val == 1) ? ONE : ZERO;
            return 1;
        }
    }
#ifdef DEBUG
    printf("D-frontier gate found but no X input net?!\n");
#endif
    return 0;
}

// find the driver gate of a net
static int find_driver_gate(size_t net_idx, size_t *gate_idx) {
#ifdef DEBUG
    printf("Finding driver gate for net idx %ld (net %ld)\n", net_idx, nnets[net_idx].net);
#endif
    size_t net_no = nnets[net_idx].net;
    for (size_t gi = 0; gi < num_gates; ++gi) {
        gate_t g = gates[gi];
        size_t out_net = g.nets[g.num_nets - 1];
        if (out_net == net_no) {
            *gate_idx = gi;
#ifdef DEBUG
            printf("  -> driver gate = %ld, type = %d (out_net = %ld)\n", gi, g.type, out_net);
#endif
            return 1;
        }
    }
    return 0;
}

// backtrace: backtrace from (t_net_idx, t_val) to a PI
static void backtrace(size_t t_net_idx, five_val_t t_val, size_t *pi_idx, five_val_t *pi_val) {
    size_t     cur_idx = t_net_idx;
    five_val_t cur_val = t_val;

    while (!is_primary_input_idx(cur_idx)) {
        size_t g_idx;
        if (!find_driver_gate(cur_idx, &g_idx))
            // not a driver gate, treat as PI
            break;

        gate_t g = gates[g_idx];
        gate_net_map_t map = gnmap[g_idx];

        // Consider both inputs and outputs to decide backtrace path
        size_t next_idx;
        if (nnets[map.nets_idxs[0]].nvalue != X)
            // if first input is not X, try second input (for 2-input gates)
            if (map.num_nets < 3 || nnets[map.nets_idxs[1]].nvalue != X)
                // both inputs are not X, cannot backtrace, treat as PI
                break;
            else
                // backtrace through second input
                next_idx = map.nets_idxs[1];
        else
            next_idx = map.nets_idxs[0];
#ifdef DEBUG
        printf("  Backtracing through gate %ld (type %d), from net idx %ld to %ld\n",
            g_idx, g.type, cur_idx, next_idx);
#endif
        int invert = 0;
        switch (g.type) {
        case INV:
        case NAND:
        case NOR:
            invert = 1;
            break;
        default:
            invert = 0;
            break;
        }

        if (invert) {
            if (cur_val == ONE)       cur_val = ZERO;
            else if (cur_val == ZERO) cur_val = ONE;
        }

        cur_idx = next_idx;
    }
#ifdef DEBUG
    printf("Backtrace result: PI net idx %ld, value %d\n", cur_idx, cur_val);
#endif
    *pi_idx = cur_idx;
    *pi_val = cur_val;
}

static five_val_t flip_binary(five_val_t v) {
    if (v == ZERO) return ONE;
    if (v == ONE)  return ZERO;
    return v;
}

static int has_d_frontier() {
    size_t dummy;
    return find_d_frontier_gate(&dummy);
}

// ======================
// core recursive PODEM
// ======================
static int podem() {
    if (is_fault_propagated())
        return 1;

    // if fault is activated but cannot be propagated, fail directly
    if (is_fault_activated() && !has_d_frontier() /*&& !is_fault_propagated()*/)
        return 0;

    size_t     t_net_idx;
    five_val_t t_val;
    if (!get_objective(&t_net_idx, &t_val))
        return 0;
#ifdef DEBUG
    printf("Objective: net idx %ld (net %ld), value %d\n",
        t_net_idx, nnets[t_net_idx].net, t_val);
#endif
    size_t     pi_idx;
    five_val_t pi_val;
    backtrace(t_net_idx, t_val, &pi_idx, &pi_val);

    // if PI already has a conflicting value, prune
    five_val_t cur = nnets[pi_idx].nvalue;
    if (cur != X && cur != pi_val)
        return 0;

    // backup for backtrack
    nnet_t *backup = (nnet_t*)malloc(num_nets * sizeof(nnet_t));
    if (!backup) return 0;
    memcpy(backup, nnets, num_nets * sizeof(nnet_t));

    // Try pi_val first
    nnets[pi_idx].nvalue = pi_val;  // PI use ZERO or ONE
    imply();
    if (podem()) {
        free(backup);
        return 1;
    }
#ifdef DEBUG
    printf("Backtracking on net idx %ld (net %ld), trying value %d failed, try %d\n",
        pi_idx, nnets[pi_idx].net, pi_val, flip_binary(pi_val));
#endif
    // restore, try the opposite value
    memcpy(nnets, backup, num_nets * sizeof(nnet_t));
    five_val_t opp = flip_binary(pi_val);
    nnets[pi_idx].nvalue = opp;
    imply();
    if (podem()) {
        free(backup);
        return 1;
    }

    // restore after failure
    memcpy(nnets, backup, num_nets * sizeof(nnet_t));
    free(backup);
    return 0;
}

// ======================
// Public APIs
// ======================
// call build() first, then sim3_initialize()
int sim3_initialize(size_t fault_net, const char* fault) {
    num_nets = num_inputs + num_outputs;
    nnets = (nnet_t*)calloc(num_nets, sizeof(nnet_t));
    for (size_t i = 0; i < num_inputs; i++) nnets[i].net = inputs[i];
    for (size_t i = 0; i < num_outputs; i++) nnets[i + num_inputs].net = outputs[i];
    for (size_t i = 0; i < num_gates; i++) {
        gate_t g = gates[i];
        size_t g_num_nets = g.num_nets;
        for (size_t j = 0; j < g_num_nets; j++) {
            size_t cnet = g.nets[j];
            uint8_t match = 0;
            for (size_t k = 0; k < num_nets; k++)
                if (cnet == nnets[k].net) match = 1;
            if (!match) {
                nnets = (nnet_t*)realloc(nnets, (num_nets + 1) * sizeof(nnet_t));
                memset(&nnets[num_nets], 0, sizeof(nnet_t));
                nnets[num_nets++].net = cnet;
            }
        }
    }
    // set initial values to X
    for (size_t i = 0; i < num_nets; i++) nnets[i].nvalue = X;

    // initialize gnmap: gate -> net index
    gnmap = (gate_net_map_t*)calloc(num_gates, sizeof(gate_net_map_t));
    for (size_t i = 0; i < num_gates; i++) {
        gate_t g = gates[i];
        size_t g_num_nets = g.num_nets;
        size_t* gp = (size_t*)calloc(g_num_nets, sizeof(size_t));
        for (size_t j = 0; j < g_num_nets; j++) {
            size_t g_net = g.nets[j];
            for (size_t k = 0; k < num_nets; k++)
                if (g_net == nnets[k].net) gp[j] = k;
        }
        gnmap[i].gate_idx = i;
        gnmap[i].num_nets = g_num_nets;
        gnmap[i].nets_idxs = gp;
    }

    // record fault info
    fault_net_num = fault_net;
    fault_net_idx = (size_t)-1;
    for (size_t i = 0; i < num_nets; ++i) {
        if (nnets[i].net == fault_net) {
            fault_net_idx = i;
            break;
        }
    }
    if (fault_net_idx == (size_t)-1) {
        fprintf(stderr, "Fault net %zu not found in circuit nets\n", fault_net);
        return 1;
    }

    if (!strcmp(fault, "sa0"))
        fault_sa = 0;
    else if (!strcmp(fault, "sa1"))
        fault_sa = 1;
    else {
        fprintf(stderr, "Unknown fault type: %s\n", fault);
        return 1;
    }
#ifdef DEBUG
    printf("==========sim3_initialization=========\n");
    for (size_t i = 0; i < num_nets; i++)
        printf("nnets[%ld]: net: %ld, nvalue: %d\n", i, nnets[i].net, nnets[i].nvalue);

    for (size_t i = 0; i < num_gates; i++) {
        printf("gnmap[%ld]: gate_no: %ld, num_nets: %ld, ", i, gnmap[i].gate_idx, gnmap[i].num_nets);
        for (size_t j = 0; j < gnmap[i].num_nets; j++)
            printf("%ld ", gnmap[i].nets_idxs[j]);
        printf("\n");
    }
#endif
    return 0;
}

int sim3_run() {
    // start from all X, use PODEM to search
    podem_success = podem();
    return 0;
}

int sim3_wrap() {
    printf("\n===== PODEM Simulator Result =====\n");
    if (!podem_success) {
        printf("No test pattern found for fault net %zu stuck-at-%d\n", fault_net_num, fault_sa);
        return 0;
    }

    printf("Test pattern found for fault net %zu stuck-at-%d\n", fault_net_num, fault_sa);

    printf("Primary inputs: ");
    for (size_t i = 0; i < num_inputs; ++i) {
        five_val_t v = nnets[i].nvalue;
        char c = 'X';
        if (v == ONE || v == D)          c = '1';
        else if (v == ZERO || v == DBAR) c = '0';
        printf("%c", c);
    }
    printf("\n");

    printf("Primary outputs: ");
    for (size_t i = 0; i < num_outputs; ++i) {
        size_t net_no = outputs[i];
        size_t idx = find_net_idx(net_no);
        char c = '?';
        if (idx != (size_t)-1) {
            five_val_t v = nnets[idx].nvalue;
            switch (v) {
            case ZERO: c = '0'; break;
            case ONE:  c = '1'; break;
            case D:    c = 'D'; break;
            case DBAR: c = 'E'; break; // use 'E' to represent D'
            case X:
            default:   c = 'X'; break;
            }
        }
        printf("%c", c);
    }
    printf("\n");
    return 0;
}

int sim3_cleanup() {
    if (inputs) {free(inputs); inputs = NULL;}
    if (outputs) {free(outputs); outputs = NULL;}
    if (gates) {
        for (size_t i = 0; i < num_gates; i++) { free(gates[i].nets); gates[i].nets = NULL;}
        free(gates); gates = NULL;
    }
    if (gnmap) {
        for (size_t i = 0; i < num_gates; i++) { free(gnmap[i].nets_idxs); gnmap[i].nets_idxs = NULL;}
        free(gnmap); gnmap = NULL;
    }
    if (nnets) { free(nnets); nnets = NULL;}
    num_inputs = 0, num_outputs = 0, num_gates = 0, num_nets = 0;
    fault_net_idx = (size_t)-1, fault_net_num = 0, fault_sa = 0, podem_success = 0;
    return 0;
}
