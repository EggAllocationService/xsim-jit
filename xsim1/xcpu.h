#ifndef XCPU_H
#define XCPU_H

#define X_MAX_REGS 16                 /* maximum size of the data stack */

/* CPU context struct */

#define X_STATE_COND_FLAG     0x0001 /* Last comparison result       */
#define X_STATE_DEBUG_ON      0x0002 /* CPU is is debug mode         */

#define X_STACK_REG           15     /* stack register */

typedef struct xcpu_context {        
  unsigned short regs[X_MAX_REGS];    /* general register file */
  unsigned short pc;                  /* program counter */
  unsigned short state;               /* state register */
} xcpu;


/* title: execute next instruction
 * param: pointer to CPU context
 * function: performs instruction indexed by the program counter 
 * returns: 1 if successful, 0 if not
 */
extern int xcpu_execute( xcpu *c ); 

/* title: print state of CPU and current instruction to be executed.
 * param: pointer to CPU context
 * function: outputs instruction (as encoded) and the state of all registers
 *           to the specified output file in the CPU context.
 * returns: void
 */
extern void xcpu_print( xcpu *c );

#endif
