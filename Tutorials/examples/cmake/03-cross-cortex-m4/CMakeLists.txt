cmake_minimum_required(VERSION 3.20)

# Include ASM because the startup file is hand-written assembly.
project(blinky C ASM)

# --- CPU selection: shared by both compiler and linker ------------------------
# Cortex-M4 with single-precision hardware FPU, Thumb instruction set.
set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_executable(blinky
    src/startup.s
    src/main.c
)

target_compile_options(blinky PRIVATE
    ${CPU_FLAGS}
    -Wall -Wextra
    -ffunction-sections -fdata-sections   # one section per function/object...
    $<$<CONFIG:Debug>:-Og;-g3>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(blinky PRIVATE
    ${CPU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/linker/cortex-m4.ld
    -Wl,--gc-sections                       # ...so the linker can drop unused ones
    -Wl,-Map=blinky.map
    -nostartfiles                           # we provide our own startup + vectors
    --specs=nano.specs --specs=nosys.specs  # newlib-nano, stubbed syscalls
)

# Post-build: create flashable images and print the size breakdown.
add_custom_command(TARGET blinky POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:blinky> blinky.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blinky> blinky.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blinky>
    COMMENT "Generating blinky.hex / blinky.bin and printing size"
)
