# Note

My `xsim2` is a little weird so I have an explainer of each folder below:

- `xsim` contains the files xcpu.c and main.c, what you're probably looking for
- `assembler` contains everything needed for the assembler
- `linkedlist` contains the `linkedlist.c` copied from my Assignment 3
- `jit` contains code related to the JIT compiler I wrote, which translates x-cpu machine code to x64 machine code at runtime

- `benchmark.xas` is a benchmark that calculates the fibbonaci series up to n=24 501 times, printing the series on the last run
- `benchmark_primes.xas` counts all the prime numbers up to 2^16 - 1

## JIT
The JIT compiler is quite straightforward:
1. Scan the section of x-cpu code to be translated to identify register usage
2. Load virtual registers into physical ones, specifically the x64 registers r8-r14 (r15 is reserved for the virtual memory pointer)
3. Translate instruction by instruction to equivalent x64 instructions
4. Patch the generated code to simplify jumps and branches

The main logic of the compiler is in `jit.c`, while everything related to generating x64 machine code is in `x64_codegen.c`. `jit_runtime.c` contains functions called by the generated code, and `linker.c` contains code used to simplify control flow.

Although I have about 6-10x more code than recommended for xsim2, on the Fibbonaci series benchmark the JIT compiler is about 42.68x faster than `xsim1`, and about 16.8x faster on the prime counting benchmark. 

If compiled with `CMAKE_BUILD_TYPE` set to `Debug`, functions generated at runtime will be dumped to `jit_func_<funcaddr>_<physaddress>.bin`.

Unfortunately, although I was able to kind of get the entire thing working through the JIT compiler using `ptrace` to implement debugging and max instruction counts, it was unstable. If `setd` is encountered, or a max instruction count is set, the JIT'd code will panic and fall back to the interpreter in `xcpu.c`.
