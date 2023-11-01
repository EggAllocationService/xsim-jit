#pragma once
/*
    These functions may be called by generated code, usually because it was simpler to emit a function call
        than these functions directly

    It's worse for performance, but frankly I don't care because I'm not going to implement another eight variants of MOV
*/

/**
 * Sets the lowest bit of the cpu flags to (src & dest) == 0
*/
extern void calc_flags_test(unsigned short src, unsigned short dest, xcpu *c);

/**
 * Sets the lowest bit of the cpu flags to src < dest
*/
extern void calc_flags_cmp(unsigned short src, unsigned short dest, xcpu *c);

/**
 * Sets the lowest bit of the cpu flags to src == dest
*/
extern void calc_flags_equ(unsigned short src, unsigned short dest, xcpu *c);