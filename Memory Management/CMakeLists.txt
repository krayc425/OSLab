cmake_minimum_required(VERSION 3.7)
project(Memory_Management)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        bottom.c
        bottom.h
        call.c
        call.h
        page.c
        page.h
        process.c
        process.h
        test.c
        type.h)

add_executable(Memory_Management ${SOURCE_FILES})