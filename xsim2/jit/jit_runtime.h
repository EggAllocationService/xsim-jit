#pragma once
#define CALL_STATIC_OVERWRITE_SIZE 35

// header of a generated function, we want to skip this if we're doing a jump
#define PREAMBLE_LENGTH 20

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