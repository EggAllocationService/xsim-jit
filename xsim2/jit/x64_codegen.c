
#define REXI(w, reg, index, rm) (0x40 | (w << 3) | ((reg & 8) >> 1) | ((index & 8) >> 2) | (( rm & 8) >> 3))
#define REX(w, reg, rm) REXI(w, reg, 0, rm)
#define MODRM(mode, reg, rm) ((mode << 6) | ((reg & 7) << 3) | (rm & 7))
#define SIB(scale, index, base) ((scale << 6) | ((index & 7) << 3) | (base & 7))


int x64_map_mov_reg2reg(unsigned char *dest, int pos, unsigned char reg1, unsigned char reg2) {
    dest[pos] = REX(1, reg1, reg2); // REX prefix for x86_64 registers
    dest[pos + 1] = 0x89; // MOV r16 -> r/m16
    dest[pos + 2] = MODRM(3, reg1, reg2); // encode registers
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

extern int x64_map_mov_indirect2reg64(unsigned char *dest, int pos, unsigned char target, unsigned char source, unsigned int offset) {
    dest[pos++] = REX(1, target, source); // REX prefix for x86_64 registers
    dest[pos++] = 0x8B; // MOV, r/m64 -> r64
    dest[pos++] = MODRM(0b10, target, source);
    for (int i = 0; i < 4; i++) {
        dest[pos++] = (offset >> (i * 8)) & 0xFF;
    }
    return 7;
}

extern int x64_map_mov_reg2indirect(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands 
    dest[pos++] = REX(0, source, target); // REX prefix for x86_64 registers
    dest[pos++] = 0x89; // MOV r16 -> r/m16
    dest[pos++] = MODRM(0b10, source, target); 
    for (int i = 0; i < 4; i++) {
        dest[pos++] = (offset >> (i * 8)) & 0xFF;
    }
    return pos - initialpos;
}

extern int x64_map_mov_reg2indirect64(unsigned char *dest, int pos, unsigned char source, unsigned char target, unsigned int offset) {
    int initialpos = pos;
    dest[pos++] = REX(1, source, target); // REX prefix for x86_64 registers
    dest[pos++] = 0x89; // MOV r64 -> r/m64
    dest[pos++] = MODRM(0b10, source, target); 
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
    dest[pos++] = MODRM(0b01, 0, target);
    dest[pos++] = offset;
    dest[pos++] = value & 0xFF;
    dest[pos++] = (value >> 8) & 0xFF;
    return pos - initialpos;
}

extern int x64_map_mov_m8_2_r8(unsigned char *dest, int pos, unsigned short source, unsigned char target) {
    // source is r/m, target is r
    dest[pos] = 0x8A; // MOV r/m8 -> r8
    dest[pos + 1] = MODRM(0b01, target, source);
    dest[pos + 2] = 0x0; // no offset
    return 3;
}
extern int x64_map_mov_r8_2_m8(unsigned char *dest, int pos, unsigned short source, unsigned char target) {
    // source is r, target is r/m
    dest[pos] = 0x8A; // MOV r/m8 -> r8
    dest[pos + 1] = MODRM(0b01, source, target);
    dest[pos + 2] = 0x0; // no offset
    return 3;
}

extern int x64_map_call(unsigned char *dest, int pos, unsigned char reg) {
    int initalpos = pos;
    if (reg > 7) {
        dest[pos++] = REX(0, 0, reg); // REX prefix for x86_64 registers
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

#define REG2REG_OP_16(opcode)     int initalpos = pos;\
    dest[pos++] = 0x66; \
    if (rm > 7 || reg > 7) { \
        dest[pos++] = REX(0, reg, rm); \
    } \
    dest[pos++] = opcode;  \
    dest[pos++] = MODRM(0b11, reg, rm); \
    return pos - initalpos;

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

extern int x64_map_sub_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x29)
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

extern int x64_map_mul_reg2ax16(unsigned char *dest, int pos, unsigned char rm) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xF7;
    dest[pos++] = MODRM(0b11, 4, rm);
    return pos - initialpos;
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
extern int x64_map_div_reg2ax16(unsigned char *dest, int pos, unsigned char rm) {
    int initialpos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xF7;
    dest[pos++] = MODRM(0b11, 6, rm);
    return pos - initialpos;
}

extern int x64_map_and_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x21)
}

extern int x64_map_and_imm16(unsigned char *dest, int pos, unsigned char rm,  unsigned short imm) {
    int start = pos;
    dest[pos++] = 0x66;
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0x81; // AND imm16->r16
    dest[pos++] = MODRM(0b11, 4, rm);
    dest[pos++] = imm & 0xFF;
    dest[pos++] = (imm >> 8)  & 0xFF;
    return pos - start;
}

extern int x64_map_or_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x09);
}
extern int x64_map_or_imm16(unsigned char *dest, int pos, unsigned char rm,  unsigned short imm) {
    int start = pos;
    dest[pos++] = 0x66;
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0x81; // AND imm16->r16
    dest[pos++] = MODRM(0b11, 1, rm);
    dest[pos++] = imm & 0xFF;
    dest[pos++] = (imm >> 8)  & 0xFF;
    return pos - start;
}

extern int x64_map_xor_indirect(unsigned char *dest, int pos, unsigned char reg, unsigned char target, unsigned char offset) {
    INDIRECT_OP_OFFSET_FROMREG(0x31);
}

extern int x64_map_xor_reg2reg64(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x31)
}

extern int x64_map_shr_reg2reg16(unsigned char *dest, int pos, unsigned char rm) {
    int initalpos = pos;
    dest[pos++] = 0x66; 
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xD3; // SHL/R CX -> r/m 
    dest[pos++] = MODRM(0b11, 5, rm);
    return pos - initalpos;
}

extern int x64_map_shl_reg2reg16(unsigned char *dest, int pos, unsigned char rm) {
    int initalpos = pos;
    dest[pos++] = 0x66; 
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xD3; // SHL/R CX -> r/m 
    dest[pos++] = MODRM(0b11, 4, rm);
    return pos - initalpos;
}

extern int x64_map_xchg_r8(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    dest[pos] = 0x86;
    dest[pos + 1] = 0b11000000 | ((reg & 7) << 3) | (rm & 7);
    return 2;
}

extern int x64_map_absolute_jmp(unsigned char *dest, int pos, unsigned char reg) {
    int initial_pos = pos;
    if(reg > 7) {
        dest[pos++] = REX(0, 0, reg);
    }
    dest[pos++] = 0xFF;
    dest[pos++] = 0b11100000 | (reg & 7); // ModRM, reg = 4

    return pos - initial_pos;
}

extern int x64_map_neg_reg16(unsigned char *dest, int pos, unsigned char target) {
    dest[pos++] = 0x66;
    if (target > 7) {
        dest[pos++] = REX(0, 0, target);
    }
    dest[pos++] = 0xF7;
    dest[pos++] = 0b11011000 | (target & 7);
    return target > 7 ? 4 : 3;
}


extern int x64_map_inc_dec_indirect(unsigned char *dest, int pos, unsigned char mode, unsigned char rm, unsigned char offset) {
    int initial_pos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xFF; // inc/dec opcode
    dest[pos++] = 0b01000000 | (mode << 3) | (rm & 7);
    dest[pos++] = offset;
    return pos - initial_pos;
}

extern int x64_map_jz_rel8(unsigned char *dest, int pos, char offset) {
    dest[pos] = 0x74;
    dest[pos + 1] = offset;
    return 2;
}

extern int x64_map_jmp_rel32(unsigned char *dest, int pos, int offset) {
    dest[pos] = 0xE9;
    for (int i = 0; i < 4; i++) {
        dest[pos + 1 + i] = (offset >> (i * 8)) & 0xFF;
    }

    return 5;
}

extern int x64_map_jnz_rel32(unsigned char *dest, int pos, int offset) {
    dest[pos] = 0x0F;
    dest[pos + 1] = 0x85;
    for (int i = 0; i < 4; i++) {
        dest[pos + 2 + i] = (offset >> (i * 8)) & 0xFF;
    }

    return 6;
}


/**
 * Stuff for conditionals
*/
extern int x64_map_cmp_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x3B)
}

extern int x64_map_test_reg2reg16(unsigned char *dest, int pos, unsigned char reg, unsigned char rm) {
    REG2REG_OP_16(0x85)
}

extern int x64_map_setz(unsigned char *dest, int pos, unsigned char rm) {
    dest[pos] = 0x0F; // required prefix
    dest[pos + 1] = 0x94; // SETZ rm/8
    dest[pos + 2] = 0b11000000 | (rm & 7);
    return 3;
}

extern int x64_map_setb(unsigned char *dest, int pos, unsigned char rm) {
    dest[pos] = 0x0F; // required prefix
    dest[pos + 1] = 0x92; // SETB rm/8
    dest[pos + 2] = 0b11000000 | (rm & 7);
    return 3;
}

extern int x64_map_setne(unsigned char *dest, int pos, unsigned char rm) {
    dest[pos] = 0x0F; // required prefix
    dest[pos + 1] = 0x95; // SETNE rm/8
    dest[pos + 2] = 0b11000000 | (rm & 7);
    return 3;
}

extern int x64_map_mov_8_16_scaled2reg(unsigned char *dest, int pos, unsigned char reg,
                                       unsigned char base, unsigned char index, char mode) {
    int startpos = pos;
    if (mode == 1) { // 16 bit load
        dest[pos++] = 0x66; // 16 bit operation
    }
    dest[pos++] = REXI(0, reg, index, base);
    dest[pos++] = 0x0F; // required prefix
    dest[pos++] = mode == 1 ? 0xB7 : 0xB6; // 0x8B = movzx r/m16->r16, 0x8A = movzx r/m8->r16
    dest[pos++] = MODRM(0b01, reg, 4); // use SIB addressing w/ 8 bit offset
    dest[pos++] = SIB(0b00, index, base); // scale of 1 byte
    dest[pos++] = 0x0; // offset
    return pos - startpos;
}


extern int x64_map_mov_8_16_reg2scaled(unsigned char *dest, int pos, unsigned char reg,
                                       unsigned char base, unsigned char index, char mode) {
    int startpos = pos;
    if (mode == 1) { // 16 bit load
        dest[pos++] = 0x66; // 16 bit operation
    }
    dest[pos++] = REXI(0, reg, index, base);
    dest[pos++] = mode == 1 ? 0x89 : 0x88; // 0x89 = mov r16->r/m16, 0x88 = mov r8->r/m8
    dest[pos++] = MODRM(0b01, reg, 4); // use SIB addressing w/ 8 bit offset
    dest[pos++] = SIB(0b00, index, base); // scale of 1 byte
    dest[pos++] = 0x0; // offset
    return pos - startpos;
}

extern int x64_map_ror_rm16_imm8(unsigned char *dest, int pos, unsigned char rm, char amount) {
    int startpos = pos;
    dest[pos++] = 0x66;
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xC1; // ROR r/m16, imm8
    dest[pos++] = MODRM(0b11, 1, rm);
    dest[pos++] = amount;
    return pos - startpos;
}

extern int x64_map_inc_dec_reg16(unsigned char *dest, int pos, unsigned char mode, unsigned char rm) {
    int initial_pos = pos;
    dest[pos++] = 0x66; // 16 bit operands
    if (rm > 7) {
        dest[pos++] = REX(0, 0, rm);
    }
    dest[pos++] = 0xFF; // inc/dec opcode
    dest[pos++] = MODRM(0b11, mode, rm);
    return pos - initial_pos;
}