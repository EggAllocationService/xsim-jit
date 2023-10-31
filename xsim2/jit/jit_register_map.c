#include "jit_register_map.h"
#include <string.h>
#include <stdlib.h>

extern jit_register_map *new_map() {
    jit_register_map *mem = malloc(sizeof(jit_register_map));
    for (int i = 0; i < 7; i++) {
        mem->assignments[i] = -1;
        mem->allocation_order[i] = 7 - i;
    }
    return mem;
}

extern char get_first_free_space(jit_register_map *map) {
    for (int i = 0; i < 7; i++) {
        if (map->assignments[i] == -1) {
            return i;
        }
    }
    return -1;
}