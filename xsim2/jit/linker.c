//
// Created by kyle on 11/1/23.
//

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "linker.h"
#include "x64_codegen.h"
#include "registers.h"

#define CONTAINER_CAPACITY_STEP 100


extern instr_results_t *new_instruction_container(unsigned short base) {
    instr_results_t  *result = malloc(sizeof(instr_results_t));
    result->addrs = malloc(sizeof(unsigned long) * CONTAINER_CAPACITY_STEP);
    result->capacity = CONTAINER_CAPACITY_STEP;
    result->length = 0;
    result->relative_to = base;
    return result;
}

static void container_extend(instr_results_t *container) {
    unsigned long *new_mem = malloc(sizeof(unsigned long) * (CONTAINER_CAPACITY_STEP + container->capacity));
    memcpy(new_mem, container->addrs, (sizeof(unsigned long) * container->capacity));
    free(container->addrs);
    container->addrs = new_mem;
    container->capacity += CONTAINER_CAPACITY_STEP;
}

extern void istr_container_push(instr_results_t *container, unsigned long addr) {
    if (container->length == container->capacity) {
        container_extend(container);
    }
    container->addrs[container->length] = addr;
    container->length++;
}

extern void jit_link(linked_list_t *requests, instr_results_t *results, unsigned char *memory) {
    ll_iterator_t *iter = ll_iterator_new(requests);
    while (ll_has_next(iter)) {
        link_req_t *req = ll_next(iter);
        signed int relative_pc = req->target_pc - results->relative_to;
        if (relative_pc < 0 || relative_pc > results->length * 2) {
            continue; // target jump is outside the compiled function, so we can't do anything
        }
        size_t instr_index = relative_pc / 2;

        unsigned long target_address = results->addrs[instr_index];
        unsigned long relative_start = req->from - ((unsigned long) memory);

        // clear out the generated code with NOOPS
        for (int i = 0; i < req->gen_code_length; i++) {
            memory[relative_start + i] = 0x90; // NOOP
        }

        int gen_ptr = (int) relative_start;

        switch (req->type) {
            case LINK_JUMP_REL: {
                // MOV <linked address>, RAX
                // JMP RAX
                unsigned long current_address = ((unsigned long) memory) + gen_ptr;
                long jump_offset = ((long) target_address) - ((long)current_address);
                x64_map_jmp_rel32(memory, gen_ptr, ((int) jump_offset) - 5);
                break;
            }
            case LINK_BRANCH_REL: {
                // mov <flags>, RAX
                // and(immediate16) 1, AX
                // jz after
                // mov <target address>, FUNC_ADDR_REG
                // jmpabs FUNC_ADDR_REG
                gen_ptr += x64_map_movzx_indirect2reg(memory, gen_ptr,
                                                      RAX, CPU_STATE_REG, 2 * FLAGS_INDEX);
                gen_ptr += x64_map_and_imm16(memory, gen_ptr,
                                             RAX, 1);
                gen_ptr += x64_map_jz_rel8(memory, gen_ptr, 0);
                int right_after_jz = gen_ptr;
                // store new pc
                gen_ptr += x64_map_mov_imm2indirect(memory, gen_ptr,
                                                    req->target_pc, CPU_STATE_REG, PC_INDEX * 2);

                unsigned long current_address = ((unsigned long) memory) + gen_ptr;

                
                long jump_offset = ((long) target_address) - ((long)current_address);
                gen_ptr += x64_map_jmp_rel32(memory, gen_ptr, ((int) jump_offset) - 5);

                char amount_to_jump = (char) (gen_ptr - right_after_jz) ;
                memory[right_after_jz - 1] = amount_to_jump;

            }
            default: {
                break;
            }
        }
    }

}