cmake_minimum_required(VERSION 3.18)
project(jit_tester C)

set(CMAKE_C_STANDARD 99)

add_executable(testjit main.c)

target_link_libraries(testjit xjit)