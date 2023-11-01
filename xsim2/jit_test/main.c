#include <stdio.h>
#include "jit.h"
#include "xcpudefs.h"
#include <string.h>
#include "xmem.h"

int main() {
        
    if (!xmem_init(65536)) {
        printf("FATAL: Failed to allocate 64k of memory\n");
        return -2;
    }

    // read the executable, 2 bytes at a time
    unsigned char tmp[2];
    FILE *program = fopen("/users/cs/kyles/CSCI2122/Assignment4/xsim2/test.xo", "r");
    for (int i = 0; 1; i+= 2) {
        memset(tmp, 0, 2);
        size_t result = fread(tmp, 1, 2, program);
        if (result == 0) break;
        xmem_store(tmp, i);
    }

    // initalize cpu state
    xcpu cpu;
    cpu.pc = 0;
    cpu.state = 0;
    memset(cpu.regs, 0, X_MAX_REGS * 2);

    xmem_virt_mem mem = xmem_get_virt_mem();
    
    jit_init_state();

    jit_prepared_function *entry = jit_prepare(mem.memory, 0, mem.size /  4);
    // dump generated assembly to disk
    FILE *fp = fopen("jit_test.bin", "wb");
    fwrite(entry->function, entry->generated_size, 1, fp);
    fclose(fp);
    printf("Wrote %d bytes to jit_test.bin\n", entry->generated_size);

    xcpu cpu;
    memset(&cpu, 0, sizeof(xcpu));

    // execute generated code
    entry->function(&cpu);
    return 0;
}