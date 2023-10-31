#include "jit.h"
#include "x64_codegen.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "jit_register_map.h"

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

static int write_function_exit_preamble(unsigned char *memory, int gen_ptr) {
    int offset = x64_map_pop(memory, gen_ptr, 15);
    offset += x64_map_pop(memory, gen_ptr + offset, 14);
    offset += x64_map_pop(memory, gen_ptr + offset, 13);
    offset += x64_map_pop(memory, gen_ptr + offset, 12);
    offset += x64_map_pop(memory, gen_ptr + offset, RBP);
    offset += x64_map_pop(memory, gen_ptr + offset, RSP);
    offset += x64_map_pop(memory, gen_ptr + offset, RBX);
    return offset;
}

extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, int max_instructions, jit_state *state) {
    int remaining_instructions = max_instructions;

    // map some memory for the generated code
    unsigned char *memory = mmap(NULL, max_instructions * 15, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    int pc = 0;
    int gen_ptr = 0;
    int processed_instructions = 0;
    jit_register_map *register_map = new_map();

    /**
     * Preamble:
     * Save required x64 registers
     * Pointer to cpu state is stored in rbx
    */
    gen_ptr += x64_map_push(memory, gen_ptr, RBX);
    gen_ptr += x64_map_push(memory, gen_ptr, RSP);
    gen_ptr += x64_map_push(memory, gen_ptr, RBP);
    gen_ptr += x64_map_push(memory, gen_ptr, 12);
    gen_ptr += x64_map_push(memory, gen_ptr, 13);
    gen_ptr += x64_map_push(memory, gen_ptr, 14);
    gen_ptr += x64_map_push(memory, gen_ptr, 15);

    gen_ptr += x64_map_move_reg2reg(memory, gen_ptr, RDI, RBX);

    /*
        calculate 8 + 9
    */
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, 13, 0x8);
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, 14, 0x9);
    gen_ptr += x64_map_sub(memory, gen_ptr, 14, 13);
    gen_ptr += x64_map_move_reg2reg(memory, gen_ptr, 14, RAX);

    /**
     * Post generation:
     * Restore saved registers
    */
    gen_ptr += write_function_exit_preamble(memory, gen_ptr);

    gen_ptr += x64_map_ret(memory, gen_ptr);

    jit_prepared_function *result = malloc(sizeof(jit_prepared_function));
    result->function = (jit_func)memory;
    result->addr_start = address;
    result->addr_end = max_instructions;
    result->generated_size = gen_ptr;
    result->translated_instruction_count = max_instructions;

    return result;
}