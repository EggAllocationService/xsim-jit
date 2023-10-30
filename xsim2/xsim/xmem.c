#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xis.h"
#include "xmem.h"

static unsigned char *memory;
static int phys_size;

/* title: init memory with a specific size
 * param: size of physical memory (between 0 and 65536)
 * function: iniializes physical memory of specified size
 * returns: 1 if successful, 0 if not
 */
extern int xmem_init(int size) {
  if (size <= XIS_MEM_SIZE) {
    phys_size = size;
    memory = calloc(1, size);
    return memory != NULL;
  }
  return 0;
}


/* title: store a word (2 bytes) of data at specified location in memory
 * param: pointer to data, location in memory
 * function: writes the two bytes at specific address in memory.  Address must
 *           be in logical range (0 ... 65536), and if no virtual mapping
 *           is used must be less than size of physical memory.
 *           this function aborts if an error occurs.
 * returns: void
 */
extern void xmem_store(unsigned char data[2], unsigned short addr) {
  if (memory && (addr <= phys_size - 2)) {
    memcpy(memory + addr, data, 2);
  } else {
    printf("Memory store error at %d\n", addr);
    exit(1);
  }
}


/* title: load a word (2 bytes) of data from specified location in memory
 * param: location in memory, pointer to where data should be stored 
 * function: loads the 2 bytes stored at the specified address into the 
             location pointed to by data.  Address must be in logical 
             range (0 ... 65536), and if no virtual mapping is used must 
             be less than size of physical memory.
 * returns: void
 */
extern void xmem_load(unsigned short addr, unsigned char data[2]) {
  if (memory && (addr <= phys_size - 2)) {
    memcpy(data, memory + addr, 2);
  } else {
    printf("Memory load error at %d\n", addr);
    exit(1);
  }
}




