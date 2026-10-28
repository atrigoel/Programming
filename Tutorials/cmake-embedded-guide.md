# CMake for Embedded Systems

CMake is a **build system generator**. You describe your build once in `CMakeLists.txt`,
and CMake generates the actual build files (Unix Makefiles, Ninja, Visual Studio, Xcode…)
for whatever tool you prefer. You then run that tool to compile. For embedded work its
killer feature is **cross-compilation** — building on your PC (the *host*) to produce
firmware for a different target chip (ARM Cortex-M, RISC-V, AVR…) that may have no OS at
all.

Why CMake instead of a hand-written Makefile? Because it scales: automatic dependency
tracking, out-of-the-box support for multiple compilers and generators, reusable
toolchain files, dependency management (`find_package`, `FetchContent`), test integration
(CTest), and IDE support (presets). The cost is a declarative, sometimes quirky, language
you have to learn.

> **Runnable examples:** every concept below is demonstrated in
> [`examples/cmake/`](examples/cmake/README.md):
> [01-hello-host](examples/cmake/01-hello-host/README.md) (configure vs build),
> [02-library-propagation](examples/cmake/02-library-propagation/README.md)
> (`PUBLIC`/`PRIVATE`), and
> [03-cross-cortex-m4](examples/cmake/03-cross-cortex-m4/README.md)
> (toolchain file + bare-metal Cortex-M4).

## Contents

1. [Core concepts](#1-core-concepts)
2. [Targets and properties](#2-targets-and-properties)
3. [Variables, cache, and scope](#3-variables-cache-and-scope)
4. [Cross-compilation: the toolchain file](#4-cross-compilation-the-toolchain-file)
5. [Generator expressions](#5-generator-expressions)
6. [Post-build steps: .elf → .bin / .hex](#6-post-build-steps-elf--bin--hex)
7. [Putting it all together](#7-putting-it-all-together)
8. [Build types (Debug vs Release)](#8-build-types-debug-vs-release)
9. [Dependencies: find_package and FetchContent](#9-dependencies-find_package-and-fetchcontent)
10. [Testing with CTest](#10-testing-with-ctest)
11. [Presets](#11-presets-recommended-for-real-projects)
12. [Common pitfalls](#12-common-pitfalls)

---

## 1. Core Concepts

### The two-phase model
```
CMakeLists.txt  ──(configure)──►  build files  ──(build)──►  firmware.elf
```
- **Configure**: `cmake -B build` reads your `CMakeLists.txt`, runs all your logic
  (checks, variable expansion, target definitions) and generates build files into a
  `build/` folder. This phase runs *once* and is re-run automatically when a
  `CMakeLists.txt` changes.
- **Build**: `cmake --build build` invokes the underlying tool (make/ninja) to actually
  compile and link. This is what you run repeatedly while coding.

Always build **out-of-source** (into a separate `build/` dir) so you never pollute your
source tree — you can delete `build/` at any time and reconfigure from scratch.

### Generators
A *generator* is the build tool CMake targets. Pick one at configure time with `-G`:
```bash
cmake -B build -G Ninja          # fast, parallel, recommended for embedded
cmake -B build -G "Unix Makefiles"  # the default on Linux
```
- **Single-config** generators (Ninja, Unix Makefiles) bake the build type in at
  *configure* time via `-DCMAKE_BUILD_TYPE=Debug`.
- **Multi-config** generators (Ninja Multi-Config, Visual Studio) choose the config at
  *build* time via `cmake --build build --config Debug`.

For embedded, **Ninja** is the usual choice: it's fast and works identically everywhere.
Whatever you pick, `cmake --build build` drives it — you never call `ninja`/`make`
directly.

### Minimal example
```cmake
cmake_minimum_required(VERSION 3.20)
project(blink C ASM)          # name + languages (C and assembly)

add_executable(blink main.c)  # produce an executable "target"
```

Everything in CMake revolves around **targets** (executables/libraries) and **properties** attached to them.

---

## 2. Targets and Properties

Prefer the **modern "target_*" commands** over global variables. They scope settings to a specific target and propagate cleanly.

```cmake
add_executable(firmware
    src/main.c
    src/gpio.c
    src/startup.s
)

# Include directories (PRIVATE = only this target; PUBLIC = also consumers)
target_include_directories(firmware PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Preprocessor defines
target_compile_definitions(firmware PRIVATE
    STM32F407xx
    USE_HAL_DRIVER
)

# Compiler flags
target_compile_options(firmware PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -Wall
    -ffunction-sections     # allow linker to drop unused code
    -fdata-sections
)

# Linker flags (linker script + garbage collection + map file)
target_link_options(firmware PRIVATE
    -T${CMAKE_SOURCE_DIR}/STM32F407.ld
    -Wl,--gc-sections
    -Wl,-Map=firmware.map
    --specs=nano.specs        # newlib-nano
)
```

### PRIVATE / PUBLIC / INTERFACE
This is the most important concept for scaling projects:
- **PRIVATE** — used to build *this* target only.
- **INTERFACE** — not used to build this target, but passed to anything that links it.
- **PUBLIC** — both (`PRIVATE` + `INTERFACE`).

Example with a library:
```cmake
add_library(hal STATIC src/hal_gpio.c src/hal_uart.c)

target_include_directories(hal PUBLIC include)  # consumers see these headers

add_executable(firmware src/main.c)
target_link_libraries(firmware PRIVATE hal)     # firmware auto-gets hal's PUBLIC includes
```
`firmware` automatically inherits `hal`'s public include paths — no manual duplication.
This "usage requirements" model is demonstrated end-to-end in
[examples/cmake/02-library-propagation](examples/cmake/02-library-propagation/README.md).

---

## 3. Variables, Cache, and Scope

CMake has two kinds of variables, and mixing them up is a top source of confusion.

### Normal variables
Set with `set(NAME value)`, expanded with `${NAME}`. They are **scoped** to the current
directory/function and its children, and vanish when configure finishes.
```cmake
set(SOURCES src/main.c src/gpio.c)   # a list is just a ;-separated string
add_executable(fw ${SOURCES})
```

### Cache variables
Persist across configure runs in `build/CMakeCache.txt`. These are the knobs a user tunes
on the command line with `-D`.
```cmake
set(MCU "STM32F407" CACHE STRING "Target MCU")
option(ENABLE_ASSERTS "Compile in runtime asserts" ON)   # a BOOL cache variable
```
```bash
cmake -B build -DMCU=STM32F411 -DENABLE_ASSERTS=OFF
```
Built-in cache variables you'll use constantly: `CMAKE_BUILD_TYPE`,
`CMAKE_TOOLCHAIN_FILE`, `CMAKE_C_COMPILER`, `CMAKE_INSTALL_PREFIX`.

### Useful predefined variables
| Variable | Meaning |
|---|---|
| `CMAKE_SOURCE_DIR` | Top-level source dir (where the root `CMakeLists.txt` lives) |
| `CMAKE_BINARY_DIR` | Top-level build dir |
| `CMAKE_CURRENT_SOURCE_DIR` | The dir of the `CMakeLists.txt` being processed |
| `CMAKE_CURRENT_BINARY_DIR` | Its corresponding build dir |
| `PROJECT_NAME` | Name from the nearest `project()` |

### Prefer targets over global variables
Old CMake leaned on globals like `CMAKE_C_FLAGS` and `include_directories()`. Modern CMake
prefers `target_*` commands because they scope settings to one target and propagate
correctly. Reach for a global (`add_compile_options`) only for flags that genuinely apply
to *everything*, such as the MCU `-mcpu` flags.

---

## 4. Cross-Compilation: The Toolchain File

This is what makes CMake work for embedded. A **toolchain file** tells CMake to use `arm-none-eabi-gcc` instead of your host's `gcc`.

`arm-toolchain.cmake`:
```cmake
set(CMAKE_SYSTEM_NAME Generic)        # "Generic" = bare-metal, no OS
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# Bare-metal: we build a static lib to test the compiler, not run a program
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Don't look for programs/libs/headers in the host's system paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Use it at configure time:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake -G Ninja
cmake --build build
```

Two critical embedded gotchas handled above:
1. **`CMAKE_SYSTEM_NAME Generic`** — signals bare-metal (no `main`-running OS).
2. **`CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`** — CMake normally test-compiles a full *executable* to verify the compiler works. On bare-metal that fails (no startup/linker script yet), so we tell it to only build a library for the check.

The `CMAKE_FIND_ROOT_PATH_MODE_*` lines keep `find_*` from accidentally picking up your
host's headers/libraries: search the host only for *programs* (the compiler), and only
the cross sysroot for libraries and includes. A complete, working toolchain file lives in
[examples/cmake/03-cross-cortex-m4/arm-none-eabi.cmake](examples/cmake/03-cross-cortex-m4/arm-none-eabi.cmake).

> **Pass a toolchain file** with either `-DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake` or
> the newer shorthand `--toolchain arm-toolchain.cmake`. It must be set on the *first*
> configure of a build dir; changing it later means deleting `build/` and reconfiguring.

---

## 5. Generator Expressions

Generator expressions (`$<...>`) are evaluated at **build/generate time**, not while the
`CMakeLists.txt` is being read. They're how you vary flags per configuration, per
language, or per compiler without `if()` sprawl.

```cmake
target_compile_options(firmware PRIVATE
    -Wall
    $<$<CONFIG:Debug>:-Og;-g3>          # only in Debug builds
    $<$<CONFIG:Release>:-Os>            # only in Release builds
    $<$<COMPILE_LANGUAGE:C>:-std=c11>   # only for C sources, not ASM
)
```

The most common ones in embedded projects:
| Expression | Resolves to |
|---|---|
| `$<TARGET_FILE:firmware>` | Full path to the built `firmware.elf` (used in post-build) |
| `$<CONFIG:Debug>` | `1` in a Debug build, else `0` |
| `$<COMPILE_LANGUAGE:C>` | `1` when compiling a C source |
| `$<BUILD_INTERFACE:...>` / `$<INSTALL_INTERFACE:...>` | Different include paths for in-tree vs installed use |

Remember: because they resolve *late*, you can't `message()`-print a generator expression
and expect its final value during configure.

---

## 6. Post-Build Steps: .elf → .bin / .hex

Flashers usually need `.bin` or `.hex`, not `.elf`. Generate them automatically and print size:

```cmake
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:firmware> firmware.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware> firmware.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
    COMMENT "Generating .hex/.bin and printing size"
)
```
`$<TARGET_FILE:firmware>` is a **generator expression** — it resolves to the full path of the output file at build time.

Add a flash target for convenience:
```cmake
add_custom_target(flash
    COMMAND st-flash write firmware.bin 0x8000000
    DEPENDS firmware
)
# Usage: cmake --build build --target flash
```

---

## 7. Putting It All Together

A realistic layout:
```
project/
├── CMakeLists.txt
├── arm-toolchain.cmake
├── STM32F407.ld
├── include/
├── src/
│   ├── main.c
│   └── startup.s
└── drivers/
    └── CMakeLists.txt
```

Top-level `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20)
project(myfirmware C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared MCU flags applied to everything below
add_compile_options(
    -mcpu=cortex-m4 -mthumb
    -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    $<$<CONFIG:Debug>:-Og;-g3>       # Debug build: optimize for debugging
    $<$<CONFIG:Release>:-Os>         # Release build: optimize for size
)

add_subdirectory(drivers)            # builds the "drivers" library

add_executable(firmware src/main.c src/startup.s)
target_link_libraries(firmware PRIVATE drivers)
target_link_options(firmware PRIVATE
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -T${CMAKE_SOURCE_DIR}/STM32F407.ld
    -Wl,--gc-sections -Wl,-Map=firmware.map
    --specs=nano.specs --specs=nosys.specs
)

# post-build .bin/.hex + size (as in section 4)
```

`drivers/CMakeLists.txt`:
```cmake
add_library(drivers STATIC
    gpio.c
    uart.c
)
target_include_directories(drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

Build:
```bash
cmake -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

---

## 8. Build Types (Debug vs Release)

For single-config generators (Make/Ninja), select at configure time:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release ...
```
CMake applies default flags per type (`Debug`, `Release`, `MinSizeRel`, `RelWithDebInfo`). On embedded you'll typically want `MinSizeRel` or `Release` (`-Os`) to fit in flash, and `Debug` (`-Og -g3`) while stepping through code.

You can override the per-type flags yourself — handy when the defaults (`-O2 -DNDEBUG`)
aren't what you want on a tiny MCU:
```cmake
set(CMAKE_C_FLAGS_RELEASE     "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG       "-Og -g3")
set(CMAKE_C_FLAGS_MINSIZEREL  "-Os -DNDEBUG -flto")
```

---

## 9. Dependencies: find_package and FetchContent

Two ways to bring in external code.

**`find_package`** locates something already installed on the system and imports it as a
target:
```cmake
find_package(Threads REQUIRED)
target_link_libraries(app PRIVATE Threads::Threads)
```
On bare-metal you rarely use this (there's no system to find things in), but it's the
backbone of host-side tooling and tests.

**`FetchContent`** downloads and builds a dependency as part of your configure step — ideal
for vendoring a HAL, an RTOS, or a unit-test framework with a pinned version:
```cmake
include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0            # always pin a tag/commit, never a branch
)
FetchContent_MakeAvailable(unity)

target_link_libraries(tests PRIVATE unity)
```
Because the dependency becomes a normal CMake target, its usage requirements (include
dirs, defines) propagate just like any local library.

---

## 10. Testing with CTest

CMake ships with a test runner, CTest. For embedded you typically split code into
hardware-independent logic (testable on the host) and hardware drivers (tested on
target or in simulation). Register host tests like this:
```cmake
enable_testing()

add_executable(test_math tests/test_math.c)
target_link_libraries(test_math PRIVATE mathx)

add_test(NAME math COMMAND test_math)
```
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```
For on-target tests you can set the `COMMAND` to a runner that flashes the board and
captures serial output, or use an emulator like QEMU or Renode as the test command.

---

## 11. Presets (recommended for real projects)

`CMakePresets.json` saves you from retyping long commands:
```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "stm32-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "toolchainFile": "${sourceDir}/arm-toolchain.cmake",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    }
  ]
}
```
```bash
cmake --preset stm32-debug
cmake --build build/debug
```

---

## 12. Common Pitfalls

- **Compiler and linker flags disagree.** The CPU flags (`-mcpu`, `-mfpu`,
  `-mfloat-abi`) must be present on *both* `target_compile_options` and
  `target_link_options`, or you get cryptic "uses VFP register arguments" link errors.
- **Editing the toolchain file after configuring.** The toolchain is locked into a build
  dir on first configure. If you change it, delete `build/` and reconfigure.
- **Forgetting `ASM` in `project()`.** Startup files won't compile; add `ASM` to the
  language list: `project(fw C ASM)`.
- **Relying on globbing for sources.** `file(GLOB ...)` won't notice new files without a
  reconfigure. List sources explicitly, or use `CONFIGURE_DEPENDS` and accept the caveats.
- **In-source builds.** Running `cmake .` litters your tree. Always use `-B build`.
- **Compiler probe fails on bare metal.** If configure dies at "The C compiler ...
  is not able to compile a simple test program", you forgot
  `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` in the toolchain file.
- **Mismatched `--specs`.** Pick a consistent C library story: `--specs=nano.specs`
  (newlib-nano) plus `--specs=nosys.specs` (stub syscalls) for a no-OS image.

---

## Mental Model Summary

| Concept | Purpose |
|---|---|
| `project()` | Declare name + languages (include `ASM` for startup files) |
| `add_executable` / `add_library` | Define **targets** |
| `target_*` commands | Attach includes/defines/flags to a target |
| `PRIVATE`/`PUBLIC`/`INTERFACE` | Control how settings propagate to consumers |
| **Toolchain file** | Switch from host gcc → cross-compiler (the embedded key) |
| `CMAKE_SYSTEM_NAME Generic` | Tell CMake it's bare-metal |
| `add_custom_command` | Post-build `.bin`/`.hex`/size steps |
| Presets | Save configure/build settings |

### Suggested learning path
1. Build a host "hello world" with `add_executable` — understand configure vs build.
   ([01-hello-host](examples/cmake/01-hello-host/README.md))
2. Split code into a library, practice `PUBLIC`/`PRIVATE` propagation.
   ([02-library-propagation](examples/cmake/02-library-propagation/README.md))
3. Write a toolchain file and cross-compile a blinky.
   ([03-cross-cortex-m4](examples/cmake/03-cross-cortex-m4/README.md))
4. Add linker script, `--gc-sections`, and post-build objcopy.
5. Adopt `CMakePresets.json`.

## Further References
- CMake official documentation: <https://cmake.org/cmake/help/latest/>
- *Professional CMake: A Practical Guide* by Craig Scott — the definitive book.
- *An Introduction to Modern CMake* (Henry Schreiner): <https://cliutils.gitlab.io/modern-cmake/>
- `cmake(1)`, `cmake-toolchains(7)`, `cmake-generator-expressions(7)` man pages.
