# GCC / GNU ld Linker Scripts — A Detailed Guide

> A **linker script** tells the GNU linker (`ld`) how to combine input object files'
> **sections** into an output executable and **where** each piece lands in memory. It
> is the bridge between compiled code and a physical memory map — indispensable for
> embedded/bare-metal targets (like the Cortex-M4), and useful to understand even for
> hosted Linux programs. Runnable examples live in
> [`examples/linker-scripts/`](examples/linker-scripts/).

---

## Table of Contents

- [GCC / GNU ld Linker Scripts — A Detailed Guide](#gcc--gnu-ld-linker-scripts--a-detailed-guide)
  - [Table of Contents](#table-of-contents)
  - [1. What the Linker Does](#1-what-the-linker-does)
  - [2. Sections](#2-sections)
  - [3. The Default Script and Why You'd Override It](#3-the-default-script-and-why-youd-override-it)
  - [4. Skeleton](#4-skeleton)
  - [5. MEMORY: Describing Physical Regions](#5-memory-describing-physical-regions)
  - [6. SECTIONS: Placing Content](#6-sections-placing-content)
  - [7. The Location Counter `.`](#7-the-location-counter-)
  - [8. Symbols Defined in the Script](#8-symbols-defined-in-the-script)
  - [9. VMA vs LMA (Load vs Run Address)](#9-vma-vs-lma-load-vs-run-address)
  - [10. ENTRY, KEEP, ALIGN, PROVIDE](#10-entry-keep-align-provide)
  - [11. A Realistic Cortex-M4 Script](#11-a-realistic-cortex-m4-script)
  - [12. Startup Code and the Script Work Together](#12-startup-code-and-the-script-work-together)
  - [13. Inspecting the Result](#13-inspecting-the-result)
  - [14. Common Pitfalls](#14-common-pitfalls)
  - [15. The Runnable Examples](#15-the-runnable-examples)
  - [16. Further References](#16-further-references)

---

## 1. What the Linker Does

The compiler turns each `.c` into an object file (`.o`) containing **sections** of code
and data, plus a **symbol table** and **relocations** (placeholders for addresses not
yet known). The linker:

1. **Collects** all input sections of the same kind from all `.o` and `.a` files.
2. **Places** them at concrete memory addresses according to the linker script.
3. **Resolves** symbols (matches each reference to its definition).
4. **Relocates** — patches the code/data with the final addresses.
5. **Emits** the output (ELF executable, or a raw/hex image after `objcopy`).

The linker script controls step 2 (and influences 5).

---

## 2. Sections

Standard sections the toolchain produces:

| Section | Contents | Typical storage |
|---------|----------|-----------------|
| `.text` | executable code, and often constants | Flash / ROM |
| `.rodata` | read-only data (string literals, `const`) | Flash / ROM |
| `.data` | initialized read/write globals (`int g = 5;`) | **stored** in Flash, **runs** in RAM |
| `.bss` | zero-initialized/uninitialized globals (`int g;`) | RAM (not stored — just reserved) |
| `.stack` / `.heap` | reserved RAM regions (script-defined) | RAM |
| `.isr_vector` | interrupt vector table (embedded) | start of Flash |

Key insight: **`.data` has two addresses** — where its initial values are *stored*
(Flash) and where the program *accesses* it at runtime (RAM). Startup code copies it
from the former to the latter. **`.bss` occupies no file space**; startup code just
zeroes that RAM range.

---

## 3. The Default Script and Why You'd Override It

`ld` has a built-in default linker script (see it with `ld --verbose`) that works for
normal hosted programs — you rarely touch it on Linux. You provide your own when:

- Targeting **bare-metal / MCUs**: you must match the chip's Flash/RAM addresses.
- You need **custom sections** (bootloader, DFU, config blocks, `.noinit` RAM).
- You need **exact placement** (vector table at 0x0, a function in RAM, a struct at a
  fixed address).
- You are writing an **OS kernel** or firmware image.

Pass a custom script with `-T script.ld` (bare-metal) or `-Wl,-T,script.ld` via gcc.

---

## 4. Skeleton

```ld
/* Where execution starts (symbol name). */
ENTRY(_start)

/* Physical memory regions of the target. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* How to map input sections into output sections and regions. */
SECTIONS
{
    .text : { *(.text*) } > FLASH
    .data : { *(.data*) } > RAM AT> FLASH
    .bss  : { *(.bss*)  } > RAM
}
```

The three top-level commands you use most: **`ENTRY`**, **`MEMORY`**, **`SECTIONS`**.

---

## 5. MEMORY: Describing Physical Regions

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
```

- **name** — used later with `> NAME`.
- **attributes** in parentheses: `r` read, `w` write, `x` execute, `a` allocatable,
  `i`/`l` initialized. These are advisory (help catch mistakes / overflow warnings).
- **ORIGIN** — start address. **LENGTH** — size (`K`, `M` suffixes allowed).

If a section's content exceeds a region's `LENGTH`, the linker errors with
"region overflowed" — an early, useful check that your firmware fits.

---

## 6. SECTIONS: Placing Content

The `SECTIONS` command maps **input sections** (from `.o` files) to **output sections**
(in the image):

```ld
SECTIONS
{
    .text :
    {
        *(.isr_vector)     /* vector table first */
        *(.text)           /* all .text from every input file */
        *(.text*)          /* and .text.foo, .text.bar (function sections) */
        *(.rodata*)        /* fold read-only data into the same region */
    } > FLASH
}
```

- `output_section : { input_specs } > REGION`
- `*(.text*)` — the `*` before `(` is a **file wildcard** (all files); inside are
  **section wildcards**. `file.o(.text)` selects from one file.
- Order inside the braces = order in memory. Put the vector table / boot code first.
- **`-ffunction-sections -fdata-sections`** (compiler) + **`--gc-sections`** (linker)
  let unused functions/data be discarded; the script uses `*(.text*)` to catch the
  per-function sections this produces.

---

## 7. The Location Counter `.`

`.` (dot) is the **current output address**. You can read and assign it:

```ld
. = 0x1000;            /* move the counter forward (creates a gap / padding) */
. = ALIGN(4);         /* round up to a 4-byte boundary */
_here = .;            /* capture the current address into a symbol */
. += 0x400;           /* reserve 0x400 bytes (e.g., a stack) */
```

Assigning to `.` never moves *backwards* (that's an error). It's how you align, pad, and
carve out reserved space (stack, heap).

---

## 8. Symbols Defined in the Script

You can **create symbols** in the script that your C/asm code references as `extern`.
This is how startup code learns the boundaries of each region:

```ld
.data :
{
    _sdata = .;          /* start of .data in RAM (VMA) */
    *(.data*)
    _edata = .;          /* end of .data in RAM */
} > RAM AT> FLASH

_sidata = LOADADDR(.data);   /* where .data's init image sits in FLASH (LMA) */

.bss :
{
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
} > RAM
```

In C, declare them (they are addresses, so take their address / use as arrays):

```c
extern uint32_t _sdata, _edata, _sidata, _sbss, _ebss;
```

Startup then copies `[_sidata .. )` from Flash into `[_sdata .. _edata)` and zeroes
`[_sbss .. _ebss)`.

---

## 9. VMA vs LMA (Load vs Run Address)

The single most important linker-script concept for embedded:

- **VMA (Virtual/Run Memory Address)** — the address the program *uses* at runtime.
- **LMA (Load Memory Address)** — the address where the data is *stored* in the image
  (Flash).

For code and read-only data, VMA == LMA (runs from Flash). For `.data`, they differ:

```ld
.data : { *(.data*) } > RAM AT> FLASH
```

- `> RAM` sets the **VMA** (runtime address in RAM).
- `AT> FLASH` sets the **LMA** (stored in Flash after `.text`).

At boot, RAM is empty, so startup copies the initial values from Flash (LMA) to RAM
(VMA). `LOADADDR(.data)` gives the LMA so the copy loop knows the source. On hosted
Linux the loader handles this; on bare metal *you* write the copy loop.

```
   FLASH (stored image)         RAM (runtime)
   +----------------+           +----------------+
   | .isr_vector    |           |                |
   | .text/.rodata  |           | .data  <-------+--- copied at boot
   | .data (LMA)  --+---copy---> | .bss  (zeroed) |
   +----------------+           | stack / heap   |
                                +----------------+
```

---

## 10. ENTRY, KEEP, ALIGN, PROVIDE

- **`ENTRY(sym)`** — sets the ELF entry point (first instruction). On MCUs it's usually
  `Reset_Handler`; the actual boot address comes from the vector table, but `ENTRY`
  also stops `--gc-sections` from discarding it.
- **`KEEP(*(.isr_vector))`** — prevents `--gc-sections` from garbage-collecting a
  section that has no direct references (the vector table is reached by hardware, not by
  a call, so you must `KEEP` it).
- **`ALIGN(n)`** — align the location counter; also `. = ALIGN(n);` or
  `.sec : ALIGN(n) { ... }`.
- **`PROVIDE(sym = value)`** — define `sym` only if the program doesn't already define
  it (weak-ish default, e.g. `PROVIDE(end = .);` for the heap start).
- **`ASSERT(cond, "msg")`** — fail the link if a constraint is violated (e.g., image too
  big, misaligned vector table).

---

## 11. A Realistic Cortex-M4 Script

```ld
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Top of stack = end of RAM (full-descending stack). */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))     /* keep even without references */
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;              /* end of code in FLASH */
    } > FLASH

    /* Initialized data: runs in RAM, stored after .text in FLASH. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH
    _sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Reserve heap + stack and fail the link if RAM is exhausted. */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        . = . + 0x400;           /* min heap  */
        . = . + 0x800;           /* min stack */
        . = ALIGN(8);
    } > RAM
}
```

This mirrors the vendor `.ld` files shipped for STM32 and other Cortex-M parts — now
you can read them.

---

## 12. Startup Code and the Script Work Together

The linker script only *places* things and *defines boundary symbols*. The **startup
code** (usually `startup.s` or a C `Reset_Handler`) uses those symbols to initialize
RAM before `main()`:

```c
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

void Reset_Handler(void)
{
    /* 1. Copy .data initial values from Flash (LMA) to RAM (VMA). */
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;

    /* 2. Zero the .bss section. */
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;

    /* 3. (call libc init if used) then main(). */
    extern int main(void);
    main();
    for (;;) { }
}
```

The vector table's first word is `_estack` (initial SP) and second is `Reset_Handler` —
both supplied via the script's symbols. This is the exact mechanism referenced in the
Cortex-M4 guide's "Reset and Vector Table" section.

---

## 13. Inspecting the Result

```bash
# Section sizes (text/data/bss) -- does it fit in Flash/RAM?
size program

# Full section table with addresses (VMA/LMA).
readelf -S program
objdump -h program

# Symbol addresses (find where _sdata, _ebss, main landed).
nm program | sort

# Generate a link map showing every section's placement and size.
ld ... -Map=program.map          # or gcc -Wl,-Map=program.map
# then read program.map -- invaluable for debugging placement.

# Convert ELF to raw binary / Intel HEX for flashing.
objcopy -O binary  program program.bin
objcopy -O ihex    program program.hex
```

The **map file** (`-Map`) is the linker-script debugging tool: it shows exactly which
input section from which object went where, and flags overflow.

---

## 14. Common Pitfalls

- **Forgetting `KEEP`** on the vector table (or other hardware-referenced data) → it's
  garbage-collected by `--gc-sections` and the device won't boot.
- **Mixing up VMA and LMA** for `.data` → variables start with garbage because the copy
  source/destination is wrong. Use `> RAM AT> FLASH` + `LOADADDR`.
- **Not zeroing `.bss`** in startup → uninitialized globals aren't actually zero.
- **Wrong `ORIGIN`/`LENGTH`** vs the datasheet → hard faults or "region overflowed".
- Using `*(.text)` only (not `*(.text*)`) with `-ffunction-sections` → per-function
  sections like `.text.main` get dropped/misplaced.
- Assigning `.` **backwards** → linker error.
- Assuming section order in the file; the *script* determines order, not the source.
- On hosted Linux, over-customizing and breaking C runtime startup (`crt0`, `.init_array`)
  — for learning, keep host examples minimal (see the examples).

---

## 15. The Runnable Examples

Under [`examples/linker-scripts/`](examples/linker-scripts/). These run on a normal
Linux host with `gcc`, `ld`, `objdump`, `readelf`, `nm` — **no hardware needed**. They
demonstrate the concepts by *observing* how sections and symbols are placed.

| Folder | Teaches |
|--------|---------|
| `01-sections-explorer` | Compile a program, then use `size`/`readelf`/`nm`/`objdump` to see `.text/.data/.bss/.rodata` and symbol addresses |
| `02-custom-section` | Place a variable in a custom section with `__attribute__((section))` and a small linker script fragment; find it with `readelf`/`nm` |
| `03-map-and-symbols` | Generate a `-Map` file and script-defined boundary symbols (`_sdata`, `_ebss`), then read them back in C |

Each folder has a `Makefile` and `run.sh`:
```bash
cd examples/linker-scripts/01-sections-explorer
make            # build + print the analysis
make clean
```

---

## 16. Further References

- **GNU ld Manual** (the "LD" info manual): https://sourceware.org/binutils/docs/ld/ —
  read "Scripts", "Memory Layout" (MEMORY), "SECTIONS", and "Location Counter".
- **"Linkers and Loaders"** by John R. Levine — the definitive book on how linking works.
- **Ian Lance Taylor's "Linkers" blog series** (20 parts) — deep, readable internals:
  https://lwn.net/Articles/276782/ (index) / airs.com/blog.
- **ARM/vendor startup + `.ld` files** (e.g. STM32Cube `*_FLASH.ld`) — real-world
  scripts you can now read line by line.
- Binutils tools: `man ld`, `man objcopy`, `man readelf`, `man nm`, `man objdump`.
- Cross-reference the **Cortex-M4 guide** (`cortex-m4-architecture-guide.md`) sections on
  the memory map, reset, and vector table to see the hardware side.

**Practice ideas:** write a minimal bare-metal `.ld` + startup for a QEMU Cortex-M
target (e.g., `qemu-system-arm -M lm3s6965evb`); place a function in RAM and verify its
address; add an `ASSERT` that the image fits in Flash and watch it fire when you exceed
`LENGTH`.
