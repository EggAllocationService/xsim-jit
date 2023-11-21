#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "jit.h"
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

  // initialize cpu state
  xcpu *cpu = calloc(sizeof(xcpu), 1);

  int max_instruction_count = -1;
  sscanf(argv[1], "%d", &max_instruction_count);

  // if there's no instruction limit, and the DISABLE_JIT flag isn't set, we can JIT the X-CPU program
  if (max_instruction_count == 0) {
      char *should_disable = getenv("DISABLE_JIT");
      if (should_disable == NULL || (strcmp(should_disable, "1") != 0)) {
          // initialize JIT state
          jit_init_state();

          // compile entry point to a function
          xmem_virt_mem memory_info = xmem_get_virt_mem();
          jit_prepared_function *entrypoint = jit_prepare(memory_info.memory, 0, 1, 0);

          // simulate cpu behavior
          unsigned char result = entrypoint->function(cpu);
          if (result == 0) { // guest code ran to completion
              return 0;
          }
          // if there was an error, continue with the interpreter
      }
  }

  int instruction_count = 0;
  while (1) {
    int successful = xcpu_execute(cpu);
    instruction_count += 1;
    if (!successful || (instruction_count >= max_instruction_count) && max_instruction_count != 0) break;
  }

  return 0;
}


