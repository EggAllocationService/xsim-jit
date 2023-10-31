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
 * Integer operations
*/

/**
 * Encodes the x64 `ADD` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is reg1 = reg1 + reg2
*/
extern int x64_map_add(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);

/**
 * Encodes the x64 `SUB` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is reg1 = reg1 - reg2
*/
extern int x64_map_sub(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);

/**
 * Encodes the x64 `IMUL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is reg1 = reg1 * reg2
*/
extern int x64_map_imul(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);
