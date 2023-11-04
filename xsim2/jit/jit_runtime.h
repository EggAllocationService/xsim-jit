#pragma once
/*
    These functions may be called by generated code, usually because it was simpler to emit a function call
        than these functions directly

    It's worse for performance, but frankly I don't care because I'm not going to implement another eight variants of MOV
*/

#define CALL_STATIC_OVERWRITE_SIZE 35

/**
 * Sets the lowest bit of the cpu flags to (src & dest) == 0
*/
extern void calc_flags_test(xcpu *c, unsigned short src, unsigned short dest);

/**
 * Sets the lowest bit of the cpu flags to src < dest
*/
extern void calc_flags_cmp(xcpu *c, unsigned short src, unsigned short dest);

/**
 * Sets the lowest bit of the cpu flags to src == dest
*/
extern void calc_flags_equ(xcpu *c, unsigned short src, unsigned short dest);

/**
 * Prepares and jumps to a target virtual address
 * This method will use `jump_region_start` to overwrite the original call with a jump directly
 * to the prepared code
*/
extern void jump_static(xcpu *cpu, unsigned short address, unsigned long jump_region_start);

/**
 * Prepares a jumps to a target virtual address
 * This method will not attempt to statically link the caller to the target, and is used for the JR and BR instructions
 * This method will return the address the caller should jump to
*/
extern unsigned long jump_dynamic(unsigned char *memory, unsigned short address);

/**
 * Prepares a function call to a target virtual address
 * @return absolute address of compiled function
 */
extern unsigned long call_dynamic(unsigned char *memory, unsigned short address);

/**
 * Prepares a function call to a target virtual address
 * This method will use `jump_region_start` to overwrite the caller with a constant load
*/
extern unsigned long call_static(unsigned char *memory, unsigned short address, unsigned long call_start);