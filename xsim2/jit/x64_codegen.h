#pragma once
/**
 * Encodes the x64 instruction for a 16 bit move from `reg1` to `reg2`
*/
extern int x64_map_move_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);

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

/**
 * Encodes an x64 MOVZX instruction with a indirect source with offset, and a register target.
 * 64 bit addressing, 16 bit operands
 * `source` is the base register for the indirect source
 * `offset` is the offset from the base register
 * `target` is where the zero-extended 16 bit word will be placed
*/
extern int x64_map_movzx_indirect2reg(unsigned char *dest, int pos, unsigned char target, unsigned char source, unsigned int offset);

/**
 * Encodes an x64 MOV instruction targeting an indirect register with 8 bit offset, with a register source
 * 64 bit addressing, 16 bit operands
*/
extern int x64_map_mov_reg2indirect(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset);

/**
 * Encodes an x64 MOV instruction moving an immediate value to an indirect register with an 8 bit offset
*/
extern int x64_map_mov_imm2indirect(unsigned char *dest, int pos, unsigned short value, unsigned char target, unsigned char offset);

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
 * Encodes the x64 `MUL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is ax *= [rm + offset];
*/
extern int x64_map_mul_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset);

/**
 * Encodes the x64 `DIV` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is ax /= [rm + offset];
*/
extern int x64_map_div_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset);

/**
 * Encodes the x64 `AND` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] &= reg;
*/
extern int x64_map_and_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `OR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] |= reg;
*/
extern int x64_map_or_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `XOR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] ^= reg;
*/
extern int x64_map_xor_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `SHR` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] >> %cx;
*/
extern int x64_map_shr_indirect(unsigned char *dest, int pos, unsigned char target, unsigned char offset);

/**
 * Encodes the x64 `SHL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is [target + offset] << %cx;
*/
extern int x64_map_shl_indirect(unsigned char *dest, int pos, unsigned char target, unsigned char offset);

/*
  Some utility instructions
*/

/**
 * Encodes the x64 `BSWAP` instruction (16 bit word)
 * `reg` is the 16 bit register that will have it's bytes swapped
*/
extern int x64_map_bswap_r16(unsigned char *dest, int pos, unsigned char reg);

