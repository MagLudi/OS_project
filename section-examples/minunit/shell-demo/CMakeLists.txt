cmake_minimum_required(VERSION 3.10)
project(shell_demo C)

set(CMAKE_C_STANDARD 99)

add_executable(shell_demo main.c shell.c shell.h)

add_executable(runAllTests runAllTests.c minunit.h shell.c shell.h)