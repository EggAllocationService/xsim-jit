#ifndef JIT_H_DEF
#define JIT_H_DEF
#include "xcpudefs.h"
#include "linkedlist.h"


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
    linked_list_t function_cache;
} jit_state;

typedef struct jit_prepared_function {
    jit_func function;
    unsigned short addr_start;
    unsigned short addr_end;
    int generated_size;
    int translated_instruction_count;
} jit_prepared_function;

/**
 * Recompiles a function located at `address` in `program`
 * 
 * `program` should already be loaded with the guest program
 *
 * Note that the prepared function stores static references to *program. It is unsafe to call the generated code if
 * `program` has been freed
*/
extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address);

/**
 * Initializes the internal state of the JIT compiler
 * Call this before trying to prepare any functions
*/
extern void jit_init_state();

/**
 * Get the internal state of the jit compiler, used for some runtime functions
 */
extern jit_state *jit_get_state();
/**
 * Sets the debug function to be called when the debug bit is set
*/
extern void jit_set_debug_function(jit_debug_func func);

#endif