cmake_minimum_required(VERSION 3.18)
project(XJIT C)

set(CMAKE_C_STANDARD 99)

add_library(xjit jit.c x64_codegen.c jit_runtime.c linker.c vreg_table.c)

target_link_libraries(xjit xlib liblinkedlist)
target_include_directories(xjit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})