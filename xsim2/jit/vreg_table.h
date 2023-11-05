#pragma once

typedef struct vreg_table {
    /**
     * Index is the extended x86 register, actual register 7 + i
     * Value is the index of the virtual register that was mapped
     * 
     * A value of -1 means that physical register is unused
    */
    char mapped[7];

    /**
     * Bitfield storing which registers have been mapped
    */
    unsigned short mapped_regs;
} vreg_table;

/**
 * This function scans a virtual program, and returns a table where as many virtual registers as possible
 * have been mapped into phyiscal ones
 * 
*/
extern vreg_table *solve_instruction_region(unsigned char *program, unsigned short address);

/**
 * This function returns a true/false value of if `vreg` is mapped into a physical register
*/
extern int is_mapped(vreg_table* table, char vreg);