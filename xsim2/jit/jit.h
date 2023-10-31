#ifndef JIT_H_DEF
#define JIT_H_DEF
#include "xcpudefs.h"

typedef struct jit_state {
    unsigned char guest_debug_bit;
} jit_state;

/**
 *  Translated portion of the guest machine code.
 *  On return, `cpu_state->pc` will be the address to be executed next
 *  
 *  If this function returns a 0, then the program should halt
 *  Else, the program should prepare and execute the next address
*/
typedef unsigned int (*jit_func)(short a, short b);

typedef struct jit_prepared_function {
    jit_func function;
    unsigned short addr_start;
    unsigned short addr_end;
    int generated_size;
    int translated_instruction_count;
} jit_prepared_function;

/**
 * Translates up to `max_instructions` of X, starting at `address`.
 * 
 * `program` should already be loaded with 
*/
extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, int max_instructions, jit_state *state);

#endif