#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xcpu.h"
#include "xis.h"
#include "xmem.h"

/**
 *
 */

/* Use
 *   xcpu_print( c );
 * to print cpu state, where c is of type xcpu *
 * See xcpu.h for full prototype of xcpu_print()
 */
#define XIS_TWO_REG_INT_OP(x, y) case x: c->regs[dest] = c->regs[dest] y c->regs[src]; break;
extern int xcpu_execute(xcpu *c) {
  unsigned short instruction;
  xmem_load(c->pc, (unsigned char *)&instruction);

  unsigned char opcode = instruction >> 8;

  // two register operands
  if (XIS_NUM_OPS(instruction) == 2) {
    size_t src = XIS_REG2(instruction);
    size_t dest = XIS_REG1(instruction);

    switch (opcode) {
    XIS_TWO_REG_INT_OP(I_ADD, +);
    XIS_TWO_REG_INT_OP(I_SUB, -);
    XIS_TWO_REG_INT_OP(I_MUL, *);
    XIS_TWO_REG_INT_OP(I_DIV, /);
    XIS_TWO_REG_INT_OP(I_AND, &);
    XIS_TWO_REG_INT_OP(I_OR, |);
    XIS_TWO_REG_INT_OP(I_XOR, ^);
    XIS_TWO_REG_INT_OP(I_SHR, >>);
    XIS_TWO_REG_INT_OP(I_SHL, <<);
    case I_TEST:
      if ((c->regs[src] & c->regs[dest]) != 0) {
        c->state |= 1;
      } else {
        c->state &= ~1;
      }
      break;
    case I_CMP:
      if (c->regs[src] < c->regs[dest]) {
        c->state |= 1;
      } else {
        c->state &= ~1;
      }
      break;
    case I_EQU:
      if (c->regs[src] == c->regs[dest]) {
        c->state |= 1;
      } else {
        c->state &= ~1;
      }
      break;
    case I_MOV:
      c->regs[dest] = c->regs[src];
      break;
    case I_LOAD:
      xmem_load(c->regs[src], (unsigned char*) (c->regs + dest));
      break;
    case I_STOR:
      xmem_store((unsigned char*) (c->regs + src), c->regs[dest]);
      break;
    case I_LOADB:
      
    }
  }

  return 0;
}
