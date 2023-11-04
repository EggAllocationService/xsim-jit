#include "vreg_table.h"
#include <stdlib.h>
#include <string.h>
#include "jit.h"
#include "xis.h"

static int insert_reg(vreg_table *table, char reg) {
    for (int i = 0; i < 7; i++) {
        if(table->mapped[i] == reg) return 1;
        if (table->mapped[i] == -1) {
            table->mapped[i] = reg;
            return 1;
        }
    }
    return 0;
}

#define TRY_INSERT_REG(reg) if (!insert_reg(result, reg)) { \
                        return result;\ 
                    }

extern vreg_table *solve_instruction_region(unsigned char *program, unsigned short address) {
    vreg_table *result = malloc(sizeof(vreg_table));
    memset(result, -1, sizeof(vreg_table));
    result->mapped_regs = 0x0;

    int pc = 0;


    while (1) {
        unsigned short instruction = load_short(program, pc + address);
        unsigned char opcode = (instruction >> 8);
        pc += 2;

        if (opcode == 0 || opcode == I_RET) {
            break;
        }
        switch(XIS_NUM_OPS(opcode)) {
            case 1: { // single operand
                if (opcode != I_BR && opcode != I_JR) {
                    char reg1 = XIS_REG1(instruction);

                    TRY_INSERT_REG(reg1)
                }
                break;
            }
            case 2: { // 2 operand
                char reg1 = XIS_REG1(instruction);
                char reg2 = XIS_REG2(instruction);

                TRY_INSERT_REG(reg1)
                TRY_INSERT_REG(reg2)
            }
            case 3: { // extended
                pc += 2;
                if (opcode == I_LOADI) { 
                    char reg1 = XIS_REG1(instruction);

                    TRY_INSERT_REG(reg1)
                }
            }
        }

    }
    for (int i = 0; i < 7; i++) {
        if (result->mapped[i] != -1) {
            result->mapped_regs |= (1 << result->mapped[i]);
        }
    }

    return result;
}