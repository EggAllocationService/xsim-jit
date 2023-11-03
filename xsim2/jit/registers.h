//
// Created by kyle on 11/1/23.
//
#pragma once
#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

#define AL 0
#define AH 4
#define BL 1
#define BH 5

// register used for the address of function calls
#define FUNC_ADDR_REG RAX

// registers that don't do anything funky with indirect addressing
#define SCRATCH_REG RCX
#define SCRATCH_REG_2 RBX
#define CPU_STATE_REG RDI
#define VMEM_BASE_REG RSI

#define PC_INDEX 16
#define FLAGS_INDEX 17