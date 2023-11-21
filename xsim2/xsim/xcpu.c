#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xcpu.h"
#include "xis.h"
#include "xmem.h"

/**
 * Macro for generating a switch case applying an integer operation to the two registers
*/
#define XIS_TWO_REG_INT_OP(x, y) case x: c->regs[dest] = c->regs[dest] y c->regs[src]; break;

static unsigned short load_short(unsigned short address) {
  unsigned short val;
  unsigned char tmp_data[2];
  xmem_load(address, tmp_data);
  val = tmp_data[0];
  val = (val << 8) | tmp_data[1];
  return val;
}
static void write_short(unsigned short value, unsigned short address) {
  unsigned char tmp_data[2];
  tmp_data[0] = value >> 8;
  tmp_data[1] = value & 0xFF;
  xmem_store(tmp_data, address);
}

/* Use
 *   xcpu_print( c );
 * to print cpu state, where c is of type xcpu *
 * See xcpu.h for full prototype of xcpu_print()
 */
extern int xcpu_execute(xcpu *c) {
  unsigned short instruction = load_short(c->pc);
  unsigned char opcode = instruction >> 8;

  unsigned char is_invalid = 0;
  c->pc += 2;
  
  // two register operands
  if (XIS_NUM_OPS(opcode) == 2) {
    size_t src = XIS_REG1(instruction);
    size_t dest = XIS_REG2(instruction);

    switch (opcode) {
      XIS_TWO_REG_INT_OP(I_ADD, +)
      XIS_TWO_REG_INT_OP(I_SUB, -)
      XIS_TWO_REG_INT_OP(I_MUL, *)
      XIS_TWO_REG_INT_OP(I_DIV, /)
      XIS_TWO_REG_INT_OP(I_AND, &)
      XIS_TWO_REG_INT_OP(I_OR, |)
      XIS_TWO_REG_INT_OP(I_XOR, ^)
      XIS_TWO_REG_INT_OP(I_SHR, >>)
      XIS_TWO_REG_INT_OP(I_SHL, <<)
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
        c->regs[dest] = load_short(c->regs[src]);
        break;
      case I_STOR:
        write_short(c->regs[src], c->regs[dest]);
        break;
      case I_LOADB: {
        unsigned char data[2];
        xmem_load(c->regs[src], data);
        c->regs[dest] = data[0];
        break;
      }
      case I_STORB: {
        unsigned char data[2];
        xmem_load(c->regs[dest], data);
        data[0] = (unsigned char) (c->regs[src] & 0xFF);
        xmem_store(data, c->regs[dest]);
        break;
      }
      default: 
        is_invalid = 1;
        break;
    }
  } else if (XIS_NUM_OPS(opcode) == 0) {
    // simple instructions
    switch (opcode) {
      case I_RET:
        c->pc = load_short(c->regs[15]);
        c->regs[15] +=2;
        break;
      case I_CLD:
        c->state &= ~2;
        break;
      case I_STD:
        c->state |= 2;
        break;
      default:
        is_invalid = 1;
        break;
    }
  } else if (XIS_NUM_OPS(opcode) == 1) {
    // one operarand instructions
    size_t reg = XIS_REG1(instruction);
    switch (opcode) {
      case I_NEG:
        c->regs[reg] = -c->regs[reg];
        break;
      case I_NOT:
        c->regs[reg] = !c->regs[reg];
        break;
      case I_INC:
        c->regs[reg]++;
        break;
      case I_DEC:
        c->regs[reg]--;
        break;
      case I_PUSH:
        c->regs[15] -= 2;
        write_short(c->regs[reg], c->regs[15]);
        break;
      case I_POP:
        c->regs[reg] = load_short(c->regs[15]);
        c->regs[15] += 2;
        break;
      case I_JMPR:
        c->pc = c->regs[reg];
        break;
      case I_CALLR:
        c->regs[15] -= 2;
        write_short(c->pc, c->regs[15]);
        c->pc = c->regs[reg];
        break;
      case I_OUT:
        printf("%c", (char) c->regs[reg]);
        break;

      // immediate mode instructions
      case I_BR:
        if ((c->state & 1) == 1) {
          c->pc -= 2;
          c->pc += (char) (instruction & 0xFF);
        }
        break;
      case I_JR:
        c->pc -= 2;
        c->pc += (char) (instruction & 0xFF);
        break;

      default:
        is_invalid = 1;
        break;
    } 
  } else { // extended instructions
    
    unsigned short extended_value = load_short(c->pc);
    switch (opcode) {
      case I_JMP:
        c->pc = extended_value;
        break;
      case I_CALL:
        c->regs[15] -= 2;
        write_short(c->pc + 2, c->regs[15]);
        c->pc = extended_value;
        break;
      case I_LOADI:
        c->pc += 2;
        c->regs[XIS_REG1(instruction)] = extended_value;
        break;
      default:
        is_invalid = 1;
        break;
    }
  }

  if (is_invalid) return 0;
  if (c->state & 2) { // debug bit set
    xcpu_print(c);
  }
  return 1;
}
