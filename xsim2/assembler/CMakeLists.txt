cmake_minimum_required(VERSION 3.18)
project(XAssembler C)

set(CMAKE_C_STANDARD 99)

add_library(xlib INTERFACE)
target_include_directories(xlib INTERFACE .)

add_executable(xas xas.c xreloc.c)