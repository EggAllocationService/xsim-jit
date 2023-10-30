#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "xis.h"
#include "xcpu.h"
#include "xmem.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Expected 2 arguments.\n");
    return -1;
  }
  if (!xmem_init(65536)) {
    printf("FATAL: Failed to allocate 64k of memory\n");
    return -2;
  }

  // figure out how big the executable is
  struct stat program_info;
  stat(argv[2], &program_info);
  if (program_info.st_size > 65536) {
    printf("FATAL: Executable too large!\n");
    return 0;
  }

  // read the executable, 2 bytes at a time
  unsigned char tmp[2];
  FILE *program = fopen(argv[2], "r");
  for (int i = 0; i < program_info.st_size; i+= 2) {
    memset(tmp, 0, 2);
    fread(tmp, 1, 2, program);
    xmem_store(tmp, i);
  }

  // initalize cpu state
  xcpu cpu;
  cpu.pc = 0;
  cpu.state = 0;
  memset(cpu.regs, 0, X_MAX_REGS * 2);

  int max_instruction_count = -1;
  sscanf(argv[1], "%d", &max_instruction_count);

  int instruction_count = 0;
  while (1) {
    int successful = xcpu_execute(&cpu);
    instruction_count += 1;
    if (!successful || instruction_count >= max_instruction_count) break;
  }

  return 0;
}


