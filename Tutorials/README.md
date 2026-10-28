# Embedded / Build-Systems Tutorials

A set of detailed, vendor-agnostic guides plus runnable examples for embedded systems
and the GNU build toolchain.

## Guides

| Guide | Topic |
|-------|-------|
| [armv7m-architecture-summary.md](armv7m-architecture-summary.md) | Explained summary of the ARMv7-M Architecture Reference Manual |
| [cortex-m0-architecture-guide.md](cortex-m0-architecture-guide.md) | ARM Cortex-M0 core (ARMv6-M): entry-level, PRIMASK-only, single HardFault |
| [cortex-m0plus-architecture-guide.md](cortex-m0plus-architecture-guide.md) | ARM Cortex-M0+ core: M0 + 2-stage pipeline, MPU, VTOR, MTB, fast I/O |
| [cortex-m3-architecture-guide.md](cortex-m3-architecture-guide.md) | ARM Cortex-M3 core (ARMv7-M): full Thumb-2, NVIC, bit-banding, faults |
| [cortex-m4-architecture-guide.md](cortex-m4-architecture-guide.md) | ARM Cortex-M4 core: registers, modes, NVIC, FPU, MPU, memory model |
| [cortex-m7-architecture-guide.md](cortex-m7-architecture-guide.md) | ARM Cortex-M7 core: dual-issue pipeline, cache, TCM, AXI, FPv5 (DP) |
| [cortex-m-comparison-guide.md](cortex-m-comparison-guide.md) | Cortex-M0/M0+/M3/M4/M7 differences at a glance + how to choose |
| [make-guide.md](make-guide.md) | GNU Make: rules, variables, pattern rules, auto-deps |
| [autoconf-guide.md](autoconf-guide.md) | Autoconf: `configure.ac`, feature checks, `config.h` |
| [automake-guide.md](automake-guide.md) | Automake: `Makefile.am`, primaries, `make check`/`dist` |
| [linker-scripts-guide.md](linker-scripts-guide.md) | GNU `ld` linker scripts: sections, `MEMORY`, VMA/LMA |
| [elf-and-executables-guide.md](elf-and-executables-guide.md) | ELF & object files, sections/segments, `.bin`/`.hex`/`.srec`, `objcopy`/`srec_cat` conversions |
| [cmake-embedded-guide.md](cmake-embedded-guide.md) | CMake for embedded builds |
| [gcc-guide.md](gcc-guide.md) | GCC: the flags you actually use, plus embedded cross-compilation |
| [udev-guide.md](udev-guide.md) | udev: the Linux device model, rules, and adding/naming devices |
| [openocd-guide.md](openocd-guide.md) | OpenOCD: flashing & debugging Cortex MCUs, SWD/JTAG, ITM/ETM/CoreSight |
| [gdb-guide.md](gdb-guide.md) | GDB: advanced debugging — watchpoints, reverse debugging, scripting, embedded via OpenOCD |

## Suggested reading order

The guides are written to stand alone, but if you're working through them for the first
time this order builds each topic on the previous ones. Pick the track that matches your
goal — or do **1 → 2 → 3 → 4** end to end.

### 1. Compile & build (start here)

1. [gcc-guide.md](gcc-guide.md) — how source becomes objects and executables; the flags you'll use everywhere.
2. [make-guide.md](make-guide.md) — automate the build with GNU Make.
3. [autoconf-guide.md](autoconf-guide.md) — portable configuration (`configure.ac`, `config.h`).
4. [automake-guide.md](automake-guide.md) — generate Makefiles from a high-level description.
5. [cmake-embedded-guide.md](cmake-embedded-guide.md) — a modern alternative to the Autotools, with cross-compilation.

### 2. From code to a memory image

6. [linker-scripts-guide.md](linker-scripts-guide.md) — where code/data land in memory (sections, `MEMORY`, VMA/LMA).
7. [elf-and-executables-guide.md](elf-and-executables-guide.md) — what the resulting `.elf` really is, and how to turn it into `.bin`/`.hex`/`.srec` for flashing.

### 3. The target hardware (ARM Cortex-M)

8. [cortex-m-comparison-guide.md](cortex-m-comparison-guide.md) — the landscape and how to choose a core.
9. Then the guide for **your** core, in increasing complexity:
   [cortex-m0](cortex-m0-architecture-guide.md) →
   [cortex-m0+](cortex-m0plus-architecture-guide.md) →
   [cortex-m3](cortex-m3-architecture-guide.md) →
   [cortex-m4](cortex-m4-architecture-guide.md) →
   [cortex-m7](cortex-m7-architecture-guide.md).

### 4. Flash & debug on hardware

10. [udev-guide.md](udev-guide.md) — let Linux see your probe/board with stable device names.
11. [openocd-guide.md](openocd-guide.md) — flash the image and open a debug channel over SWD/JTAG.
12. [gdb-guide.md](gdb-guide.md) — debug the running target (breakpoints, watchpoints, scripting, post-mortem).

> **Just building host software?** Steps 1 (plus `elf-and-executables-guide.md` for the
> ELF/inspection parts) and `gdb-guide.md` are all you need — skip the Cortex-M, udev
> and OpenOCD guides.

## Examples

Runnable code for each build-system guide lives in [`examples/`](examples/README.md).
Run them on a Linux host (native, WSL, or container):

```bash
cd examples/make/01-hello && make && ./hello
```

See [examples/README.md](examples/README.md) for prerequisites and a suggested order.

## How the pieces fit together

```
 C sources ──▶ [ Make ] ──▶ compiler ──▶ objects ──▶ [ ld + linker script ] ──▶ image
                  ▲                                          ▲
      generated Makefile                            controls memory layout
                  │
        [ Autoconf + Automake ]  ── portability + high-level project description
```

- **Make** runs the build.
- **Autoconf/Automake** generate portable Makefiles and probe the system.
- **Linker scripts** decide where code/data live in memory (crucial for the Cortex-M4).

Each guide ends with a **Further References** section for deeper study.
