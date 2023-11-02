cmake_minimum_required(VERSION 3.18)
project(XLinkedList C)

set(CMAKE_C_STANDARD 99)

add_library(liblinkedlist linkedlist.c)
target_include_directories(liblinkedlist PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
