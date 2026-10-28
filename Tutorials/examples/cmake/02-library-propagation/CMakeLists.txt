# Demonstrates targets, libraries, and PRIVATE/PUBLIC/INTERFACE propagation.
cmake_minimum_required(VERSION 3.20)
project(propagation LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The library subproject defines the `mathx` target.
add_subdirectory(mathx)

# The application links the library. Because mathx marks its include dir PUBLIC,
# app automatically sees mathx/include without repeating the path here.
add_executable(app src/main.c)
target_link_libraries(app PRIVATE mathx)
target_compile_options(app PRIVATE -Wall -Wextra)
