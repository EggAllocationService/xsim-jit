#include <stdio.h>

unsigned short fib(unsigned short n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) +fib(n - 2);
}

void calcSeries(char print) {
    for (unsigned short i = 0; i < 25; i++) {
        unsigned short result = fib(i);
        if (print) {
            printf("%d\n", result);
        }
    }
}


int main() {
    for (int i = 0; i < 500; i++) {
        calcSeries(0);
    }
    calcSeries(1);
}


