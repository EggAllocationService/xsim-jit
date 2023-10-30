cmake_minimum_required(VERSION 3.18)
project(Assignment4)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(xsim)
add_subdirectory(assembler)

add_custom_target(BuildAll)
add_dependencies(BuildAll xsim2 xasm)