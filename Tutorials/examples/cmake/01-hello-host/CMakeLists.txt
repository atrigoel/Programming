# The smallest useful CMake project: one target, one source file.
cmake_minimum_required(VERSION 3.20)

# Declare the project name and the languages it uses.
project(hello VERSION 1.0.0 LANGUAGES C)

# Produce an executable "target" named `hello` from main.c.
add_executable(hello main.c)

# Attach warnings to THIS target only (modern, scoped style).
target_compile_options(hello PRIVATE -Wall -Wextra)
