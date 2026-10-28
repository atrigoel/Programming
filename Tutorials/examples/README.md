# Runnable Examples

Hands-on companions to the guides in the parent folder. Everything here is meant to be
run on a **Linux host** (native, WSL, or a container). Each subfolder has its own
`README.md` with step-by-step commands.

## Layout

| Folder | Guide | Tools needed |
|--------|-------|--------------|
| [`make/`](make/) | [make-guide.md](../make-guide.md) | `gcc`, `make` |
| [`autoconf/`](autoconf/) | [autoconf-guide.md](../autoconf-guide.md) | `gcc`, `make`, `autoconf` |
| [`automake/`](automake/) | [automake-guide.md](../automake-guide.md) | `gcc`, `make`, `autoconf`, `automake` |
| [`linker-scripts/`](linker-scripts/) | [linker-scripts-guide.md](../linker-scripts-guide.md) | `gcc`, `binutils` (`ld`, `nm`, `objdump`, `readelf`, `size`) |
| [`cmake/`](cmake/) | [cmake-embedded-guide.md](../cmake-embedded-guide.md) | `cmake`, `ninja`, host `gcc`, `arm-none-eabi-gcc` (ex. 03) |
| [`gcc/`](gcc/) | [gcc-guide.md](../gcc-guide.md) | host `gcc`, `binutils`, `arm-none-eabi-gcc` (ex. 05) |
| [`udev/`](udev/) | [udev-guide.md](../udev-guide.md) | Linux with `udev` (rule files; installed with `sudo`) |
| [`openocd/`](openocd/) | [openocd-guide.md](../openocd-guide.md) | `openocd`, `arm-none-eabi-gdb`, a debug probe + board |
| [`gdb/`](gdb/) | [gdb-guide.md](../gdb-guide.md) | host `gcc`, `gdb` (no board needed) |

## One-time setup (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install build-essential autoconf automake libtool binutils
```

`build-essential` provides `gcc`, `make`, and `binutils`. Add `autoconf automake
libtool` for the Autotools examples. For the CMake, GCC-cross, and OpenOCD examples also
install:

```bash
sudo apt-get install cmake ninja-build gcc-arm-none-eabi binutils-arm-none-eabi \
                     openocd gdb-multiarch
```

## Suggested learning order

1. **make/01-hello → 05-auto-deps** — learn the build engine everything else generates.
2. **gcc/01 → 05** — the compiler flags every build system ultimately drives.
3. **linker-scripts/01 → 03** — see where code/data actually goes (great alongside the
   Cortex-M4 guide).
4. **cmake/01 → 03** — the modern build-system generator, up to cross-compiling.
5. **autoconf/01-configure-only** — the raw portability layer over Make.
6. **automake/01-hello-automake** — the high-level generator that ties it all together.
7. **gdb/01 → 06** — advanced debugging on the host (breakpoints, watchpoints, reverse,
   scripting); no hardware required.
8. **udev/ + openocd/** — get a probe recognized on Linux, then flash and debug on target.

## Quick smoke test

```bash
cd make/01-hello && make && ./hello && make clean && cd -
```

Each example folder is self-contained and safe to `make clean`. Generated Autotools
artifacts are covered by the local `.gitignore` files.
