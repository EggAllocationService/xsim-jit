cmake_minimum_required(VERSION 3.18)
project(Assignment4)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(xsim)
add_subdirectory(assembler)
add_subdirectory(jit)
add_subdirectory(jit_test)