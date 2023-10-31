#include <stdio.h>
#include "jit.h"

int main() {
    printf("Hello, world!\n");
    jit_prepared_function *result = jit_prepare(NULL, 0, 100, NULL);
    // dump generated assembly to disk
    FILE *fp = fopen("jit_test.bin", "wb");
    fwrite(result->function, result->generated_size, 1, fp);
    fclose(fp);

    // execute generated code
    printf("Result(8, 13): %d\n", (unsigned short) result->function(8));
    return 0;
}