#ifndef X_CPU_DEFS_H
#define X_CPU_DEFS_H

#define X_MAX_REGS 16                 /* maximum size of the data stack */

typedef struct xcpu_context {        
  unsigned short regs[X_MAX_REGS];    /* general register file */
  unsigned short pc;                  /* program counter */
  unsigned short state;               /* state register */
} xcpu;
#endif