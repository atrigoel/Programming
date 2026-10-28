# CMake Example 03 — Cross-Compiling for Cortex-M4 (bare metal)

A **real** bare-metal firmware image: vector table, startup assembly, linker script,
and post-build `.hex`/`.bin` generation. This is the payoff — the same CMake concepts
from examples 01–02, now aimed at an ARM target instead of your PC.

## Prerequisites

The GNU Arm Embedded toolchain must be installed and on your `PATH`:

```bash
# Debian/Ubuntu
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
# Verify
arm-none-eabi-gcc --version
```

Ninja is optional but used by the presets (`sudo apt-get install ninja-build`).

## Layout

```
03-cross-cortex-m4/
├── CMakeLists.txt          # target + CPU flags + post-build objcopy/size
├── CMakePresets.json       # debug / release presets
├── arm-none-eabi.cmake     # toolchain file (host gcc -> arm-none-eabi-gcc)
├── linker/cortex-m4.ld     # FLASH/RAM layout, .data/.bss placement
└── src/
    ├── startup.s           # vector table + reset handler (.data copy, .bss zero)
    └── main.c              # blinky skeleton exercising .data and .bss
```

## Run

Using presets (recommended):

```bash
cmake --preset cortex-m4-debug
cmake --build --preset cortex-m4-debug
```

Or the explicit form:

```bash
cmake -B build --toolchain arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

You'll get `blinky.elf`, `blinky.hex`, `blinky.bin`, and a size line like:

```
   text    data     bss     dec     hex filename
    148       8       8     164      a4 blinky.elf
```

## What to notice

- **The toolchain file is the whole trick.** `--toolchain arm-none-eabi.cmake` swaps
  the compiler; nothing in `CMakeLists.txt` is host-specific.
- **`CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`** in the toolchain file stops CMake's
  compiler probe from trying to link a full executable (which would fail before we have
  a linker script).
- **CPU flags go to *both* compile and link.** `-mcpu`/`-mfpu`/`-mfloat-abi` must match
  on both sides or you get link errors about incompatible float ABIs.
- **`-Wl,--gc-sections` + `-ffunction-sections -fdata-sections`** work as a pair to strip
  unused code — check `blinky.map` to see what was kept.
- **`-nostartfiles`** tells GCC not to pull in its own crt0; *we* supply the vector table
  and reset handler in `startup.s`.
- Disassemble to see it's really Thumb/Cortex-M4 code:
  `arm-none-eabi-objdump -d build/blinky.elf | less`.
