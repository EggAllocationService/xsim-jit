cmake_minimum_required(VERSION 3.18)
project(xsim2 C)

set(CMAKE_C_STANDARD 99)

add_executable(xsim2 main.c xcpu.c xcpuprnt.c xmem.c xcpu.h xis.h xmem.h)
