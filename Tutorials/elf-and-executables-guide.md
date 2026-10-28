# ELF, Object Files & Flashable Images — A Detailed Guide

> Every guide so far *produced* files — `.o`, `.elf`, `.bin`, `.hex` — but never
> explained **what they actually are**. This guide fills that gap. It covers the
> **ELF** format that Linux uses for objects, executables, shared libraries and core
> dumps; the **raw and hex image formats** used to program microcontrollers; and the
> **tools and options** to inspect files and convert between them.
>
> It ties together the toolchain guides:
> [gcc-guide.md](gcc-guide.md) (produces objects/ELF),
> [linker-scripts-guide.md](linker-scripts-guide.md) (places sections),
> [gdb-guide.md](gdb-guide.md) (reads symbols/DWARF) and
> [openocd-guide.md](openocd-guide.md) (flashes the image).

---

## Table of Contents

- [ELF, Object Files \& Flashable Images — A Detailed Guide](#elf-object-files--flashable-images--a-detailed-guide)
  - [Table of Contents](#table-of-contents)
  - [1. What "executable" means on Linux](#1-what-executable-means-on-linux)
  - [2. The ELF format at a glance](#2-the-elf-format-at-a-glance)
  - [3. ELF file types (`ET_REL`, `ET_EXEC`, `ET_DYN`, `ET_CORE`)](#3-elf-file-types-et_rel-et_exec-et_dyn-et_core)
  - [4. The ELF header](#4-the-elf-header)
  - [5. Sections vs segments (the two views)](#5-sections-vs-segments-the-two-views)
  - [6. The common sections (`.text`, `.data`, `.bss`, `.rodata`, …)](#6-the-common-sections-text-data-bss-rodata-)
  - [7. Symbol table \& relocations](#7-symbol-table--relocations)
  - [8. Dynamic linking: PLT, GOT, `.interp`, `RUNPATH`](#8-dynamic-linking-plt-got-interp-runpath)
  - [9. Debug info (DWARF) and stripping](#9-debug-info-dwarf-and-stripping)
  - [10. Other executable/container formats](#10-other-executablecontainer-formats)
  - [11. Inspecting files: the binutils toolbox](#11-inspecting-files-the-binutils-toolbox)
  - [12. Embedded images: why not just flash the ELF?](#12-embedded-images-why-not-just-flash-the-elf)
  - [13. Raw binary (`.bin`)](#13-raw-binary-bin)
  - [14. Intel HEX (`.hex`)](#14-intel-hex-hex)
  - [15. Motorola S-record (`.srec`/`.s19`/`.mot`)](#15-motorola-s-record-srecs19mot)
  - [16. Other image formats (TI-TXT, Verilog, ELF-on-target, UF2, DFU)](#16-other-image-formats-ti-txt-verilog-elf-on-target-uf2-dfu)
  - [17. Converting between formats with `objcopy`](#17-converting-between-formats-with-objcopy)
  - [18. Converting with `srec_cat` (the swiss-army knife)](#18-converting-with-srec_cat-the-swiss-army-knife)
  - [19. Gaps, fill bytes, addresses \& checksums](#19-gaps-fill-bytes-addresses--checksums)
  - [20. Round-tripping: hex/bin → ELF](#20-round-tripping-hexbin--elf)
  - [21. A complete embedded post-build recipe](#21-a-complete-embedded-post-build-recipe)
  - [22. Quick reference tables](#22-quick-reference-tables)
  - [23. Common pitfalls](#23-common-pitfalls)
  - [24. Further References](#24-further-references)

---

## 1. What "executable" means on Linux

On Linux an "executable" is just a file the kernel knows how to load. The kernel
inspects the first few bytes (the **magic number**) and hands the file to the right
loader:

| Magic (first bytes) | Format | Handled by |
|---------------------|--------|-----------|
| `7F 45 4C 46` (`\x7fELF`) | **ELF** | kernel ELF loader (`binfmt_elf`) |
| `23 21` (`#!`) | script shebang | `binfmt_script` → named interpreter |
| `4D 5A` (`MZ`) | Windows PE | Wine / `binfmt_misc` |
| `CA FE BA BE` / `CF FA ED FE` | Mach-O (macOS) | not native on Linux |

For native compiled programs on Linux the answer is almost always **ELF**
(Executable and Linkable Format). It is the single format used for **object files,
executables, shared libraries, and core dumps** — one container, four roles.

> The very first tool to reach for is `file`:
> ```bash
> file /bin/ls
> # ELF 64-bit LSB pie executable, x86-64, ... dynamically linked, ...
> ```

---

## 2. The ELF format at a glance

An ELF file has up to four parts:

```
┌─────────────────────────┐
│      ELF header         │  who am I? (type, machine, entry, table offsets)
├─────────────────────────┤
│  Program header table   │  SEGMENTS  — how to LOAD at run time  (used by the OS loader)
├─────────────────────────┤
│                         │
│      Section data       │  .text .rodata .data .bss .symtab .debug_* ...
│                         │
├─────────────────────────┤
│  Section header table   │  SECTIONS  — how to LINK / inspect     (used by ld, gdb, objdump)
└─────────────────────────┘
```

Two independent "tables of contents" describe the same bytes:

- **Section headers** → the *linking* view (fine-grained: `.text`, `.data`, `.symtab`, `.debug_info`, …). Used by `ld`, `gdb`, `objdump`, `readelf`.
- **Program headers** → the *execution* view (coarse: a handful of loadable **segments**). Used by the kernel/dynamic loader.

A relocatable object (`.o`) has **sections only**; a runnable file has **both**.

---

## 3. ELF file types (`ET_REL`, `ET_EXEC`, `ET_DYN`, `ET_CORE`)

The `e_type` field in the header says what the file is for:

| `e_type` | Name | What it is | Produced by |
|----------|------|-----------|-------------|
| `ET_REL` | Relocatable | An **object file** (`.o`) or static lib member: sections + relocations, not yet placed in memory | compiler/assembler (`gcc -c`, `as`) |
| `ET_EXEC` | Executable | A **fixed-address** program (classic non-PIE, and typical for bare-metal firmware) | `ld` |
| `ET_DYN` | Shared object | A **position-independent** file: shared library (`.so`) **or** a PIE executable | `ld -shared` / `-pie` |
| `ET_CORE` | Core dump | Snapshot of a crashed process's memory + registers | kernel on crash |

> Modern distros build normal programs as **PIE** (`ET_DYN`) for ASLR, which is why
> `file /bin/ls` says "pie executable" — it is technically a shared object with an
> entry point. See the ELF core-dump workflow in
> [gdb-guide.md](gdb-guide.md) §4.

---

## 4. The ELF header

The first 64 bytes (on 64-bit) identify everything else. Read it with `readelf -h`:

```bash
readelf -h /bin/ls
```

```
Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
Class:                             ELF64            # 32- or 64-bit
Data:                              2's complement, little endian
Type:                              DYN (Position-Independent Executable)
Machine:                           Advanced Micro Devices X86-64
Entry point address:               0x6a80           # where execution starts
Start of program headers:          64 (bytes)
Start of section headers:          140816 (bytes)
Number of program headers:         13
Number of section headers:         31
```

Key fields:

- **`e_ident`** — the 16-byte identifier: magic `\x7fELF`, **class** (32/64-bit), **endianness**, ABI.
- **`e_machine`** — target ISA: `EM_X86_64` (0x3E), `EM_ARM` (0x28), `EM_AARCH64` (0xB7), `EM_RISCV` (0xF3). This is why an ARM ELF won't run on x86.
- **`e_entry`** — virtual address of the first instruction (for Cortex-M this is usually the reset handler reached via the vector table, not `e_entry`).
- **`e_phoff` / `e_shoff`** — file offsets of the program- and section-header tables.

---

## 5. Sections vs segments (the two views)

This is the single most important concept and the usual source of confusion.

```
   LINKING VIEW (sections)          EXECUTION VIEW (segments)
   fine-grained, for tools          coarse, for the loader

   .text   ┐
   .rodata ┘ ───────────────▶  LOAD segment #1  (R-X)   ← code + constants
   .data   ┐
   .bss    ┘ ───────────────▶  LOAD segment #2  (RW-)   ← writable data
   .dynamic ───────────────▶  DYNAMIC segment          ← info for the dynamic linker
   .debug_* ──(not loaded)──▶  (no segment)             ← only for debuggers
```

- One **segment** groups several **sections** with the same permissions so the kernel
  can `mmap` them in a few calls.
- Sections like `.debug_info` or `.symtab` belong to **no** segment — they are never
  loaded into RAM, which is exactly why you can **strip** them without changing behaviour.

See it yourself:

```bash
readelf -l a.out     # program headers + the "Section to Segment mapping"
readelf -S a.out     # section headers
```

---

## 6. The common sections (`.text`, `.data`, `.bss`, `.rodata`, …)

| Section | Contents | Loaded? | Occupies file space? |
|---------|----------|---------|----------------------|
| `.text` | Executable machine code | yes (R-X) | yes |
| `.rodata` | Read-only constants, string literals, jump tables | yes (R--) | yes |
| `.data` | Initialised **writable** globals/statics | yes (RW-) | yes (the initial values) |
| `.bss` | **Zero-initialised** writable globals/statics | yes (RW-) | **no** — only a size; zeroed at startup |
| `.symtab` / `.strtab` | Symbol table + names (for linking/debug) | no | yes |
| `.dynsym` / `.dynstr` | Symbols needed at **run time** for dynamic linking | yes | yes |
| `.rela.*` / `.rel.*` | Relocation entries | no | yes |
| `.init` / `.fini` / `.init_array` | Constructors/destructors run before/after `main` | yes | yes |
| `.debug_*` | DWARF debug info | no | yes (until stripped) |
| `.comment` / `.note.*` | Toolchain metadata, build-id | no | yes |

> **Why `.bss` is "free" in flash/on disk:** it stores only a length. The C runtime
> (`crt0`/startup) zeroes that RAM range before `main`. On embedded targets the startup
> code also **copies `.data` from flash (its LMA) into RAM (its VMA)** — the load-vs-run
> address split explained in [linker-scripts-guide.md](linker-scripts-guide.md) §9.

---

## 7. Symbol table & relocations

A **symbol** names an address: a function, a global, a section. A **relocation** is a
"fill this in later" note the linker resolves.

```bash
nm a.out                 # symbols: T=text U=undefined B=bss D=data ...
nm -D libfoo.so          # dynamic symbols (what the .so exports/imports)
readelf -s a.out         # full symbol table with sizes/binding/visibility
readelf -r a.out         # relocation entries
objdump -dr a.out        # disassembly *with* relocations annotated
```

- **Undefined (`U`)** symbols in an object are what the linker must satisfy from other
  objects/libraries — the source of the classic *"undefined reference to …"* error
  (see [gcc-guide.md](gcc-guide.md) §8 on link order).
- Relocation **types** are ISA-specific (`R_X86_64_PC32`, `R_ARM_THM_CALL`, …) and
  describe *how* to patch the bytes (absolute, PC-relative, GOT-relative).

---

## 8. Dynamic linking: PLT, GOT, `.interp`, `RUNPATH`

A dynamically linked program doesn't contain libc — it asks the **dynamic linker** to
load it at start-up.

```bash
readelf -p .interp a.out      # e.g. /lib64/ld-linux-x86-64.so.2  (the loader)
ldd a.out                     # which shared libs get pulled in (and from where)
readelf -d a.out              # .dynamic: NEEDED libs, RUNPATH, SONAME, symbol tables
```

- **`.interp`** names the loader the kernel `exec`s first.
- **`.dynamic`** lists `DT_NEEDED` libraries, `DT_SONAME`, and search paths
  (`DT_RUNPATH`/`DT_RPATH`).
- **GOT** (Global Offset Table) holds resolved addresses of external symbols;
  **PLT** (Procedure Linkage Table) is the trampoline that fills the GOT lazily on the
  first call.

> Security note: prefer `DT_RUNPATH` over the legacy `DT_RPATH`, and never bake an
> attacker-writable directory (e.g. `.` or a world-writable path) into it — that is a
> classic library-injection vector. Use `$ORIGIN` for relocatable install trees.

---

## 9. Debug info (DWARF) and stripping

Debug information lives in `.debug_*` sections in **DWARF** format — line tables,
variable types, scopes — everything `gdb` needs to map addresses back to source.

```bash
readelf -S a.out | grep debug     # see the .debug_* sections
objdump --dwarf=info a.out        # dump DWARF (verbose)
```

Because debug sections are **not loaded**, you can remove them to shrink the file:

```bash
strip a.out                       # remove symbols + debug (in place)
strip --strip-debug a.out         # keep .symtab, drop only DWARF
strip -s -o a.stripped a.out      # write a stripped copy, keep the original
```

The professional pattern is **split debug info** — ship a small stripped binary, keep
symbols separately for post-mortem:

```bash
objcopy --only-keep-debug a.out a.out.debug     # extract debug to a sidecar
strip --strip-debug --strip-unneeded a.out       # slim the shipped binary
objcopy --add-gnu-debuglink=a.out.debug a.out    # link them so gdb finds the sidecar
```

> Keep the **unstripped** `.elf` for embedded work: GDB and OpenOCD read its symbols
> even though you flash a stripped image. See [gdb-guide.md](gdb-guide.md) §intro.

---

## 10. Other executable/container formats

You'll mostly meet ELF, but it helps to recognise the neighbours:

| Format | Where | Notes |
|--------|-------|-------|
| **ELF** | Linux, most Unix, bare-metal ARM/RISC-V | The subject of this guide |
| **PE/COFF** (`.exe`, `.dll`) | Windows | Runs on Linux via Wine; cross-built with MinGW |
| **Mach-O** | macOS/iOS | Fat/universal binaries bundle multiple arches |
| **`a.out`** | ancient Unix | Obsolete; the name survives only as the default output |
| **Flat binary** | small bootloaders, DOS `.com`, some RTOS | No headers at all — pure load-and-jump; the `.bin` below is essentially this |
| **Script + shebang** | `#!/usr/bin/env python3` | The kernel runs the named interpreter |

Static archives (`.a`) are not an executable format — they're just an `ar` bundle of
`ET_REL` objects with an index (`ar t libfoo.a`, `ar x libfoo.a`).

---

## 11. Inspecting files: the binutils toolbox

The GNU **binutils** are your microscope. All exist as cross versions
(`arm-none-eabi-readelf`, etc.).

| Tool | Answers the question |
|------|----------------------|
| `file` | What kind of file is this at all? |
| `readelf -h/-l/-S/-s/-d/-r` | Everything the ELF headers say (header/segments/sections/symbols/dynamic/relocs) |
| `objdump -d/-h/-t/-s` | Disassemble, list sections, symbols, hex-dump section bytes |
| `nm` | Symbol list (quick "is this symbol defined?") |
| `size` | Byte counts of `.text`/`.data`/`.bss` → **flash & RAM budget** |
| `strings` | Printable strings (find version banners, paths) |
| `ldd` | Shared-library dependencies (hosted only) |
| `addr2line` | Map a crash address back to `file:line` (needs symbols) |
| `c++filt` | Demangle C++ symbol names |
| `objcopy` | **Convert/strip/patch** — the format converter of §17 |

Two everyday one-liners:

```bash
size fw.elf            # text data bss dec hex — does it fit in flash/RAM?
objdump -d fw.elf | less   # read the actual machine code
```

---

## 12. Embedded images: why not just flash the ELF?

An ELF is a *rich container*: headers, symbol tables, debug info, and — crucially —
**addresses that assume an OS loader**. A microcontroller has no loader. It just wants
raw bytes placed at specific flash addresses, starting at the reset vector.

So the final build step **strips the ELF down to only its loadable content**, expressed
in a format the flasher/programmer understands:

```
   fw.elf  ──objcopy──▶  fw.bin   (raw bytes, no addresses)
           ──objcopy──▶  fw.hex   (Intel HEX: bytes + addresses + checksums)
           ──objcopy──▶  fw.srec  (Motorola S-record: same idea, different syntax)
```

What gets dropped: section headers, `.symtab`, `.debug_*`, anything not in a `LOAD`
segment. What's kept: the bytes of `.text`, `.rodata`, and the **initial values** of
`.data` (placed at their **LMA**, i.e. flash). `.bss` contributes **nothing** to the
image — it's just zeroed RAM at startup.

> The ELF's **VMA vs LMA** distinction is why `.data` ends up in the flash image even
> though it *runs* from RAM. Re-read [linker-scripts-guide.md](linker-scripts-guide.md)
> §9 if that split isn't crystal clear — it's the crux of every embedded image.

---

## 13. Raw binary (`.bin`)

The simplest image: **just the bytes**, in order, with **no address or type
information** at all.

```bash
arm-none-eabi-objcopy -O binary fw.elf fw.bin
```

- **Pros:** smallest possible; trivial; ideal for tools that take a load address
  separately (`st-flash write fw.bin 0x08000000`, or a bootloader that copies to a
  fixed offset).
- **Cons:** because it carries **no address**, you must tell the flasher where it goes.
  If your image has a **gap** (e.g. code at `0x08000000` and a config blob at
  `0x0800F000`), `objcopy` fills the gap with `0xFF` (or your `--gap-fill`) so the file
  can stay contiguous — which can make a `.bin` far larger than a `.hex` of the same
  program.

Flashing a `.bin` always needs an explicit base address (see
[openocd-guide.md](openocd-guide.md) §program: `program fw.bin 0x08000000`).

---

## 14. Intel HEX (`.hex`)

The most common MCU format. **ASCII text**, where each line ("record") carries a
**byte count, a 16-bit address, a record type, the data, and a checksum**. Because it
encodes addresses, it can represent **non-contiguous** regions compactly and is
**self-locating** — the flasher knows where every byte goes.

```bash
arm-none-eabi-objcopy -O ihex fw.elf fw.hex
```

Record line format:

```
:LLAAAATT[DD…]CC
 │ │   │  └ data bytes (LL of them)      │
 │ │   └ record type                     └ 2's-complement checksum of all prior bytes
 │ └ 16-bit load address (this record)
 └ byte count (LL)
: start-of-record marker
```

Record **types**:

| Type | Name | Purpose |
|------|------|---------|
| `00` | Data | Up to 255 bytes at the 16-bit address |
| `01` | End Of File | Always the last line: `:00000001FF` |
| `02` | Extended Segment Address | ×16 base (legacy 20-bit addressing) |
| `04` | Extended Linear Address | Sets the **upper 16 bits** of a 32-bit address (how flash at `0x08000000` is expressed) |
| `05` | Start Linear Address | Optional entry-point / initial PC |

Example (two data bytes `0x3F 0x7C` at address `0x0100`, then EOF):

```
:020100003F7C41
:00000001FF
```

- **Pros:** carries addresses + checksums; human-readable; the de-facto default for
  ST-LINK Utility, avrdude, most bootloaders and gang programmers.
- **Cons:** ~2× the size of the raw bytes (it's ASCII hex); still needs the right
  extended-address records for >64 KiB targets (objcopy emits them automatically).

---

## 15. Motorola S-record (`.srec`/`.s19`/`.mot`)

The other classic ASCII format — same idea as Intel HEX, different record letters, and
**selectable address width** (16/24/32-bit), which some NXP/Freescale, TI and automotive
toolchains prefer.

```bash
arm-none-eabi-objcopy -O srec fw.elf fw.srec
arm-none-eabi-objcopy -O srec --srec-forceS3 fw.elf fw.srec   # force 32-bit addr records
```

Record types:

| Record | Meaning |
|--------|---------|
| `S0` | Header (usually the file name / comment) |
| `S1` / `S2` / `S3` | Data with **16 / 24 / 32-bit** address |
| `S5` / `S6` | Count of data records (integrity check) |
| `S7` / `S8` / `S9` | Start address (entry) for 32 / 24 / 16-bit |

Line layout mirrors Intel HEX: `S<type><bytecount><address><data><checksum>`.
File extensions `.srec`, `.s19`, `.s28`, `.s37`, `.mot`, `.exo` are all S-record.

> Rule of thumb: **Intel HEX** dominates ARM Cortex-M/AVR/PIC; **S-record** shows up in
> NXP/automotive/older Motorola ecosystems. `objcopy` speaks both, so you convert freely.

---

## 16. Other image formats (TI-TXT, Verilog, ELF-on-target, UF2, DFU)

| Format | `objcopy -O` name | Where it's used |
|--------|-------------------|-----------------|
| **TI-TXT** | `tektronix` (close) / vendor tools | TI MSP430 (`.txt` with `@addr` + hex) |
| **Verilog hex** | `verilog` | `$readmemh` for FPGA/RTL simulation memories |
| **ELF on target** | (keep the ELF) | JTAG/SWD debuggers (OpenOCD, J-Link) load ELF directly — they read its LOAD segments |
| **UF2** | external tool (`uf2conv.py`) | RP2040, many bootloaders — drag-and-drop mass-storage flashing |
| **DFU** | `dfu-suffix`/`dfu-util` | USB Device Firmware Upgrade (STM32 system bootloader) |
| **Binary + header** | vendor packagers | Signed/OTA images: raw `.bin` wrapped with CRC/signature/version |

For SWD/JTAG the winner is often the **ELF itself** — debuggers use its addresses and
symbols directly, so you skip conversion entirely:

```bash
openocd ... -c "program fw.elf verify reset exit"     # ELF straight to flash
```

See [openocd-guide.md](openocd-guide.md) for the full flash/verify/reset flow.

---

## 17. Converting between formats with `objcopy`

`objcopy` copies an object file while optionally **changing the output format**,
**stripping**, **filling gaps**, or **moving addresses**. It's the primary converter.

Core syntax:

```bash
objcopy -I <in-fmt> -O <out-fmt> [options] input output
# -I is usually auto-detected for ELF; you must give -O for bin/hex/srec.
```

Everyday conversions (host `objcopy` or `arm-none-eabi-objcopy`):

```bash
# ELF → raw / hex / srec
objcopy -O binary  fw.elf fw.bin
objcopy -O ihex    fw.elf fw.hex
objcopy -O srec    fw.elf fw.srec

# hex ↔ srec ↔ bin  (formats convert directly, no ELF needed)
objcopy -I ihex -O binary fw.hex fw.bin
objcopy -I ihex -O srec   fw.hex fw.srec
objcopy -I srec -O ihex   fw.srec fw.hex
```

The options that matter for embedded:

| Option | Effect |
|--------|--------|
| `-O binary\|ihex\|srec\|verilog` | Output format |
| `-I ihex\|srec\|binary\|elf32-littlearm` | Input format (needed when input isn't ELF) |
| `-j .text -j .data` | Keep **only** these sections in the output |
| `-R .comment -R .note*` | **Remove** sections |
| `--gap-fill 0xFF` | Value used to fill address gaps (flash erases to `0xFF`) |
| `--pad-to 0x08010000` | Pad the image up to an address (fixed-size images/CRC regions) |
| `--change-addresses 0x08000000` | Shift all addresses (turn a 0-based `.bin` view into flash-based) |
| `--set-start 0x08000000` | Set the recorded entry/start address |
| `-S` / `--strip-all`, `-g`/`--strip-debug` | Strip while copying |
| `--srec-len=32 --srec-forceS3` | S-record line length / force 32-bit address records |
| `--verilog-data-width=4` | Byte width per Verilog word |

> **`-O binary` loses addresses.** When you later convert a `.bin` *back* to hex you
> must re-supply the base address (`--change-addresses`, see §20), because the raw file
> never stored one.

---

## 18. Converting with `srec_cat` (the swiss-army knife)

When `objcopy` isn't enough — merging multiple images, inserting CRCs, complex
offsets — the **SRecord** suite's `srec_cat`/`srec_info` are the tools professionals
reach for. (Package: `srecord`.)

```bash
srec_info  fw.hex -intel                       # summarise address ranges & records
```

```bash
# Convert (input format then output format; order matters)
srec_cat fw.hex -intel          -o fw.bin  -binary
srec_cat fw.bin -binary -offset 0x08000000 -o fw.hex  -intel   # give a raw bin an address
srec_cat fw.hex -intel          -o fw.srec -motorola
```

```bash
# Merge application + bootloader + config into one hex
srec_cat boot.hex -intel  app.hex -intel  config.hex -intel \
         -o combined.hex -intel
```

```bash
# Fill unused flash with 0xFF over a range, then append a CRC16 at the end
srec_cat fw.hex -intel \
         -fill 0xFF 0x08000000 0x08020000 \
         -CRC16_Big_Endian 0x0801FFFE \
         -o fw_crc.hex -intel
```

`srec_cat` understands Intel HEX, S-record, TI-TXT, binary, Verilog and more, and can
crop (`-crop`), exclude (`-exclude`), byte-swap, and offset arbitrarily — it's the tool
for production programming files.

---

## 19. Gaps, fill bytes, addresses & checksums

Three things trip people up when converting:

1. **Gaps between regions.** `.bin` is *contiguous*, so a gap between two placed regions
   gets **filled** (default `0xFF`, matching erased NOR flash). A program with code at
   `0x08000000` and a 1-page config at `0x0800F000` becomes a ~60 KiB `.bin` of mostly
   `0xFF`, but a tiny `.hex`/`.srec` (which simply skip the gap). Prefer hex/srec when
   your layout is sparse.

2. **Addresses.** Hex/srec are **self-locating**; `.bin` is not. Match the format to the
   tool: `st-flash`/`dfu-util` take a `.bin` + explicit address; ST-LINK Utility /
   avrdude / bootloaders take a `.hex` that carries its own addresses.

3. **Checksums.** The per-record checksums in hex/srec only guard **transmission**. A
   whole-image **integrity CRC/signature** (for a bootloader to validate before jumping)
   is a *separate* concern you add with `srec_cat -CRC*` or a vendor packager — don't
   confuse the two.

---

## 20. Round-tripping: hex/bin → ELF

Sometimes you only have a `.hex`/`.bin` (a vendor blob) and want to inspect or
disassemble it. You can wrap it back into an ELF so `objdump`/`gdb` can chew on it.

```bash
# hex → ELF (addresses preserved from the hex records)
objcopy -I ihex -O elf32-littlearm fw.hex fw.elf

# raw bin → ELF: you MUST supply the base address the bytes belong at
objcopy -I binary -O elf32-littlearm \
        --change-addresses 0x08000000 \
        --rename-section .data=.text,alloc,load,code \
        fw.bin fw_from_bin.elf

# now disassemble (tell objdump the CPU, since a wrapped bin has no arch metadata)
arm-none-eabi-objdump -D -b elf32-littlearm -m arm fw_from_bin.elf
```

Caveats:

- A reconstructed ELF has **no symbols and no DWARF** — you get raw disassembly only.
- For a raw `.bin` you must **know the architecture and load address**; nothing in the
  file tells you. Getting either wrong yields nonsense disassembly.
- `objdump -D -b binary -m arm --adjust-vma=0x08000000 fw.bin` disassembles a raw bin
  directly, without the ELF wrap, if you just want a quick look.

---

## 21. A complete embedded post-build recipe

Putting it together — the standard tail of a Cortex-M build (mirrors
[gcc-guide.md](gcc-guide.md) §12 and the CMake post-build in
[cmake-embedded-guide.md](cmake-embedded-guide.md) §6):

```makefile
PREFIX  = arm-none-eabi-
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

fw.hex: fw.elf
	$(OBJCOPY) -O ihex $< $@

fw.bin: fw.elf
	$(OBJCOPY) -O binary $< $@

fw.srec: fw.elf
	$(OBJCOPY) -O srec --srec-forceS3 $< $@

size: fw.elf
	$(SIZE) $<                 # text data bss → flash = text+data, RAM = data+bss
```

The reasoning:

1. Compile/link produces **`fw.elf`** — the master artefact (symbols + DWARF).
   Keep it for [gdb-guide.md](gdb-guide.md) / [openocd-guide.md](openocd-guide.md).
2. **`objcopy -O ihex/binary/srec`** derives the flashable images.
3. **`size`** verifies it fits: `flash usage = text + data`, `RAM usage = data + bss`.
4. Flash with whichever the programmer wants — ELF/hex over SWD, or bin + address.

---

## 22. Quick reference tables

**File → what it is**

| Ext | Format | Addresses? | Text/Binary | Typical use |
|-----|--------|-----------|-------------|-------------|
| `.o` | ELF `ET_REL` | relative | binary | Compiler output, pre-link |
| `.a` | `ar` archive | — | binary | Static library (bundle of `.o`) |
| `.so` | ELF `ET_DYN` | virtual | binary | Shared library |
| `.elf` (no ext / `a.out`) | ELF `ET_EXEC`/`ET_DYN` | virtual | binary | Runnable / firmware master |
| `.bin` | Raw | **none** | binary | Flash blob (+ external load addr) |
| `.hex` | Intel HEX | yes | **ASCII** | MCU flashing (most common) |
| `.srec`/`.s19`/`.mot` | Motorola S-record | yes | **ASCII** | NXP/automotive flashing |
| `core` | ELF `ET_CORE` | — | binary | Crash dump for `gdb` |

**Conversion cheat-sheet**

| Goal | Command |
|------|---------|
| ELF → bin | `objcopy -O binary fw.elf fw.bin` |
| ELF → Intel HEX | `objcopy -O ihex fw.elf fw.hex` |
| ELF → S-record | `objcopy -O srec fw.elf fw.srec` |
| HEX → bin | `objcopy -I ihex -O binary fw.hex fw.bin` |
| bin → HEX (needs addr) | `srec_cat fw.bin -binary -offset 0x08000000 -o fw.hex -intel` |
| HEX ↔ SREC | `objcopy -I ihex -O srec fw.hex fw.srec` |
| Merge images | `srec_cat a.hex -intel b.hex -intel -o all.hex -intel` |
| Fill + CRC | `srec_cat fw.hex -intel -fill 0xFF S E -CRC16_Big_Endian A -o out.hex -intel` |
| Inspect ranges | `srec_info fw.hex -intel` |

**Inspection cheat-sheet**

| Goal | Command |
|------|---------|
| What is this file? | `file X` |
| ELF header | `readelf -h X` |
| Sections | `readelf -S X` / `objdump -h X` |
| Segments + mapping | `readelf -l X` |
| Symbols | `nm X` / `readelf -s X` |
| Dynamic deps | `ldd X` / `readelf -d X` |
| Disassemble | `objdump -d X` |
| Flash/RAM size | `size X` |
| Addr → line | `addr2line -e X 0x1234` |

---

## 23. Common pitfalls

- **Flashing the `.elf` to a raw-`.bin` tool.** The ELF has headers/symbols; a tool
  expecting raw bytes will write garbage. Convert first (or use an ELF-aware programmer).
- **`.bin` with no address.** Forgetting the base address (`st-flash write fw.bin 0x08000000`)
  writes to the wrong place or offset 0. Prefer `.hex` when in doubt.
- **Huge `.bin` from a sparse layout.** A gap between regions is fill-padded. Use
  `.hex`/`.srec`, or `objcopy -j` only the sections you need.
- **Wrong endianness/arch on round-trip.** Wrapping a raw `.bin` into ELF for
  disassembly needs the correct `-m`/`-b`; otherwise the disassembly is meaningless.
- **`.data` "missing" from RAM.** It lives at its **LMA** in the image; startup code
  copies it to its **VMA** in RAM. If your startup omits the copy loop, initialised
  globals are wrong — a linker-script/startup bug, not an image bug.
- **Confusing record checksums with image integrity.** Intel/S-record per-line checksums
  don't protect against a corrupt *program*; add a real CRC/signature for the bootloader.
- **Stripping the only copy.** `strip fw.elf` in place destroys your symbols. Keep an
  unstripped master or use `--only-keep-debug` first.

---

## 24. Further References

- **ELF specification** — *Tool Interface Standard (TIS) ELF v1.2* and the
  System V ABI (generic + processor supplements, e.g. ARM/AArch64/RISC-V psABIs).
- **DWARF** — [dwarfstd.org](https://dwarfstd.org) for the debug-info format.
- **binutils manuals** — `man objcopy`, `man readelf`, `man objdump`, `man nm`,
  `man strip`, `man size`, `info binutils`.
- **SRecord** — `man srec_cat`, `man srec_info`, and the SRecord manual for the full
  list of formats, fills, CRCs and byte operations.
- **Intel HEX** and **Motorola S-record** — the original format specifications for the
  exact record layouts.
- Local companions: [gcc-guide.md](gcc-guide.md) §12,
  [linker-scripts-guide.md](linker-scripts-guide.md) §9,
  [gdb-guide.md](gdb-guide.md), [openocd-guide.md](openocd-guide.md),
  [cmake-embedded-guide.md](cmake-embedded-guide.md) §6.
