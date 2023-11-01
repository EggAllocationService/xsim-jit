#include "jit.h"

extern void calc_flags_test(unsigned short src, unsigned short dest, xcpu *c) {
    if ((c->regs[src] & c->regs[dest]) != 0) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

extern void calc_flags_cmp(unsigned short src, unsigned short dest, xcpu *c) {
    if (c->regs[src] < c->regs[dest]) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

extern void calc_flags_equ(unsigned short src, unsigned short dest, xcpu *c) {
    if (c->regs[src] == c->regs[dest]) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

extern unsigned short load_short(unsigned char *memptr, unsigned short addr) {
    unsigned short result = 0;
    result |= memptr[addr] << 8;
    result |= memptr[addr + 1];
    return result;
}