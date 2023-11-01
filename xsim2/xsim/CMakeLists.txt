cmake_minimum_required(VERSION 3.18)
project(xsim2 C)

set(CMAKE_C_STANDARD 99)

add_library(xsimlib xmem.c xcpuprnt.c)
target_link_libraries(xsimlib xlib)
target_include_directories(xsimlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(xsim2 main.c xcpu.c)
target_link_libraries(xsim2 xlib xsimlib)