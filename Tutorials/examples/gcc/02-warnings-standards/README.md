# GCC Example 02 — Warnings & Language Standard

`main.c` is deliberately sloppy. Watch how each warning level exposes more problems.

## Run

```bash
make lax        # builds quietly — most bugs slip through
make strict     # -Wall -Wextra -Wshadow -Wconversion: several warnings appear
make pedantic   # -std=c11 -Wpedantic: strict ISO conformance
make werror     # -Werror: build FAILS (warnings become errors) — expected!
make clean
```

## What to notice

- **`-Wall` is not "all warnings."** It's a curated common set. `-Wextra` adds more, and
  even together they don't enable `-Wshadow` or `-Wconversion` — you opt into those.
- **`-Wshadow`** flags the inner `total` hiding the outer one.
- **`-Wconversion`** flags `unsigned int u = -1;` (a value-changing implicit conversion).
- **`-Wreturn-type`** (in `-Wall`) flags `compute()` falling off the end without a return.
- **`make werror` fails on purpose** — this is exactly how CI stops sloppy code from
  merging. Fix the warnings and it passes.
- Swap `-std=c11` for `-std=gnu11` to allow GNU extensions under `-pedantic`.
