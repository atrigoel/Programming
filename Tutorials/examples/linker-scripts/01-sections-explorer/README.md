# Linker Example 01 — Sections Explorer

Runs on a normal Linux host (no hardware). Builds a small program and inspects how the
linker sorts globals into `.text`, `.rodata`, `.data`, and `.bss`.

## Run

```bash
make            # builds ./explorer and prints the section analysis
./explorer      # run it
make clean
```

## What to notice

- **`size explorer`** shows three numbers: `text` (code + rodata), `data`
  (initialized globals), `bss` (zeroed globals). `big_buffer[4096]` inflates **bss**
  but **not** the on-disk file — `.bss` reserves RAM without storing bytes.
- **`readelf -S`** lists each section's address and size.
- **`nm`** shows each symbol's section letter:
  - `T`/`t` → `.text`, `R`/`r` → `.rodata`, `D`/`d` → `.data`, `B`/`b` → `.bss`.
- Change `initialized_global` to `= 0` and rebuild: it migrates from `.data` to `.bss`
  (zero-initialized globals need no stored value).
