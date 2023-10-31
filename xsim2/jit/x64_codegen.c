/**
 * Encodes the x64 instruction for a 16 bit move from `reg1` to `reg2`
 * Returns the number of bytes written
*/
int x64_map_move_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    dest[pos] = 0x66; // 16 bit operand size, 64 bit addressing
    dest[pos + 1] = 0x40 | ((reg1 & 8) >> 1) | ((reg2 & 8) >> 3); // REX prefix for x86_64 registers
    dest[pos + 2] = 0x89; // MOV r16 -> r/m16
    dest[pos + 3] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return 4;
}

extern int x64_map_ret(unsigned char *dest, int pos) {
    dest[pos] = 0xc3; // RET
    return 1;
}

extern int x64_map_imul(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    dest[pos] = 0x66; // 16 bit operand size, 64 bit addressing
    dest[pos + 1] = 0x40 | ((reg1 & 8) >> 1) | ((reg2 & 8) >> 3); // REX prefix for x86_64 registers
    dest[pos + 2] = 0x0F; // required prefix
    dest[pos + 3] = 0xAF; // IMUL, two registers
    dest[pos + 4] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return 5;
}