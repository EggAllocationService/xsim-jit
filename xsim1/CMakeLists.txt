cmake_minimum_required(VERSION 3.18)
project(xsim1 C)

set(CMAKE_C_STANDARD 99)

add_executable(xsim1
        main.c
        xcpuprnt.c
        xmem.c
        xcpu.o
        )
