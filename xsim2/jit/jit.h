#ifndef JIT_H_DEF
#define JIT_H_DEF
#include "xcpudefs.h"
#include "linkedlist.h"


/**
 *  Function that emulates running some X-CPU instructions on a given cpu state
 *  @returns either 1 or 0, indicating if the function panicked
 *
 *  If returning 1, the caller should then use an interpreter, as some code was encountered that couldn't be JIT
 *  compiled
*/
typedef unsigned char (*jit_func)(xcpu *state);

/**
 * Debug function called after every instruction if the debug bit is set
*/
typedef jit_func jit_debug_func;

typedef struct jit_state {
    unsigned long escape_hatch;
    unsigned long escape_registers[7];
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
 * Note that the prepared function stores inline references to *program. It is unsafe to call the generated code if
 * `program` has been freed
 *
 * `is_entrypoint` Should be set to 1 if the generated code will be used as the entrypoint to the virtual program
 * This parameter allows the jit compiler to set an escape hatch to use if the debug bit gets set
 *
 * `internal_abi` generates a function that does not preserve any registers, ideal for recursive functions
*/
extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, char is_entrypoint,
                                          char internal_abi);

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

/**
 * Utility function
*/
extern unsigned short load_short(unsigned char *mem, int offset);

#endif