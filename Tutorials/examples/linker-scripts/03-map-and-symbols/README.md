# Linker Example 03 — Map File & Symbols

The **map file** (`-Wl,-Map=`) is the linker's report of exactly where every section
and symbol ended up — the essential tool for debugging placement and image size.

## Run

```bash
make            # builds ./mapdemo + mapdemo.map, prints highlights
less mapdemo.map    # read the full report
make clean
```

## What to notice

- **`-Wl,-Map=mapdemo.map`** produces the map. It contains the memory configuration,
  the linker-script-to-output mapping, and a symbol cross-reference.
- **`--gc-sections`** (plus `-ffunction-sections -fdata-sections`) discards unused
  code/data. Because `helper()` *is* used here it stays; add an unused function and
  confirm it's dropped in the map.
- **`nm -n`** lists symbols sorted by address — see `table` (in `.data`) vs `scratch`
  (in `.bss`) vs `main` (in `.text`) at very different addresses.
- The map file is how you answer "why is my firmware too big?" and "which object pulled
  in that huge symbol?" — search it for the largest contributors.
