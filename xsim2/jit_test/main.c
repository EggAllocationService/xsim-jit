#include <stdio.h>
#include "jit.h"
#include "xcpudefs.h"
#include <string.h>
#include "xmem.h"
#include "xcpu.h"

int main() {
        
    if (!xmem_init(65536)) {
        printf("FATAL: Failed to allocate 64k of memory\n");
        return -2;
    }

    // read the executable, 2 bytes at a time
    unsigned char tmp[2];
    FILE *program = fopen("/home/kyle/CSCI2122/Assignment4/xsim2/test.xo", "r");
    for (int i = 0; 1; i+= 2) {
        memset(tmp, 0, 2);
        size_t result = fread(tmp, 1, 2, program);
        if (result == 0) break;
        xmem_store(tmp, i);
    }


    xmem_virt_mem mem = xmem_get_virt_mem();
    
    jit_init_state();
    jit_set_debug_function(xcpu_print);

    jit_prepared_function *entry = jit_prepare(mem.memory, 0);


    xcpu cpu;
    memset(&cpu, 0, sizeof(xcpu));

    // execute generated code
    entry->function(&cpu);
    cpu.pc = 0x38;
    xcpu_print(&cpu);
    return 0;
}