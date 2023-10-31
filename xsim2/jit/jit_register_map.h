#pragma once

/**
 * Used to keep track of which virtual registers are currently in hardware registers
*/
typedef struct jit_register_map {
    /**
     * virtual registers are mapped to hardware registers r8-r15
     * index 0 is r8, index 1 is r9, and so on
     * a negative value means no virtual register is mapped to that hardware register
    */
    char assignments[8];
    /**
     * Performance optimization that keeps track of which register was last pulled
     * from the xcpu struct
     * This way we first remap older registers before ones we just mapped, since each
     * mapping is one memory load and store
     * Access order is index 0 -> index 7, and the value at the index is the *hardware* register 
     * that was last mapped to something
    */
    unsigned char allocation_order[8];
} jit_register_map;

extern jit_register_map *new_map();

extern char get_first_free_space(jit_register_map *map);