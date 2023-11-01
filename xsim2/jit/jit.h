#ifndef JIT_H_DEF
#define JIT_H_DEF
#include "xcpudefs.h"



/**
 *  Translated X rou tine
*/
typedef void (*jit_func)(xcpu *state);

/**
 * Debug function called after every instruction if the debug bit is set
*/
typedef jit_func jit_debug_func;

typedef struct jit_state {
    unsigned char guest_debug_bit;
    jit_debug_func debug_function;
} jit_state;

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
extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address);

/**
 * Initializes the internal state of the JIT compiler
 * Call this before trying to prepare any functions
*/
extern void jit_init_state();

/**
 * Sets the debug function to be called when the debug bit is set
*/
extern void jit_set_debug_function(jit_debug_func func);

#endif