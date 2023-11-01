
/**
 * Encodes the x64 instruction for a 64 bit move from `reg1` to `reg2`
 * Returns the number of bytes written
*/
int x64_map_move_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    dest[pos] = 0b01001000 | ((reg1 & 8) >> 1) | ((reg2 & 8) >> 3); // REX prefix for x86_64 registers
    dest[pos + 1] = 0x89; // MOV r16 -> r/m16
    dest[pos + 2] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return 3;
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

extern int x64_map_push(unsigned char *dest, int pos, unsigned char reg) {
    if (reg > 7) {
        dest[pos++] = 0x40 | ((reg & 8) >> 3); // REX prefix for x86_64 registers
    }
    dest[pos] = 0x50 | (reg & 7); // PUSH r64
    return 1 + (reg > 7);
}

extern int x64_map_pop(unsigned char *dest, int pos, unsigned char reg) {
    if (reg > 7) {
        dest[pos++] = 0x40 | ((reg & 8) >> 3); // REX prefix for x86_64 registers
    }
    dest[pos] = 0x58 | (reg & 7); // POP r64
    return 1 + (reg > 7);
}

extern int x64_map_move_imm2reg(unsigned char *dest, int pos, unsigned char reg, unsigned long imm) {
    dest[pos] = 0b01001000 | ((reg & 8) >> 3); // REX prefix for x86_64 registers, 64 bit operand size
    dest[pos + 1] = 0xB8 | (reg & 7); // MOV imm -> r

    // write the immediate value to the output in little-endian format
    for (int i = 0; i < 8; i++) {
        dest[pos + 2 + i] = (imm >> (i * 8)) & 0xFF;
    }
    return 10;
}

extern int x64_map_movzx_indirect2reg(unsigned char *dest, int pos, unsigned char reg, unsigned char base, unsigned char offset) {
    int initialpos = pos;
    dest[pos++] = 0b01001000 | ((reg & 8) >> 1) | ((base & 8) >> 3); // REX prefix for x86_64 registers
    dest[pos++] = 0x0F; // required prefix
    dest[pos++] = 0xB7; // MOVZX, r/m16 -> r64
    dest[pos++] = 0b01000000 | ((reg & 7) << 3) | (base & 7); // MODRM with 8 bit immediate  offset
    dest[pos++] = offset;
    return pos - initialpos;
}

extern int x64_map_mov_reg2indirect(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned char offset) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands 
    dest[pos++] = 0b01000000 | ((source & 8) >> 1) | ((target & 8) >> 3); // REX prefix for x86_64 registers
    dest[pos++] = 0x89; // MOV r/m16 -> r16
    dest[pos++] = 0b01000000 | ((source & 7) << 3) | (target & 7); // MODRM with 8 bit immediate  offset
    dest[pos++] = offset;
    return pos - initialpos;
}

extern int x64_map_call(unsigned char *dest, int pos, unsigned char reg) {
    int initalpos = pos;
    if (reg > 7) {
        dest[pos++] = 0x40 | ((reg & 8) >> 3); // REX prefix for x86_64 registers
    }
    dest[pos++] = 0xFF;
    dest[pos++] = 0xD0 | (reg & 7); // CALL r64
    return pos - initalpos;
}

extern int x64_map_add(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    int intitialpos = pos;
    dest[pos] = 0x66;
    if (reg1 > 7 || reg2 > 7) {
        dest[pos++] = 0x40 | ((reg1 & 8) >> 1) | ((reg2 & 8) >> 3); // REX prefix for x86_64 registers
    }
    dest[pos++] = 0x03; // ADD r/m16 -> r16
    dest[pos++] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return pos - intitialpos;
}

extern int x64_map_sub(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    int intitialpos = pos;
    dest[pos] = 0x66;
    if (reg1 > 7 || reg2 > 7) {
        dest[pos++] = 0x40 | ((reg1 & 8) >> 1) | ((reg2 & 8) >> 3); // REX prefix for x86_64 registers
    }
    dest[pos++] = 0x2B; // SUB r/m16 -> r16
    dest[pos++] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return pos - intitialpos;
}
