long test_push(long val) {
    long val2;
    asm("mov $0xABCDEF1234567890, %r10;" "push %r10;");
    asm("pop %0" : "=r" (val2));
    return val2;
}