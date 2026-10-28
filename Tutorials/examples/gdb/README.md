# GDB Examples

Hands-on companions to [../../gdb-guide.md](../../gdb-guide.md). Every example here runs on
a **plain Linux host** (native, WSL, or container) — no debug probe or board required — so
you can practice the *techniques* before applying them on target. The embedded workflow
(connecting GDB to a chip) is covered by the [openocd/](../openocd/) example and the
[OpenOCD guide](../../openocd-guide.md).

Each folder ships a **`demo.gdb`** batch script so you can watch the feature work in one
command (`make demo`), then repeat it interactively to explore.

| Folder | Focus | Guide section |
|--------|-------|---------------|
| [`01-breakpoints-stepping/`](01-breakpoints-stepping/) | breakpoints, `backtrace`, `frame`, `step`/`next`/`finish` | §3, §6 |
| [`02-watchpoints/`](02-watchpoints/) | `watch`/`rwatch`/`awatch` — "who changed my variable?" | §5 |
| [`03-conditional-dprintf/`](03-conditional-dprintf/) | conditional breakpoints, `ignore`, command lists, `dprintf` | §4, §8 |
| [`04-coredump-postmortem/`](04-coredump-postmortem/) | core dumps, `bt full`, post-mortem inspection | §11 |
| [`05-reverse-debugging/`](05-reverse-debugging/) | `record`, `reverse-continue`, `reverse-step` | §10 |
| [`06-scripting-python/`](06-scripting-python/) | user-defined commands, loops, Python pretty-printers | §9, §17 |

## Setup (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install build-essential gdb
```

That's everything for examples 01–06. (Example 05's reverse execution needs native
x86-64/AArch64 Linux — it works under WSL2 but not inside every emulated container.)

## Quick smoke test

```bash
cd 01-breakpoints-stepping && make demo && make clean && cd -
```

## Suggested order

Work top to bottom — each builds on the previous. Run `make demo` first to see the scripted
walkthrough, then open GDB yourself (`make debug`) and try the "Now you try" steps in each
README. Every folder is safe to `make clean`.
