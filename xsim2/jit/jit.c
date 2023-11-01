#include "jit.h"
#include "x64_codegen.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "xis.h"
#include "jit_runtime.h"


static jit_state *state;

extern void jit_init_state() {
    state = malloc(sizeof(jit_state));
    state->guest_debug_bit = 0;
}
extern void jit_set_debug_function(jit_debug_func func) {
    state->debug_function = func;
}

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

// register used for the address of function calls
#define FUNC_ADDR_REG 10
 
// registers that don't do anything funky with indirect addressing
#define SCRATCH_REG 14
#define SCRATCH_REG_2 11
#define CPU_STATE_REG 15
#define VMEM_BASE_REG 13

#define PC_INDEX 16
#define FLAGS_INDEX 17

/**
 * Shortcut for LOAD_VREG_TO_REG(src, SCRATCH_REG)
*/
#define LOAD_VREG_TO_SCRATCH(src) LOAD_VREG_TO_REG(src, SCRATCH_REG)
/**
 * Loads a virtual register to a physical one
*/
#define LOAD_VREG_TO_REG(src, dest) gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr, \
                                dest, CPU_STATE_REG, (2 * src));

#define STORE_REG_TO_VREG(src, dest) gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr, \
                                src, CPU_STATE_REG, (2 * dest));

/**
 * Moves the function address into `FUNC_ADDR_REG` then calls that register
*/
#define WRITE_FUNCTION_CALL(func) gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, \
                                FUNC_ADDR_REG, (unsigned long) (func)); \
                        gen_ptr += x64_map_call(memory, gen_ptr, \
                                FUNC_ADDR_REG);


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

static unsigned short load_short(unsigned char *mem, size_t offset) {
    unsigned short result = 0;
    result |= mem[offset] << 8;
    result |= mem[offset + 1];
    return result;
}


extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address) {

    // map some memory for the generated code
    unsigned char *memory = mmap(NULL, 16000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    
    unsigned short pc = address;
    int gen_ptr = 0;
    int processed_instructions = 0;

    /**
     * Preamble:
     * Save required x64 registers
    */
    gen_ptr += x64_map_push(memory, gen_ptr, RBX);
    gen_ptr += x64_map_push(memory, gen_ptr, RSP);
    gen_ptr += x64_map_push(memory, gen_ptr, RBP);
    gen_ptr += x64_map_push(memory, gen_ptr, 12);
    gen_ptr += x64_map_push(memory, gen_ptr, 13);
    gen_ptr += x64_map_push(memory, gen_ptr, 14);
    gen_ptr += x64_map_push(memory, gen_ptr, 15);

    // move the address of the cpu struct into r15
    gen_ptr += x64_map_move_reg2reg(memory, gen_ptr, RDI, CPU_STATE_REG);

    // hardcode the address of the virtual memory in r11
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, VMEM_BASE_REG, (unsigned long) program);

    // at the start of the procedure, update the cpu struct's program counter
    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr, pc, CPU_STATE_REG, PC_INDEX * 2);

    /**
     * Fetch, decode, and translate instructions
     * Exit if we find a `ret` or invalid opcode
    */
    while (1) {
        char invalid_op = 0;

        unsigned short instruction = load_short(program, pc);
        pc += 2;
        unsigned char opcode = instruction >> 8;
        if (XIS_NUM_OPS(opcode) == 3) { // immediate (extended)
            unsigned short extended_value = load_short(program, pc);
            pc += 2;
            switch (opcode) {
                case I_LOADI: {
                    unsigned char target_reg = XIS_REG1(instruction);
                    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                            extended_value, CPU_STATE_REG, (2 * target_reg));
                    break;
                }
                default:
                    invalid_op = 1;
                    break;

            }
        } else if (XIS_NUM_OPS(opcode) == 2) { // two register operands
                char src = XIS_REG1(instruction);
                char dest = XIS_REG2(instruction);
                switch(opcode) {
                    case I_ADD: {
                        LOAD_VREG_TO_SCRATCH(src);
                        gen_ptr += x64_map_add_indirect(memory, gen_ptr,
                                SCRATCH_REG, CPU_STATE_REG, (2 * dest)); 
                        break;
                    }
                    case I_SUB: {
                        LOAD_VREG_TO_SCRATCH(src);
                        gen_ptr += x64_map_sub_indirect(memory, gen_ptr,
                                SCRATCH_REG, CPU_STATE_REG, (2 * dest));
                        break;
                    }
                    case I_MUL: {
                        // vdest -> ax
                        LOAD_VREG_TO_REG(dest, RAX);
                        // clear DX
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                RDX, 0);
                        // mul vsrc -> ax
                        gen_ptr += x64_map_mul_indirect(memory, gen_ptr,
                                CPU_STATE_REG, (2 * src));
                        // mov ax -> vdest
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                RAX, CPU_STATE_REG, (2 * dest));
                        break;
                    }
                    case I_DIV: {
                        // vdest -> ax
                        LOAD_VREG_TO_REG(dest, RAX);
                        // clear DX 
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                RDX, 0);
                        // div vsrc -> ax
                        gen_ptr += x64_map_div_indirect(memory, gen_ptr,
                                CPU_STATE_REG, (2 * src));
                        // mov ax -> vdest
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                RAX, CPU_STATE_REG, (2 * dest));
                        break;
                    }
                    case I_AND: {
                        LOAD_VREG_TO_SCRATCH(src);
                        gen_ptr += x64_map_and_indirect(memory, gen_ptr,
                                SCRATCH_REG, CPU_STATE_REG, (2 * dest)); 
                        break;
                    }
                    case I_OR: {
                        LOAD_VREG_TO_SCRATCH(src);
                        gen_ptr += x64_map_or_indirect(memory, gen_ptr,
                                SCRATCH_REG, CPU_STATE_REG, (2 * dest)); 
                        break;
                    }
                    case I_XOR: {
                        LOAD_VREG_TO_SCRATCH(src);
                        gen_ptr += x64_map_xor_indirect(memory, gen_ptr,
                                SCRATCH_REG, CPU_STATE_REG, (2 * dest)); 
                        break;
                    }
                    case I_SHR: {
                        // vsrc -> cx
                        LOAD_VREG_TO_REG(src, RCX);
                        gen_ptr += x64_map_shr_indirect(memory, gen_ptr, 
                                CPU_STATE_REG, (2 * dest));
                        break;
                    }
                    case I_SHL: {
                        // vsrc -> cx
                        LOAD_VREG_TO_REG(src, RCX);
                        gen_ptr += x64_map_shl_indirect(memory, gen_ptr, 
                                CPU_STATE_REG, (2 * dest));
                        break;
                    }
                    case I_CMP: {
                        // work delegated to a runtime function because i don't care enough
                        
                        // load arguments
                        LOAD_VREG_TO_REG(src, RDI);
                        LOAD_VREG_TO_REG(dest, RSI);

                        // load cpu state pointer
                        gen_ptr += x64_map_move_reg2reg(memory, gen_ptr, 
                                CPU_STATE_REG, RDX);

                        WRITE_FUNCTION_CALL(calc_flags_cmp);
                        break;
                    }
                    case I_EQU: {
                        // load arguments
                        LOAD_VREG_TO_REG(src, RDI);
                        LOAD_VREG_TO_REG(dest, RSI);

                        // load cpu state pointer
                        gen_ptr += x64_map_move_reg2reg(memory, gen_ptr, 
                                CPU_STATE_REG, RDX);

                        WRITE_FUNCTION_CALL(calc_flags_equ);
                        break;
                    }
                    case I_MOV: {
                        LOAD_VREG_TO_SCRATCH(src);
                        STORE_REG_TO_VREG(SCRATCH_REG, dest);
                        break;
                    }
                    case I_LOAD: {
                        LOAD_VREG_TO_SCRATCH(src);
                        // compute effective address with src + mem base
                        gen_ptr += x64_map_add_reg2reg(memory, gen_ptr, 
                                SCRATCH_REG, VMEM_BASE_REG);
                        // load 16 bit word from virtual memory
                        gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr,
                                SCRATCH_REG_2, SCRATCH_REG, 0);
                        // the loaded word is in big-endian format, but x64 is little endian
                        // therefore, we must swap the bytes around using the BSWAP instruction
                        gen_ptr += x64_map_bswap_r16(memory, gen_ptr,
                                SCRATCH_REG_2);
                        // write the swapped integer to the appropriate register
                        STORE_REG_TO_VREG(SCRATCH_REG_2, dest);
                        break;
                    }

                    default: {
                        // invalid opcode
                        invalid_op = 1;
                        break;
                    }
                }
        } 

        processed_instructions++;
        if (invalid_op) break;
    }

    
    /**
     * Post generation: 
     * Update program counter to procedure end position
    */
    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
        pc, CPU_STATE_REG, (2 * PC_INDEX));

    /**
     * Post generation:
     * Restore saved registers
    */
    gen_ptr += write_function_exit_preamble(memory, gen_ptr);

    gen_ptr += x64_map_ret(memory, gen_ptr);

    jit_prepared_function *result = malloc(sizeof(jit_prepared_function));
    result->function = (jit_func)memory;
    result->addr_start = address;
    result->addr_end = pc;
    result->generated_size = gen_ptr;
    result->translated_instruction_count = processed_instructions;

    return result;
}