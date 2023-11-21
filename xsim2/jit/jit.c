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
#include "vreg_table.h"

static jit_state *state;

extern void jit_init_state() {
    state = malloc(sizeof(jit_state));
    state->guest_debug_bit = 0;
    state->function_cache = ll_new();
}
extern jit_state *jit_get_state() {
    return state;
}



#define VREG_IS_MAPPED(reg) ((table->mapped_regs & (1 << (reg))) > 0)

/**
 * Macro that creates a variable `destvar`, holding the register containing `vreg`
 * If `vreg` was already mapped, destvar holds the x86 register that contains it
 * Else, this macro will load `vreg` to `loadtarget`, and `destvar` will contain `loadtarget`
*/
#define GET_OR_LOAD_VREG(destvar, vreg, loadtarget) char destvar; \
            if (VREG_IS_MAPPED(vreg)) {destvar = get_mapping(table, vreg);} \
            else { \
                gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr, \
                            loadtarget, CPU_STATE_REG, (2 * vreg)); \
                destvar = loadtarget;}
/**
 * If `physreg` is not one of the mapped virtual registers, then this macro will store
 * the 16-bit value contained in physreg to vreg
*/
#define STORE_IF_NEEDED(physreg, vreg) if (physreg < 8) {STORE_ALWAYS(physreg, vreg)}

#define STORE_ALWAYS(physreg, vreg) gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr, \
                                                            physreg, CPU_STATE_REG, (2 * vreg));

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


extern unsigned short load_short(unsigned char *mem, int offset) {
    unsigned short result = 0;
    result |= mem[offset] << 8;
    result |= mem[offset + 1];
    return result;
}

static int load_virtual_registers(unsigned char *memory, int gen_ptr, vreg_table *table, char only_caller_save) {
    int offset = 0;
    int max_index = 7;
    if (only_caller_save) {
        max_index = 4;
    }
    for (int i = 0; i < max_index; i++) { // load all virtual pointers
        if (table->mapped[i] == -1) continue;
        offset += x64_map_movzx_indirect2reg(memory, gen_ptr + offset,
                                            8 + i, CPU_STATE_REG, 2 * table->mapped[i]);
    }
    return offset;
}
static int store_mapped_virtual_registers(unsigned char *memory, int gen_ptr, vreg_table *table, char only_caller_save) {
    int offset = 0;
    int max_index = 7;
    if (only_caller_save) {
        max_index = 4;
    }
    for (int i = 0; i < max_index; i++) {
        if (table->mapped[i] == -1) continue;
        offset += x64_map_mov_reg2indirect(memory, gen_ptr + offset,
                                           8 + i, CPU_STATE_REG, 2 * table->mapped[i]);
    }
    return offset;
}

static char get_mapping(vreg_table *table, char vreg) {
    for (char i = 0; i < 7; i++) {
        if (table->mapped[i] == vreg) {
            return 8 + i;
        }
    }
    return -1;
}


extern jit_prepared_function *jit_prepare(unsigned char *program, unsigned short address, char is_entrypoint,
                                          char internal_abi) {

    // map some memory for the generated code
    unsigned char *memory = mmap(NULL, 16000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    
    unsigned short pc = address;
    int gen_ptr = 0;
    int processed_instructions = 0;
    char emitted_ret = 0;

    // generate vreg -> reg mapping table
    vreg_table *table = solve_instruction_region(program, address);

    /**
     * Optimization for recursive functions: internal_abi preserves registers between calls, allowing fast
     * recursion
     */
     if (!internal_abi) {

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


         // move the address of the cpu struct into CPU_STATE_REG
         if (CPU_STATE_REG != RDI) {
             gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, RDI, CPU_STATE_REG);
         }


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



         // load all mapped virtual registers
         gen_ptr += load_virtual_registers(memory, gen_ptr, table, 0);

     }

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
                    char target_vreg = XIS_REG1(instruction);

                    GET_OR_LOAD_VREG(target_reg, target_vreg, SCRATCH_REG)

                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                            target_reg, extended_value);

                    STORE_IF_NEEDED(target_reg, target_vreg)
                    break;
                }
                case I_JMP: {
                    
                    // call jump_dynamic to get the address we should be jumping to
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                   RSI, extended_value);
                    WRITE_FUNCTION_CALL(jump_dynamic)


                    gen_ptr += store_mapped_virtual_registers(memory, gen_ptr,
                                                              table, 0); // save all mapped registers

                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);
                    break;
                }
                case I_CALL: {
                    GET_OR_LOAD_VREG(sp, 15, SCRATCH_REG)
                    if(extended_value != address) {
                        // call_static will overwrite this region, so repeated calls are faster
                        // there are 36 bytes in this block

                        // save mapped registers
                        gen_ptr += store_mapped_virtual_registers(memory, gen_ptr, table, 0);



                        // sub 2 from sp
                        gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                         MODE_DEC, sp);
                        gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                         MODE_DEC, sp);
                        // write return address to scratch
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                        SCRATCH_REG_2, switch_endianness(pc));
                        // store return address on stack
                        gen_ptr += x64_map_mov_8_16_reg2scaled(memory, gen_ptr,
                                                               SCRATCH_REG_2, VMEM_BASE_REG, sp, MOV_SCALE_16);
                        // write new stack pointer
                        STORE_ALWAYS(sp, 15)

                        int start = gen_ptr;
                        unsigned long call_start = ((unsigned long)memory) + gen_ptr;
                        // load call_static parameters
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                       VMEM_BASE_REG, RDI);
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                        RSI, extended_value);
                        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                        RDX, call_start);

                        // store all callee-saved mapped registers

                        WRITE_FUNCTION_CALL(call_static);

                        // end overwritten region

                        int overwritten_len = gen_ptr - start;
                        if (overwritten_len != CALL_STATIC_OVERWRITE_SIZE) {
                            printf("Assertion failed: check CALL_STATIC_OVERWRITE_SIZE! (%d)", overwritten_len);
                            exit(-4);
                        }
                        memory[gen_ptr++] = 0x90;
                        memory[gen_ptr++] = 0x90;
                        memory[gen_ptr++] = 0x90;

                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, CPU_STATE_REG, RDI);
                        gen_ptr += x64_map_call(memory, gen_ptr, RAX);

                        // reload all mapped registers
                        gen_ptr += load_virtual_registers(memory, gen_ptr, table, 0);
                        // reload memory register
                    } else {
                        // recursive function, so register mapping is guaranteed to be the same

                        gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_DEC, sp);
                        gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_DEC, sp);
                        STORE_ALWAYS(sp, 15)

                        // prevent recursive compilation
                        if (!internal_abi) {
                            jit_prepared_function *internal_version = jit_prepare(program, address,
                                                                                  is_entrypoint, 1);
                            gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, RAX,
                                                            (unsigned long) internal_version->function);

                        } else {
                            // call the currently generated version
                            gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, RAX,
                                                            (unsigned long) memory);
                        }

                        gen_ptr += x64_map_call(memory, gen_ptr, RAX);

                    }


                    break;
                }
                default: {
                    invalid_op = 1;
                    break;
                }

            }
        } else if (XIS_NUM_OPS(opcode) == 2) { // two register operands
                char vsrc = XIS_REG1(instruction);
                char vdest = XIS_REG2(instruction);

                GET_OR_LOAD_VREG(src, vsrc, SCRATCH_REG)
                GET_OR_LOAD_VREG(dest, vdest, SCRATCH_REG_2)

                switch(opcode) {
                    case I_ADD: {
                        gen_ptr += x64_map_add_reg2reg(memory, gen_ptr,
                                    dest, src);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_SUB: {
                        gen_ptr += x64_map_sub_reg2reg16(memory, gen_ptr,
                                src, dest);

                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_MUL: {
                        // make sure dest is in RAX
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                    dest, RAX);
                        // clear DX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                    RDX, RDX);
                        // mul vsrc -> ax
                        gen_ptr += x64_map_mul_reg2ax16(memory, gen_ptr,
                                    src);
                        // mov ax -> dest
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                RAX, dest);

                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_DIV: {
                        // make sure dest is in RAX
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                    dest, RAX);
                        // clear DX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                    RDX, RDX);
                        // div vsrc -> ax
                        gen_ptr += x64_map_div_reg2ax16(memory, gen_ptr,
                                    src);
                        // mov ax -> dest
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                RAX, dest);
                                
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_AND: {
                        gen_ptr += x64_map_and_reg2reg16(memory, gen_ptr,
                                    src, dest);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_OR: {
                        gen_ptr += x64_map_or_reg2reg16(memory, gen_ptr,
                                src, dest);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_XOR: {
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                    src, dest);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_SHR: {
                        if (src != RCX) {
                            gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                        src, RCX);
                        }
                        gen_ptr += x64_map_shr_reg2reg16(memory, gen_ptr,
                                        dest);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_SHL: {
                        if (src != RCX) {
                            gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                        src, RCX);
                        }
                        gen_ptr += x64_map_shl_reg2reg16(memory, gen_ptr,
                                        dest);
                        STORE_IF_NEEDED(dest, vdest)
                    }
                    case I_CMP: {
                        
                        // the JIT doesn't support the debug bit, so we can overwrite

                        // clear RAX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                        RAX, RAX);
                        // calc src < dest
                        gen_ptr += x64_map_cmp_reg2reg16(memory, gen_ptr,
                                                        src, dest);
                        // move below bit to AL
                        gen_ptr += x64_map_setb(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);
                        break;
                    }
                    case I_EQU: {
                        // clear RAX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                        RAX, RAX);
                        // calc src == dest
                        gen_ptr += x64_map_cmp_reg2reg16(memory, gen_ptr,
                                                        dest, src);
                        // move zero bit to AL
                        gen_ptr += x64_map_setz(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);
                        break;
                    }
                    case I_TEST: {
                        // clear RAX
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr,
                                                        RAX, RAX);
                        // calc (src & dest) != 0
                        gen_ptr += x64_map_test_reg2reg16(memory, gen_ptr,
                                                dest, src);

                        // move NE to AL
                        gen_ptr += x64_map_setne(memory, gen_ptr, AL);

                        // store new result
                        gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                            RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);
                        break;
                    }
                    case I_MOV: {
                        gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                        src, dest);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_LOAD: {
                        gen_ptr += x64_map_mov_8_16_scaled2reg(memory, gen_ptr,
                                                               dest, VMEM_BASE_REG, src, MOV_SCALE_16);
                        // swap byte order
                        gen_ptr += x64_map_ror_rm16_imm8(memory, gen_ptr,
                                                         dest, 8);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_STOR: {

                        gen_ptr += x64_map_mov_8_16_reg2scaled(memory, gen_ptr,
                                                               src, VMEM_BASE_REG, dest, MOV_SCALE_16);
                        break;
                    }
                    case I_LOADB: {
                        // zero destination register
                        gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr, dest, dest);
                        gen_ptr += x64_map_mov_8_16_scaled2reg(memory, gen_ptr,
                                                               dest, VMEM_BASE_REG, src, MOV_SCALE_8);
                        STORE_IF_NEEDED(dest, vdest)
                        break;
                    }
                    case I_STORB: {
                        gen_ptr += x64_map_mov_8_16_reg2scaled(memory, gen_ptr,
                                                               src, VMEM_BASE_REG, dest, MOV_SCALE_8);
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
                    GET_OR_LOAD_VREG(sp, 15, SCRATCH_REG)

                    // pop pc from stack
                    gen_ptr += x64_map_mov_8_16_scaled2reg(memory, gen_ptr,
                                                           SCRATCH_REG_2, VMEM_BASE_REG, sp, MOV_SCALE_16);

                    // stackptr ++
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_INC, sp);
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_INC, sp);

                    STORE_IF_NEEDED(sp, 15)

                    // switch endianness
                    gen_ptr += x64_map_ror_rm16_imm8(memory, gen_ptr,
                                                     SCRATCH_REG_2, 8);

                    // store new pc value in cpu struct
                    gen_ptr += x64_map_mov_reg2indirect(memory, gen_ptr,
                                                        SCRATCH_REG_2, CPU_STATE_REG, PC_INDEX * 2);

                    emitted_ret = 1;
                    break;
                }
                case I_STD: {
                    // use the escape hatch to get out, "returning" a value of 1

                    // first load the debug bit
                    GET_OR_LOAD_VREG(flags, FLAGS_INDEX, SCRATCH_REG)

                    gen_ptr += x64_map_or_imm16(memory, gen_ptr, flags, 2);

                    STORE_IF_NEEDED(flags, FLAGS_INDEX)

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
                default: {
                    invalid_op = 1;
                    break;
                }
            }
        } else { // 1 operand
            unsigned char vreg = XIS_REG1(instruction);
            GET_OR_LOAD_VREG(reg, vreg, SCRATCH_REG)
            switch (opcode) {
                case I_PUSH: {
                    // switch endianness
                    gen_ptr += x64_map_ror_rm16_imm8(memory, gen_ptr,
                                                     reg, 8);

                    // sub 2 from stack pointer
                    GET_OR_LOAD_VREG(sp, 15, SCRATCH_REG_2)
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                     MODE_DEC, sp);
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                     MODE_DEC, sp);
                    // push value to stack
                    gen_ptr += x64_map_mov_8_16_reg2scaled(memory, gen_ptr,
                                                           reg, VMEM_BASE_REG, sp, MOV_SCALE_16);
                    gen_ptr += x64_map_ror_rm16_imm8(memory, gen_ptr,
                                                     reg, 8);

                    STORE_IF_NEEDED(sp, 15)
                    break;
                }
                case I_NEG: {
                    gen_ptr += x64_map_neg_reg16(memory, gen_ptr, reg);

                    STORE_IF_NEEDED(reg, vreg)
                    break;
                }
                case I_NOT: {

                    // clear RAX
                    gen_ptr += x64_map_xor_reg2reg64(memory, gen_ptr, RAX, RAX);

                    gen_ptr += x64_map_test_reg2reg16(memory, gen_ptr,
                                            reg, reg);

                    gen_ptr += x64_map_setz(memory, gen_ptr, AL);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, RAX, reg);

                    STORE_IF_NEEDED(reg, vreg)
                    break;
                }
                case I_INC: {
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                     MODE_INC, reg);
                    STORE_IF_NEEDED(reg, vreg)
                    break;
                }
                case I_DEC: {
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr,
                                                     MODE_DEC, reg);
                    STORE_IF_NEEDED(reg, vreg)
                    break;
                }
                case I_POP: {
                    GET_OR_LOAD_VREG(sp, 15, SCRATCH_REG_2)

                    // pop value
                    gen_ptr += x64_map_mov_8_16_scaled2reg(memory, gen_ptr,
                                                           reg, VMEM_BASE_REG, sp, MOV_SCALE_16);
                    // switch endianness
                    gen_ptr += x64_map_ror_rm16_imm8(memory, gen_ptr,
                                                     reg, 8);

                    // stackptr ++
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_INC, sp);
                    gen_ptr += x64_map_inc_dec_reg16(memory, gen_ptr, MODE_INC, sp);

                    STORE_IF_NEEDED(sp, 15)
                    STORE_IF_NEEDED(reg, vreg)
                    break;
                }
                case I_OUT: {
                    // save mapped registers
                    gen_ptr += store_mapped_virtual_registers(memory, gen_ptr, table, 1);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, reg, RDI);
                    WRITE_FUNCTION_CALL(putchar);

                    // restore execution state
                    gen_ptr += load_virtual_registers(memory, gen_ptr, table, 1);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, VMEM_BASE_REG, ((unsigned long) program));
                    break;
                }
                case I_JR: {
                    BEGIN_LINK_REQUEST(LINK_JUMP_REL)

                    // save execution state
                    gen_ptr += x64_map_push(memory, gen_ptr, CPU_STATE_REG);
                    gen_ptr += store_mapped_virtual_registers(memory, gen_ptr, table, 0);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);

                    char offset = (char) (instruction & 0xFF);
                    unsigned short target = pc + offset - 2;
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RSI, target);
                    // encode this as a dynamic jump, the linker will overwrite it if it can
                    WRITE_FUNCTION_CALL(jump_dynamic)

                    // restore execution state
                    gen_ptr += x64_map_pop(memory, gen_ptr, CPU_STATE_REG);
                    gen_ptr += load_virtual_registers(memory, gen_ptr, table, 1);

                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);

                    SUBMIT_LINK_REQUEST(target)
                    break;
                }
                case I_BR: {
                    BEGIN_LINK_REQUEST(LINK_BRANCH_REL)

                    char offset = (char) (instruction & 0xFF);
                    unsigned short target = pc + offset - 2;


                    // load flags
                    gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr,
                                                          RAX, CPU_STATE_REG, FLAGS_INDEX * 2);
                    // sets the zero flag
                    gen_ptr += x64_map_and_imm16(memory, gen_ptr, RAX, 1);
                    // skip the following code if the register flag was zero
                    gen_ptr += x64_map_jz_rel8(memory, gen_ptr, 0);

                    // used to calculate the offset after generating the rest of the function
                    int right_after_jz = gen_ptr;

                    // ======== BEGIN CONDITIONAL BLOCK =======
                    // push execution state
                    gen_ptr += store_mapped_virtual_registers(memory, gen_ptr, table, 0);

                    gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr, // store new pc
                                                        target, CPU_STATE_REG, PC_INDEX * 2);
                    // setup jump_dynamic params
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr,
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr,
                                                    RSI, target);
                    // calculate jump target
                    WRITE_FUNCTION_CALL(jump_dynamic)

                    // pop execution state
                    gen_ptr += load_virtual_registers(memory, gen_ptr, table, 1);
                    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, VMEM_BASE_REG, ((unsigned long) program));

                    // jump to address
                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr, RAX);
                    // ======= END CONDITIONAL BLOCK =======

                    // setup conditional jump
                    char amount_to_jump = (char) (gen_ptr - right_after_jz);
                    memory[right_after_jz - 1] = amount_to_jump;

                    SUBMIT_LINK_REQUEST(target)
                    break;
                }
                case I_JMPR: {
                    // store all registers
                    store_mapped_virtual_registers(memory, gen_ptr, table, 0);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, // load virtual memory base
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, // load target address
                                                   reg, RSI);

                    WRITE_FUNCTION_CALL(jump_dynamic)

                    gen_ptr += x64_map_absolute_jmp(memory, gen_ptr,
                                                    RAX);
                    break;
                }
                case I_CALLR: {

                    // store all registers
                    store_mapped_virtual_registers(memory, gen_ptr, table, 0);

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, // load virtual memory base
                                                   VMEM_BASE_REG, RDI);
                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, // load target address
                                                   reg, RSI);

                    WRITE_FUNCTION_CALL(call_dynamic)

                    gen_ptr += x64_map_mov_reg2reg(memory, gen_ptr, CPU_STATE_REG, RDI);
                    gen_ptr += x64_map_call(memory, gen_ptr, RAX);

                    // reload all registers
                    gen_ptr += load_virtual_registers(memory, gen_ptr, table, 0);

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
     * Post gen: 
     * Update program counter to procedure end position
    */
    if (!emitted_ret) {
        gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                            pc, CPU_STATE_REG, (2 * PC_INDEX));
    }
    /**
     * Post gen: save all mapped registers
     */
    if (!internal_abi) {
        gen_ptr += store_mapped_virtual_registers(memory, gen_ptr,
                                                  table, 0); // 0 = all mapped registers
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

        gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, RAX, emitted_ret); // load return value

        gen_ptr += x64_map_ret(memory, gen_ptr);
    }


    /**
     * Post generation:
     * Restore saved registers
    */
    if (!internal_abi) {
        gen_ptr += x64_map_pop(memory, gen_ptr, 15);
        gen_ptr += x64_map_pop(memory, gen_ptr, 14);
        gen_ptr += x64_map_pop(memory, gen_ptr, 13);
        gen_ptr += x64_map_pop(memory, gen_ptr, 12);
        gen_ptr += x64_map_pop(memory, gen_ptr, RBX);
    }

    // return 0 - did not panic
    gen_ptr += x64_map_move_imm2reg(memory, gen_ptr, RAX, 0);
    gen_ptr += x64_map_ret(memory, gen_ptr);

    // link in-procedure jumps if possible
    jit_link(link_requests, instr_mapping, memory);

    jit_prepared_function *result = malloc(sizeof(jit_prepared_function));
    result->function = (jit_func)memory;
    result->addr_start = address;
    result->addr_end = pc;
    result->generated_size = gen_ptr;
    result->translated_instruction_count = processed_instructions;

    /**
     * Cleanup
    */
    ll_destroy(link_requests);

    free(instr_mapping->addrs);
    free(instr_mapping);

    return result;
}