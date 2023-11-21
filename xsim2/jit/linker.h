//
// Created by kyle on 11/1/23.
//
#pragma once
#include "linkedlist.h"

enum LINK_REQUEST_TYPE {
    LINK_JUMP_REL,
    LINK_BRANCH_REL
};

typedef struct link_req {
    unsigned long from;
    unsigned short target_pc;
    unsigned char type;
    unsigned int gen_code_length;
} link_req_t;

typedef struct translated_instructions {
    unsigned long *addrs;
    unsigned int length;
    unsigned int capacity;
    unsigned short relative_to;
} instr_results_t;

extern instr_results_t *new_instruction_container(unsigned short base);

/**
 * Add `addr` as the x86 instruction address for the next pc index
*/
extern void istr_container_push(instr_results_t *container, unsigned long addr);

/**
 * Using all the info contained in `requests`, this function attempts to patch the generated assembly to allow direct jumps inside the 
 * same generated code block, eliminating the overhead of dynamic jumps
*/
extern void jit_link(linked_list_t *requests, instr_results_t *results, unsigned char *memory);
