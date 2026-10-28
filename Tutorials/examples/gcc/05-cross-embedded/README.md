# GCC Example 05 — Cross-Compiling for Bare-Metal Cortex-M4

The embedded payoff: the same `gcc` flags plus ARM CPU selectors, a linker script, and a
startup file produce a flashable firmware image.

## Prerequisites

```bash
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
arm-none-eabi-gcc --version
```

## Layout

```
05-cross-embedded/
├── src/startup.s          # vector table + reset handler
├── src/main.c             # exercises .data and .bss
├── linker/cortex-m4.ld    # FLASH/RAM layout
└── Makefile
```

## Run

```bash
make            # -> build/fw.elf, fw.hex, fw.bin, and a size report
make clean
```

Expected size output (numbers vary):

```
   text    data     bss     dec     hex filename
    140       8       8     156      9c build/fw.elf
```

## What to notice

- **CPU flags in one place (`CPU`)** feed both `CFLAGS` and `LDFLAGS`. If they differ
  between compile and link you get "uses VFP register arguments" link errors.
- **`-ffunction-sections -fdata-sections` + `-Wl,--gc-sections`** strip unused code —
  open `build/fw.map` to see what the linker kept and dropped.
- **`-nostartfiles`** — we supply the vector table and `Reset_Handler` in `startup.s`
  instead of GCC's default crt0.
- **`--specs=nano.specs --specs=nosys.specs`** — link newlib-nano and stub the syscalls a
  bare-metal target has no OS for.
- **`.elf` vs `.bin`/`.hex`** — the ELF holds debug info for GDB/OpenOCD; the bin/hex are
  the stripped images you flash. See the OpenOCD guide to load and debug `fw.elf`.
- Inspect the machine code: `arm-none-eabi-objdump -d build/fw.elf | less` — you'll see
  16-bit Thumb instructions.

This is the Make-based twin of the CMake port in
[../../cmake/03-cross-cortex-m4](../../cmake/03-cross-cortex-m4/README.md).
