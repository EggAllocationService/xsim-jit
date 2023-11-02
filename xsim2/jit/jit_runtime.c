#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "jit.h"
#include "jit_runtime.h"
#include "x64_codegen.h"

extern void calc_flags_test(unsigned short src, unsigned short dest, xcpu *c) {
    if ((src & dest) != 0) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

extern void calc_flags_cmp(unsigned short src, unsigned short dest, xcpu *c) {
    if (src < dest) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

extern void calc_flags_equ(unsigned short src, unsigned short dest, xcpu *c) {
    if (src == dest) {
        c->state |= 1;
    } else {
        c->state &= ~1;
    }
}

// header of a generated function, we want to skip this if we're doing a jump
#define PREAMBLE_LENGTH 20

extern unsigned long compile_or_get_cached_addr(unsigned char *memory, unsigned short address) {
    jit_state *state = jit_get_state();
    ll_iterator_t *iter = ll_iterator_new(state->function_cache);
    jit_prepared_function *target = NULL;
    while (ll_has_next(iter)) {
        jit_prepared_function *potential = ll_next(iter);
        if (potential->addr_start == address) {
            target = potential;
            break;
        }
    }
    if (target == NULL) {
        // target function is not prepared
        target = jit_prepare(memory, address, 0);
        ll_add_front(state->function_cache, target);
#ifndef NDEBUG
        char * filename = malloc(80);
        sprintf(filename, "jit_func_%x_%lx.bin", address, (unsigned long) target->function);
        FILE *fp = fopen(filename, "wb");
        fwrite(target->function, target->generated_size, 1, fp);
        fclose(fp);
        free(filename);
#endif
       // printf("Wrote %d bytes to %s\n\n", target->generated_size, filename);

    }

    ll_iterator_destroy(iter);
    return (unsigned long) target->function;
}

extern unsigned long call_dynamic(unsigned char *memory, unsigned short address) {
    //printf("JIT: Dynamic call to %x\n", address);
    return compile_or_get_cached_addr(memory, address);
}
extern unsigned long jump_dynamic(unsigned char *memory, unsigned short address) {
    //printf("JIT: Dynamic jump to %x\n", address);
    return compile_or_get_cached_addr(memory, address) + PREAMBLE_LENGTH;
}


#define RAX 0

extern unsigned long call_static(unsigned char *memory, unsigned short address, unsigned long call_start) {
    unsigned long call_target = compile_or_get_cached_addr(memory, address);
    unsigned char *target = (unsigned char *) (call_start);
    for (int i = 0; i < CALL_STATIC_OVERWRITE_SIZE; i++) {
        target[i] = 0x90; // NOOP
    }
    x64_map_move_imm2reg(target, 0, RAX, call_target);
    return call_target;
}