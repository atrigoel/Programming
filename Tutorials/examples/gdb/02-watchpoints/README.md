# GDB Example 02 — Watchpoints ("who changed my variable?")

The single most powerful technique for state-corruption bugs: a **watchpoint** stops the
program whenever a value changes — you don't need to know *where* the write is. Covers
[gdb-guide §5](../../../gdb-guide.md#5-watchpoints-data-breakpoints).

`main.c` changes `a.balance` in three places, one of them through an **alias pointer**
inside `apply_bonus()` — the kind of write you'd never spot by reading `main()`.

## Run the scripted demo

```bash
make demo
```

Each `continue` runs to the next write and GDB prints:

```
Hardware watchpoint 2: a.balance
Old value = 500
New value = 450
main () at main.c:24
```

The third stop lands *inside* `apply_bonus()` — the watchpoint found the aliased write for
you, with the exact `Old value`/`New value` and the line.

## What to notice

- **Watch the state, not the code.** You set one watchpoint and let GDB find all writers —
  no need to guess which function is guilty.
- **Old/New values** are printed automatically, so you see exactly how the value mutated.
- **The backtrace at each stop** names the culprit function and its callers.
- On this host it's a **hardware** watchpoint (the CPU debug registers do it at full speed).
  On a Cortex-M the same feature is backed by the **DWT** unit — see
  [gdb-guide §13](../../../gdb-guide.md#13-hardware-breakpoints--watchpoints-on-cortex-m).

## Now you try (interactive)

```bash
make debug
```
```gdb
break main
run
watch a.balance          # write watchpoint
# rwatch a.balance       # stop when READ instead
# awatch a.balance       # stop on read OR write
continue                 # walk change by change
print &a.balance         # get the address...
watch *(int*)<that addr> # ...and watch the raw address (works with no symbol)
info watchpoints         # list them + hit counts
```

For a stack/local variable, watch its **address** (`watch *(int*)&local`) so the watchpoint
survives even as the symbol goes in and out of scope.

```bash
make clean
```
