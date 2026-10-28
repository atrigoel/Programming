# Builds a static library and exposes its headers to consumers.
add_library(mathx STATIC
    src/mathx.c
)

# PUBLIC: needed to build mathx AND propagated to anything that links mathx.
# So `app` inherits this include path automatically — no duplication.
target_include_directories(mathx PUBLIC include)

# PRIVATE: mathx compiles with these warnings, but consumers do not inherit them.
target_compile_options(mathx PRIVATE -Wall -Wextra)
