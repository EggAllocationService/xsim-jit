
#define REX(w, reg, rm) dest[pos] = 0x40 | (w << 3) | ((reg & 8) >> 1) | (( rm & 8) >> 3);

/**
 * Encodes the x64 instruction for a 64 bit move from `reg1` to `reg2`
 * Returns the number of bytes written
*/
int x64_map_move_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    dest[pos] = REX(1, reg1, reg2); // REX prefix for x86_64 registers
    dest[pos + 1] = 0x89; // MOV r16 -> r/m16
    dest[pos + 2] = 0b11000000 | ((reg1 & 7) << 3) | (reg2 & 7); // encode registers
    return 3;
}

extern int x64_map_ret(unsigned char *dest, int pos) {
    dest[pos] = 0xc3; // RET
    return 1;
}

extern int x64_map_push(unsigned char *dest, int pos, unsigned char reg) {
    if (reg > 7) {
        dest[pos++] = REX(0, 0, reg); // REX prefix for x86_64 registers
    }
    dest[pos] = 0x50 | (reg & 7); // PUSH r64
    return 1 + (reg > 7);
}

extern int x64_map_pop(unsigned char *dest, int pos, unsigned char reg) {
    if (reg > 7) {
        dest[pos++] = REX(0, 0, reg); // REX prefix for x86_64 registers
    }
    dest[pos] = 0x58 | (reg & 7); // POP r64
    return 1 + (reg > 7);
}

extern int x64_map_move_imm2reg(unsigned char *dest, int pos, unsigned char reg, unsigned long imm) {
    dest[pos] = REX(1, 0, reg); // REX prefix for x86_64 registers, 64 bit operand size
    dest[pos + 1] = 0xB8 | (reg & 7); // MOV imm -> r

    // write the immediate value to the output in little-endian format
    for (int i = 0; i < 8; i++) {
        dest[pos + 2 + i] = (imm >> (i * 8)) & 0xFF;
    }
    return 10;
}

extern int x64_map_movzx_indirect2reg(unsigned char *dest, int pos, unsigned char reg, unsigned char base, unsigned int offset) {
    int initialpos = pos;
    dest[pos++] = REX(1, reg, base); // REX prefix for x86_64 registers
    dest[pos++] = 0x0F; // required prefix
    dest[pos++] = 0xB7; // MOVZX, r/m16 -> r64
    dest[pos++] = 0b10000000 | ((reg & 7) << 3) | (base & 7); // MODRM with 16 bit immediate  offset
    for (int i = 0; i < 4; i++) {
        dest[pos++] = (offset >> (i * 8)) & 0xFF;
    }
    return pos - initialpos;
}

extern int x64_map_mov_reg2indirect(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands 
    dest[pos++] = REX(0, source, target); // REX prefix for x86_64 registers
    dest[pos++] = 0x89; // MOV r/m16 -> r16
    dest[pos++] = 0b10000000 | ((source & 7) << 3) | (target & 7); // MODRM with 16 bit immediate  offset
    for (int i = 0; i < 4; i++) {
        dest[pos++] = (offset >> (i * 8)) & 0xFF;
    }
    return pos - initialpos;
}

extern int x64_map_mov_imm2indirect(unsigned char *dest, int pos, unsigned short value, unsigned char target, unsigned char offset) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (target > 7) {
        dest[pos++] = 0b01000001; // REX.B
    }
    dest[pos++] = 0xC7; // MOV imm16 -> r/m
    dest[pos++] = 0b01000000 | (target & 7); // encode base register
    dest[pos++] = offset;
    dest[pos++] = value & 0xFF;
    dest[pos++] = (value >> 8) & 0xFF;
    return pos - initialpos;
}

extern int x64_map_call(unsigned char *dest, int pos, unsigned char reg) {
    int initalpos = pos;
    if (reg > 7) {
        dest[pos++] = REX(0, 0, reg) // REX prefix for x86_64 registers
    }
    dest[pos++] = 0xFF;
    dest[pos++] = 0xD0 | (reg & 7); // CALL r64
    return pos - initalpos;
}


#define INDIRECT_OP_OFFSET_FROMREG(opcode)     int initialpos = pos; \
    dest[pos++] = 0x66; \
    dest[pos++] = REX(0, reg, target); \
    dest[pos++] = (opcode); \
    dest[pos++] = 0b01000000 | ((reg & 7) << 3) | (target & 7); \
    dest[pos++] = offset; \
    return pos - initialpos;

extern int x64_map_add_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x01);
}

extern int x64_map_add_reg2reg(unsigned char *dest, int pos, unsigned char rm, unsigned char reg) {
    int initalpos = pos;
    if (rm > 7 || reg > 7) {
        dest[pos++] = REX(1, reg, rm); // for x86_64 registers
    }
    dest[pos++] = 0x01; // ADD reg -> rm
    dest[pos++] = 0b11000000 | ((reg & 7) << 3) | (rm & 7);
    return pos - initalpos;
}

extern int x64_map_sub_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x29);
}

extern int x64_map_mul_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset) {
    int initalpos = pos;
    dest[pos++] = 0x66;
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xF7; // MUL r/m -> AX
    dest[pos++] = 0b01100000 | (rm & 7); // modrm, ext = 4
    dest[pos++] = offset;
    return pos - initalpos;
}

extern int x64_map_div_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset) {
    int initalpos = pos;
    dest[pos++] = 0x66; 
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xF7; // DIV r/m -> AX
    dest[pos++] = 0b01110000 | (rm & 7); // modrm, ext = 6
    dest[pos++] = offset;
    return pos - initalpos;
}

extern int x64_map_and_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x21);
}

extern int x64_map_or_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x09);
}

extern int x64_map_xor_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x31);
}

extern int x64_map_shr_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset) {
    int initalpos = pos;
    dest[pos++] = 0x66; 
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xD3; // SHR CX -> r/m 
    dest[pos++] = 0b01101000 | (rm & 7); // modrm, ext = 5
    dest[pos++] = offset;
    return pos - initalpos;
}

extern int x64_map_shl_indirect(unsigned char *dest, int pos, unsigned char rm, unsigned char offset) {
    int initalpos = pos;
    dest[pos++] = 0x66; 
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xD3; // SHL CX -> r/m 
    dest[pos++] = 0b01100000 | (rm & 7); // modrm, ext = 4
    dest[pos++] = offset;
    return pos - initalpos;
}

extern int x64_map_bswap_r16(unsigned char *dest, int pos, unsigned char reg) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (reg > 7) {
        dest[pos++] = REX(0, 0, reg);
    }
    dest[pos++] = 0x0F; // required prefix
    dest[pos++] = 0xC8 | (reg & 7); // BSWAP r16
    return pos - initialpos;
}