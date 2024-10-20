cmake_minimum_required(VERSION 3.26)
project(sudoku C)

set(CMAKE_C_STANDARD 11)

# 创建可执行文件 sudoku
add_executable(sudoku main.c cnf_library.c
        dpll_solver.h
        dpll_solver.c)

set(CMAKE_C_FLAGS "-g -O0 -Wall")