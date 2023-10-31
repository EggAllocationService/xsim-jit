/**
 * Encodes the x64 instruction for a 16 bit move from `reg1` to `reg2`
*/
extern int x64_map_move_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);

/**
 * Encodes the x64 `ret` instruction
*/
extern int x64_map_ret(unsigned char *dest, int pos);

/**
 * Encodes the x64 `IMUL` instruction for 16 bit operands
 * Returns the number of bytes written
 * Operation performed is reg1 = reg1 * reg2
*/
extern int x64_map_imul(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2);