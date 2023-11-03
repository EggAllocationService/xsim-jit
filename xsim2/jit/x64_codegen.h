#pragma once


/**
 * Encodes the x64 `ret` instruction
*/
extern int x64_map_ret(unsigned char *dest, int pos);



/**
 * Encodes the x64 `PUSH` instruction for a 64 bit register
 * Returns the number of bytes written
*/
extern int x64_map_push(unsigned char *dest, int pos, unsigned char reg);

/**
 * Encodes the x64 `POP` instruction targeting a 64 bit register
 * Returns the number of bytes written
*/
extern int x64_map_pop(unsigned char *dest, int pos, unsigned char reg);

/**
 * Encodes the x64 `MOV` instruction, moving an immediate unsigned 16 bit value into a register
 * Returns the number of bytes written
*/
extern int x64_map_move_imm2reg(unsigned char *dest, int pos, unsigned char reg, unsigned long imm);

/**
 * Encodes a System V ABI compliant function call
 * Before encoding this, ensure the arguments are moved into the appropriate registers
 * All registers other than RBX, RSP, RBP, and R12-R15 may be trampled, save if you need to!
 * `reg` is the register containing the fuction address
*/
extern int x64_map_call(unsigned char *dest, int pos, unsigned char reg);

/*
 *************************************
  Lots of MOV instruction variants
 *************************************
*/

/**
 * Encodes the x64 instruction for a 64 bit move from `reg1` to `reg2`
 * Returns the number of bytes written
*/
extern int x64_map_mov_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);

/**
 * Encodes an x64 MOVZX instruction with a indirect source with offset, and a register target.
 * 64 bit addressing, 16 bit operands
 * `source` is the base register for the indirect source
 * `offset` is the offset from the base register
 * `target` is where the zero-extended 16 bit word will be placed
*/
extern int x64_map_movzx_indirect2reg(unsigned char *dest, int pos, unsigned char target, unsigned char source, unsigned int offset);

/**
 * Encodes an x64 MOV instruction targeting an indirect register with 32 bit offset, with a register source
 * 64 bit addressing, 16 bit operands
*/
extern int x64_map_mov_reg2indirect(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset);

/**
 * Encodes an x64 MOV instruction targeting an indirect register with 32 bit offset, with a register source
 * 64 bit addressing, 64 bit operands
*/
extern int x64_map_mov_reg2indirect64(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset);

/**
 * Encodes an x64 MOV instruction targeting an indirect register with 32 bit offset, with a register source
 * 64 bit addressing, 64 bit operands
*/
extern int x64_map_mov_indirect2reg64(unsigned char *dest, int pos, unsigned char target, unsigned char source, unsigned int offset);

/**
 * Encodes an x64 MOV instruction moving an immediate value to an indirect register with an 8 bit offset
*/
extern int x64_map_mov_imm2indirect(unsigned char *dest, int pos, unsigned short value, unsigned char target, unsigned char offset);

/**
 * Encodes an x64 MOV instruction indirectly loading one byte from a register into an 8 bit register
 * WARNING: This operation does not support extended registers for either operand! Bad things will happen!
 */
extern int x64_map_mov_m8_2_r8(unsigned char *dest, int pos, unsigned short source, unsigned char target);

/**
 * Encodes an x64 MOV instruction storing one byte from an 8 bit register indirectly into another register
 * WARNING: This operation does not support extended registers for either operand! Bad things will happen!
 */
extern int x64_map_mov_r8_2_m8(unsigned char *dest, int pos, unsigned short source, unsigned char target);

/*
  Integer operations
*/

/**
 * Encodes the x64 `ADD` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] += reg;
*/
extern int x64_map_add_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `ADD` instruction for 64 bit operands
 * This variant takes two register operands
 * Operation performed is rm = rm + reg
*/
extern int x64_map_add_reg2reg(unsigned char *dest, int pos, unsigned char rm, unsigned char reg);

/**
 * Encodes the x64 `SUB` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] -= reg;
*/
extern int x64_map_sub_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `SUB` instruction for two 16 bit register operands
 * Returns the number of bytes written
 * Operation performed is rm = rm - reg
*/
extern int x64_map_sub_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);

/**
 * Encodes the x64 `MUL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is ax *= [rm + offset];
*/
extern int x64_map_mul_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset);

/**
 * Encodes the x64 `MUL` instruction for two 16 bit registers
 * Returns the number of bytes written
 * Operation performed is ax *= rm;
*/
extern int x64_map_mul_reg2ax16(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `DIV` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is ax /= [rm + offset];
*/
extern int x64_map_div_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset);

/**
 * Encodse the x65 DIV instruction for 16 bit operands
 * returns the number of bytes written
 * Operation performed is ax /= rm
*/
extern int x64_map_div_reg2ax16(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `AND` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] &= reg;
*/
extern int x64_map_and_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);

/**
 * Encodes the x64 `AND` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is target &= IMM
*/
extern int x64_map_and_imm16(unsigned char *dest, int pos, unsigned char rm,  unsigned short imm);

/**
 * Encodes the x64 `OR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is rm |= reg;
*/
extern int x64_map_or_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) ;

/**
 * Encodes the x64 `OR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is target |= IMM
*/
extern int x64_map_or_imm16(unsigned char *dest, int pos, unsigned char rm,  unsigned short imm);

/**
 * Encodes the x64 `XOR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] ^= reg;
*/
extern int x64_map_xor_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `XOR` instruction for two 64 bit register operands
 * Returns the number of bytes written
 * Operation performed is rm = rm ^ reg
*/
extern int x64_map_xor_reg2reg64(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);

/**
 * Encodes the x64 `SHR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is rm = rm >> %cx;
*/
extern int x64_map_shr_reg2reg16(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `SHL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is rm = rm << %cx
*/
extern int x64_map_shl_reg2reg16(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `NEG` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] = ~[target + offset];
 */
extern int x64_map_neg_indirect(unsigned char *dest, int pos, unsigned char target, unsigned char offset);

/*
  Some utility instructions
*/

/**
 * Encodes the x64 `XCHG` instruction (8 bit word)
 * `reg` is one of the 8-bit registers that will be swapped
 * `rm` is the other 8-bit target register
*/
extern int x64_map_xchg_r8(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);


/*
 Control flow
*/

/**
 * Encodes the x64 JMP instruction with a 64 bit register operand
 * @param reg Register containing the address to jump to
 * @return number of instructions written
 */
extern int x64_map_absolute_jmp(unsigned char *dest, int pos, unsigned char reg);


#define MODE_DEC 1
#define MODE_INC 0
/**
 * Encodes the x64 INC and DEC instructions, indirectly targeting a 16 bit value
 * @return number of bytes written
*/
extern int x64_map_inc_dec_indirect(unsigned char *dest, int pos, unsigned char mode, unsigned char rm, unsigned char offset);

/**
* Encodes the x64 JZ/E instruction, with an 8 bit 2's compliment offset
*/
extern int x64_map_jz_rel8(unsigned char *dest, int pos, char offset);

/**
* Encodes the x64 JMP instruction, with an 32 bit 2's compliment offset
*/
extern int x64_map_jmp_rel32(unsigned char *dest, int pos, int offset);

/**
 * Contitional statements
*/
/*
  Encodes the x64 CMP instruction, setting B if rm < reg
  Returns the number of bytes written
*/
extern int x64_map_cmp_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);

/**
 * Encodes the x64 `SETZ/E` instruction, setting `rm` to the value of the zero flag
 * Do NOT encode this instruction with any of the new registers, BAD things will happen.
 */
extern int x64_map_setz(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `SETB/NAE/C` instruction, setting `rm` to the value of the below flag
 * Do NOT encode this instruction with any of the new registers, BAD things will happen.
 */
extern int x64_map_setb(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 `SETNE` instruction, setting `rm` to the value of the below flag
 * Do NOT encode this instruction with any of the new registers, BAD things will happen.
 */
extern int x64_map_setne(unsigned char *dest, int pos, unsigned char rm);

/**
 * Encodes the x64 TEST instruction, with two 16 bit operands
 * Returns the number of bytes written
*/
extern int x64_map_test_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm);