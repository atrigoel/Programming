# GDB Example 03 — Conditional Breakpoints & `dprintf`

Stop only when it matters, and trace without recompiling. Covers
[gdb-guide §4](../../../gdb-guide.md#4-conditional--counted-breakpoints) and
[§8](../../../gdb-guide.md#8-breakpoint-command-lists--dprintf).

`main.c` runs `process()` five times; only item id=3 (value `-1`) is interesting.

## Run the scripted demo

```bash
make demo
```

It shows three escalating techniques on the same program:

1. **Conditional breakpoint** — `break process if it->value < 0` fires *only* on the bad
   item, skipping the four good ones. `print *it` confirms you caught id=3.
2. **Breakpoint command list** — attach `silent` + `printf` + `continue` so the breakpoint
   auto-logs every call and keeps running (tracing without stopping).
3. **`dprintf`** — the same trace in a single line, with no source edit and no rebuild.

## What to notice

- A **condition** is any C expression in the program's context (`it->value < 0`), so you
  filter a hot loop down to the one case you care about.
- **`ignore N COUNT`** is the faster cousin when you just want the Nth hit — GDB doesn't
  evaluate an expression each time.
- **`dprintf LOC, FMT, ARGS`** is "add a log line" without touching the code. On embedded
  this means logging in **flashed firmware with no UART and no reflash** — see
  [gdb-guide §8](../../../gdb-guide.md#8-breakpoint-command-lists--dprintf).

## Now you try (interactive)

```bash
make debug
```
```gdb
break process if it->id == 4      # stop on a specific id
run
print *it
delete
break process
ignore 1 2                        # skip the first 2 calls, stop on the 3rd
run
print *it                         # -> id=3
info breakpoints                  # see the remaining ignore count / hit counts
```

```bash
make clean
```
