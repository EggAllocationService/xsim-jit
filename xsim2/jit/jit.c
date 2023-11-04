#include "jit.h"
#include "x64_codegen.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "xis.h"
#include "jit_runtime.h"
#include "linker.h"
#include "registers.h"

static jit_state *state;

extern void jit_init_state() {
    state = malloc(sizeof(jit_state));
    state->guest_debug_bit = 0;
    state->function_cache = ll_new();
}
extern void jit_set_debug_function(jit_debug_func func) {
    state->debug_function = func;
}
extern jit_state *jit_get_state() {
    return state;
}




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

#define BEGIN_LINK_REQUEST(t) link_req_t *req = malloc(sizeof(link_req_t)); \
                        req->from = ((unsigned long) memory) + gen_ptr; \
                        req->type = t;

#define SUBMIT_LINK_REQUEST(target) req->target_pc = target; \
                                req->gen_code_length = (((unsigned long) memory) + gen_ptr) - (req->from); \
                                ll_add_front(link_requests, req);


static unsigned short switch_endianness(unsigned short i) {
    return (i << 8) | (i >> 8);
}

static int write_function_exit_preamble(unsigned char *memory, int gen_ptr) {
    int offset = x64_map_pop(memory, gen_ptr, 15);
    offset += x64_map_pop(memory, gen_ptr + offset, 14);
    offset += x64_map_pop(memory, gen_ptr + offset, 13);
    offset += x64_map_pop(memory, gen_ptr + offset, 12);
    offset += x64_map_pop(memory, gen_ptr + offset, RBX);

    // pop stack frame
    return offset;
}

static unsigned short load_short(unsigned char *mem, size_t offset) {
    unsigned short result = 0;
    result |= mem[offset] << 8;
    result |= mem[offset + 1];
    return result;
}


extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, unsigned char is_entrypoint) {

    // map some memory for the generated code
    unsigned char *memory = mmap(NULL, 16000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    
    unsigned short pc = address;
    int gen_ptr = 0;
    int processed_instructions = 0;
    char emitted_ret = 0;


    /*
     * Setup the escape hatch
     */
    if (is_entrypoint) {

        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                        RAX, (unsigned long) state);
        // read the return address into RSI
        gen_ptr += x64_map_pop(memory, gen_ptr, RSI);
        gen_ptr += x64_map_push(memory, gen_ptr, RSI);

        // store return address in the jit state struct
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              RSI, RAX, 0);

        // store callee-save pointers in escape info
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                               RBX, RAX, 8);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              RSP, RAX, 16);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              RBP, RAX, 24);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              12, RAX, 32);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              13, RAX, 40);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              14, RAX, 48);
        gen_ptr += x64_map_mov_reg2indirect64(memory, gen_ptr,
                                              15, RAX, 56);

    }

    /**
     * Preamble:
     * Save required x64 registers
    */
    gen_ptr += x64_map_push(memory, gen_ptr, RBX);
    gen_ptr += x64_map_push(memory, gen_ptr, 12);
    gen_ptr += x64_map_push(memory, gen_ptr, 13);
    gen_ptr += x64_map_push(memory, gen_ptr, 14);
    gen_ptr += x64_map_push(memory, gen_ptr, 15);

    // move the address of the cpu struct into r15
    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, RDI, CPU_STATE_REG);


     if (gen_ptr < 20) {
         while (gen_ptr < 20) {
             memory[gen_ptr++] = 0x90; // NOP
         }
     }
    /**
       * BEGIN JUMP TARGET
       *
       * An out-of-procedure jump that needs to skip the preamble will land 24 instructions after the start of
       * a generated function
       */
    // hardcode the address of the virtual memory in r11
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, VMEM_BASE_REG, (unsigned long) program);

    // at the start of the procedure, update the cpu struct's program counter
    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr, pc, CPU_STATE_REG, PC_INDEX * 2);

    /*
     * Structure to map x86 addresses -> start of virtual instructions
     * This is used for the linking stage
     */
    instr_results_t *instr_mapping = new_instruction_container(address);

    linked_list_t link_requests = ll_new();

    /**
     * Fetch, decode, and translate instructions
     * Exit if we find a `ret` or invalid opcode
    */
    char invalid_op = 0;
    while (1) {

        unsigned short instruction = load_short(program, pc);
        pc += 2;
        unsigned char opcode = instruction >> 8;
        istr_container_push(instr_mapping, ((unsigned long)memory) + gen_ptr);
        if (XIS_NUM_OPS(opcode) == 3) { // immediate (extended)
            unsigned short extended_value = load_short(program, pc);
            pc += 2;

            // needed because an extended instruction is the length of two instructions
            istr_container_push(instr_mapping, ((unsigned long)memory) + gen_ptr);

            switch (opcode) {
                case I_LOADI: {
                    unsigned char target_reg = XIS_REG1(instruction);
                    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                            extended_value, CPU_STATE_REG, (2 * target_reg));
                    break;
                }
                case I_JMP: {
                    // uses jump dynamic because i'm boring
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                   RSI, extended_value);
                    WRITE_FUNCTION_CALL(jump_dynamic)
                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);
                    break;
                }
                case I_CALL: {
                    // call_static will overwrite this region, so repeated calls are faster
                    // there are 36 bytes in this block
                    int start = gen_ptr;
                    unsigned long call_start = ((unsigned long)memory) + gen_ptr;
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RSI, extended_value);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                   RDX, call_start);
                    WRITE_FUNCTION_CALL(call_static);
                    // end overwritten region

                    // make sure the constant is still valid
                    int overwritten_len = gen_ptr - start;
                    if (overwritten_len != CALL_STATIC_OVERWRITE_SIZE) {
                        printf("Assertion failed: check CALL_STATIC_OVERWRITE_SIZE! (%d)", overwritten_len);
                        exit(-4);
                    }

                    // sub 2 from virtual stack pointer ( r15
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    SCRATCH_REG, 2);
                    gen_ptr += x64_map_sub_indirect(memory, gen_ptr,
                                                    SCRATCH_REG, CPU_STATE_REG, (2 * 15));
                    // store pc + 2 on stack
                    LOAD_VREG_TO_SCRATCH(15);
                    gen_ptr += x64_map_add_reg2reg(memory, gen_ptr, // compute address of mem[r15]
                                                   SCRATCH_REG, VMEM_BASE_REG);
                    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                                        switch_endianness(pc), SCRATCH_REG, 0);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   CPU_STATE_REG, RDI);
                    gen_ptr += x64_map_call(memory, gen_ptr, RAX);
                    break;
                }
                default: {
                    invalid_op = 1;
                    break;
                }

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
                        
                        // load arguments
                        LOAD_VREG_TO_REG(src, RDI);
                        LOAD_VREG_TO_REG(dest, RSI);


                        // clear RAX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                         RAX, RAX);
                        gen_ptr += x64_map_cmp_reg2reg16(memory, gen_ptr,
                                                         RDI, RSI);
                        // move below bit to AL
                        gen_ptr += x64_map_setb(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);


                        break;
                    }
                    case I_EQU: {
                        // load arguments
                        LOAD_VREG_TO_REG(src, RDI);
                        LOAD_VREG_TO_REG(dest, RSI);

                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                         RAX, RAX);
                        // calc src == dest
                        gen_ptr += x64_map_cmp_reg2reg16(memory, gen_ptr,
                                                         RDI, RSI);
                        // move zero bit to AL
                        gen_ptr += x64_map_setz(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);

                        break;
                    }
                    case I_TEST: {
                        // load arguments
                        LOAD_VREG_TO_REG(src, RDI);
                        LOAD_VREG_TO_REG(dest, RSI);

                        // clear RAX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                         RAX, RAX);
                        // calc (src & dest) != 0
                        gen_ptr += x64_map_test_reg2reg16(memory, gen_ptr,
                                                          RDI, RSI);

                        // move NE to AL
                        gen_ptr += x64_map_setne(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);

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
                                RAX, SCRATCH_REG, 0);
                        // the loaded word is in big-endian format, but x64 is little endian
                        // therefore, we must swap AH and AL using the `XCHG` instruction
                        gen_ptr += x64_map_xchg_r8(memory, gen_ptr,
                                                   AH, AL);
                        // write the swapped integer to the appropriate register
                        STORE_REG_TO_VREG(RAX, dest);
                        break;
                    }
                    case I_STOR: {
                        // load destination address
                        LOAD_VREG_TO_SCRATCH(dest);

                        // need to convert to big-endian format before writing values to memory
                        LOAD_VREG_TO_REG(src, RAX);
                        gen_ptr += x64_map_xchg_r8(memory, gen_ptr,
                                                   AH, AL);

                        // compute effective address
                        gen_ptr += x64_map_add_reg2reg(memory, gen_ptr,
                                                       SCRATCH_REG, VMEM_BASE_REG);

                        // store 16-bit word
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, SCRATCH_REG, 0);

                        break;
                    }
                    case I_LOADB: {
                        // load src address
                        LOAD_VREG_TO_SCRATCH(src);

                        // clear RAX
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                        RAX, 0);

                        // compute effective address
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                       VMEM_BASE_REG, RBX);
                        gen_ptr += x64_map_add_reg2reg(memory, gen_ptr,
                                                       RBX, SCRATCH_REG);

                        // load one byte
                        gen_ptr += x64_map_mov_m8_2_r8(memory, gen_ptr,
                                                       RBX, AL);

                        // store to target register
                        STORE_REG_TO_VREG(RAX, dest);
                        break;
                    }
                    case I_STORB: {
                        // load dest address
                        LOAD_VREG_TO_SCRATCH(dest);

                        // load value to store
                        LOAD_VREG_TO_REG(src, RAX);

                        // compute effective address
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                       VMEM_BASE_REG, RBX);
                        gen_ptr += x64_map_add_reg2reg(memory, gen_ptr,
                                                       RBX, SCRATCH_REG);

                        // store value
                        gen_ptr += x64_map_mov_r8_2_m8(memory, gen_ptr,
                                                       AL, RBX);
                        break;
                    }
                    default: {
                        // invalid opcode
                        invalid_op = 1;
                        pc -=2;
                        break;
                    }
                }
        } else if (XIS_NUM_OPS(opcode) == 0) { // no operands
            switch (opcode) {
                case I_RET:{
                    LOAD_VREG_TO_SCRATCH(15); // load pc to register

                    // load value at pc
                    gen_ptr += x64_map_add_reg2reg(memory, gen_ptr, // compute address
                                                   SCRATCH_REG, VMEM_BASE_REG);
                    gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr, // scratch = [scratch]
                                                          RAX, SCRATCH_REG, 0);

                    // swap bytes
                    gen_ptr += x64_map_xchg_r8(memory, gen_ptr,
                                               AH, AL);
                    // at this point RAX contains the new pc, so write it to the proper field
                    gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                        RAX, CPU_STATE_REG, 2 * PC_INDEX);

                    // need to add 2 to the stack pointer
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RAX, 2);
                    gen_ptr += x64_map_add_indirect(memory, gen_ptr,
                                                    RAX, CPU_STATE_REG, 2 * 15);


                    emitted_ret = 1;
                    break;
                }
                case I_STD: {
                    // use the escape hatch to get out, "returning" a value of 1

                    // first actually set the debug bit
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    SCRATCH_REG, 2);
                    gen_ptr += x64_map_or_indirect(memory, gen_ptr,
                                                   SCRATCH_REG, CPU_STATE_REG, (FLAGS_INDEX * 2));

                    invalid_op = 1;
                    emitted_ret = 1;
                    break;
                }
                case I_CLD: {
                    // this should never be reached
                    invalid_op = 1;
                    emitted_ret = 1;
                    break;
                }
            }
        } else { // 1 operand
            unsigned char reg = XIS_REG1(instruction);
            switch (opcode) {
                case I_PUSH: {
                    // load register value and switch endianness
                    LOAD_VREG_TO_REG(reg, RAX);
                    gen_ptr += x64_map_xchg_r8(memory, gen_ptr,
                                               AH, AL);

                    // sub 2 from stack pointer
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    SCRATCH_REG, 2);
                    gen_ptr += x64_map_sub_indirect(memory, gen_ptr,
                                                    SCRATCH_REG, CPU_STATE_REG, (2 * 15));

                    LOAD_VREG_TO_SCRATCH(15); // load stack pointer to scratch

                    gen_ptr += x64_map_add_reg2reg(memory, gen_ptr, // compute effective address
                                                   SCRATCH_REG, VMEM_BASE_REG);
                    gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr, // store value in virtual memory
                                                        RAX, SCRATCH_REG, 0);
                    break;
                }
                case I_NEG: {
                    gen_ptr += x64_map_neg_indirect(memory, gen_ptr,
                                                    CPU_STATE_REG, (2 * reg));
                    break;
                }
                case I_NOT: {
                    LOAD_VREG_TO_SCRATCH(reg);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RAX, 0);
                    gen_ptr += x64_map_test(memory, gen_ptr,
                                            SCRATCH_REG, SCRATCH_REG);
                    gen_ptr += x64_map_setz(memory, gen_ptr, AL);
                    STORE_REG_TO_VREG(RAX, reg);
                    break;
                }
                case I_INC: {
                    gen_ptr += x64_map_inc_dec_indirect(memory, gen_ptr,
                                                        MODE_INC, CPU_STATE_REG, (2 * reg));
                    break;
                }
                case I_DEC: {
                    gen_ptr += x64_map_inc_dec_indirect(memory, gen_ptr,
                                                        MODE_DEC, CPU_STATE_REG, (2 * reg));
                    break;
                }
                case I_POP: {
                    // reg is dest
                    LOAD_VREG_TO_SCRATCH(15);
                    gen_ptr += x64_map_add_reg2reg(memory, gen_ptr,
                                                   SCRATCH_REG, VMEM_BASE_REG);
                    gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr,
                                                          RAX, SCRATCH_REG, 0);
                    // switch endianness
                    gen_ptr += x64_map_xchg_r8(memory, gen_ptr,
                                               AL, AH);
                    STORE_REG_TO_VREG(RAX, reg);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    SCRATCH_REG, 2);
                    gen_ptr += x64_map_add_indirect(memory, gen_ptr,
                                                    SCRATCH_REG, CPU_STATE_REG, (2 * 15));
                    break;
                }
                case I_OUT: {
                    LOAD_VREG_TO_REG(reg, RDI);
                    WRITE_FUNCTION_CALL(putchar);
                    break;
                }
                case I_JR: {
                    // make link request
                    BEGIN_LINK_REQUEST(LINK_JUMP_REL)

                    // encode this as a dynamic jump, the linker will overwrite it if it can
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    char offset = (char) (instruction & 0xFF);
                    unsigned short target = pc + offset - 2;
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RSI, target);

                    WRITE_FUNCTION_CALL(jump_dynamic)

                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);

                    SUBMIT_LINK_REQUEST(target)
                    break;
                }
                case I_BR: {
                    BEGIN_LINK_REQUEST(LINK_BRANCH_REL)

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    char offset = (char) (instruction & 0xFF);
                    unsigned short target = pc + offset - 2;
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RSI, target);

                    LOAD_VREG_TO_REG(FLAGS_INDEX, RAX)

                    // sets the zero flag
                    gen_ptr += x64_map_and_imm16(memory, gen_ptr, RAX, 1);
                    // skip the following code if the register flag was zero
                    gen_ptr += x64_map_jz_rel8(memory, gen_ptr, 0);

                    // used to calculate the offset after generating the rest of the function
                    int right_after_jz = gen_ptr;
                    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                                        target, CPU_STATE_REG, PC_INDEX * 2);
                    // jump to target address
                    WRITE_FUNCTION_CALL(jump_dynamic)
                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);

                    // setup conditional jump
                    char amount_to_jump = (char) (gen_ptr - right_after_jz);
                    memory[right_after_jz - 1] = amount_to_jump;

                    SUBMIT_LINK_REQUEST(target)
                    break;
                }
                case I_JMPR: {
                    LOAD_VREG_TO_REG(reg, RSI);
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);

                    WRITE_FUNCTION_CALL(jump_dynamic)

                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr,
                                                    RAX);
                    break;
                }
                case I_CALLR: {
                    LOAD_VREG_TO_REG(reg, RSI);
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);

                    WRITE_FUNCTION_CALL(call_dynamic)

                    gen_ptr += x64_map_call(memory, gen_ptr, RAX);

                    break;
                }
                default: {
                    invalid_op = 1;
                    break;
                }
            }

        }

        processed_instructions++;
        if (invalid_op || opcode == 0) {
            pc -= 2;
            break;
        }
        if (emitted_ret) {
            break;
        }
    }

    
    /**
     * Post generation: 
     * Update program counter to procedure end position
    */
    if (!emitted_ret) {
        gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                            pc, CPU_STATE_REG, (2 * PC_INDEX));
    }

    /**
     * If invalid op, immediately return to entry with 0 value
     * If invalid_op AND emitted_ret, immediately return to entry with a result of 1
     */
    if (invalid_op) {
        gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                            pc, CPU_STATE_REG, (2 * PC_INDEX));
        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                        RAX, (unsigned long) state);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              RBX, RAX, 8);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              RSP, RAX, 16);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              RBP, RAX, 24);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              12, RAX, 32);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              13, RAX, 40);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              14, RAX, 48);
        gen_ptr += x64_map_mov_indirect2reg64(memory, gen_ptr,
                                              15, RAX, 56);

        gen_ptr += x64_map_ret(memory, gen_ptr);
    }

    /**
     * Post generation:
     * Restore saved registers
    */
    gen_ptr += write_function_exit_preamble(memory, gen_ptr);

    // return 0 - did not panic
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, RAX, 0);
    gen_ptr += x64_map_ret(memory, gen_ptr);

    jit_link(link_requests, instr_mapping, memory);

    jit_prepared_function *result = malloc(sizeof(jit_prepared_function));
    result->function = (jit_func)memory;
    result->addr_start = address;
    result->addr_end = pc;
    result->generated_size = gen_ptr;
    result->translated_instruction_count = processed_instructions;

    ll_destroy(link_requests);

    free(instr_mapping->addrs);
    free(instr_mapping);

    return result;
}