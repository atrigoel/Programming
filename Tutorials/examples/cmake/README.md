# CMake Examples

Companions to [../../cmake-embedded-guide.md](../../cmake-embedded-guide.md). Work through
them in order — each adds one concept.

| Folder | Concept | Tools |
|--------|---------|-------|
| [`01-hello-host/`](01-hello-host/) | Configure vs build, out-of-source builds | `cmake`, host `gcc` |
| [`02-library-propagation/`](02-library-propagation/) | Targets, static libraries, `PRIVATE`/`PUBLIC`/`INTERFACE` | `cmake`, host `gcc` |
| [`03-cross-cortex-m4/`](03-cross-cortex-m4/) | Toolchain file, bare-metal link, post-build `.hex`/`.bin`, presets | `cmake`, `arm-none-eabi-gcc` |

## Setup

```bash
# Host examples (01, 02)
sudo apt-get install cmake ninja-build build-essential

# Cross example (03)
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
```

## Quick smoke test

```bash
cd 01-hello-host && cmake -B build && cmake --build build && ./build/hello && cd -
```

Each example generates everything into a `build/` directory; delete it with `rm -rf build`.
