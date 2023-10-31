#include "jit.h"
#include "x64_codegen.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#define EAX 0
#define RSI 6
#define RDI 7





extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, int max_instructions, jit_state *state) {
    int remaining_instructions = max_instructions;

    // map some memory for the generated code
    unsigned char *memory = mmap(NULL, max_instructions * 15, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    int instr_pointer = 0;
    instr_pointer += x64_map_move_reg2reg(memory, instr_pointer, RDI, 9);
    instr_pointer += x64_map_move_reg2reg(memory, instr_pointer, RSI, EAX);
    instr_pointer += x64_map_imul(memory, instr_pointer, EAX, 9);
    instr_pointer += x64_map_move_reg2reg(memory, instr_pointer, 9, EAX);
    instr_pointer += x64_map_ret(memory, instr_pointer);


    jit_prepared_function *result = malloc(sizeof(jit_prepared_function));
    result->function = (jit_func)memory;
    result->addr_start = address;
    result->addr_end = max_instructions;
    result->generated_size = instr_pointer;
    result->translated_instruction_count = max_instructions;

    return result;
}