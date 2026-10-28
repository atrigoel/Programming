# GCC Examples

Companions to [../../gcc-guide.md](../../gcc-guide.md). Each folder isolates one group of
flags so you can see exactly what they do.

| Folder | Focus | Tools |
|--------|-------|-------|
| [`01-compilation-stages/`](01-compilation-stages/) | `-E` `-S` `-c` — the preprocess/compile/assemble/link pipeline | host `gcc` |
| [`02-warnings-standards/`](02-warnings-standards/) | `-Wall` `-Wextra` `-Werror` `-std=` `-pedantic` | host `gcc` |
| [`03-optimization-debug/`](03-optimization-debug/) | `-O0/-O2/-Os/-Og`, `-g3`, comparing size & assembly | host `gcc`, `binutils` |
| [`04-preprocessor-linking/`](04-preprocessor-linking/) | `-D` `-I` `-L` `-l`, static libraries, `-MMD` auto-deps | host `gcc`, `ar` |
| [`05-cross-embedded/`](05-cross-embedded/) | `-mcpu` `-mthumb` `-mfpu`, linker script, `--specs`, `objcopy` | `arm-none-eabi-gcc` |

## Setup

```bash
sudo apt-get install build-essential binutils        # examples 01-04
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi   # example 05
```

## Quick smoke test

```bash
cd 01-compilation-stages && make stages && make clean && cd -
```
