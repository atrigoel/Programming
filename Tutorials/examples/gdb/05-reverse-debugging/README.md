# GDB Example 05 — Reverse Debugging

Run forward, hit the problem, then **step backwards through history** to see what led there.
Covers [gdb-guide §10](../../../gdb-guide.md#10-reverse-debugging--recording).

`main.c` mutates `state` in three stages (`+5`, `*2`, `-3`). We record the run, then rewind
to find each writer without restarting the program.

> **Requirements:** GDB's built-in `record` works on **native x86-64/AArch64 Linux**. It
> runs under WSL2 but may be unavailable in emulated containers or on non-native arches. If
> `record` errors out, see the `rr` note below.

## Run the scripted demo

```bash
make demo
```

It records from `main`, runs forward to the end, then uses a watchpoint plus
`reverse-continue` to walk **backwards** through the three writes, printing the value of
`state` before each stage:

```
before step_c: state was 10
before step_b: state was 5
before step_a: state was 0
```

## What to notice

- **`record`** turns on execution logging; from then on you can move in either direction.
- **`reverse-continue` / `reverse-step` / `reverse-next`** are the mirror images of the
  normal commands — combine with a `watch` to jump straight to "the last time this changed."
- The classic use: a value is wrong at a crash, so you `watch` it and `reverse-continue`
  once to land exactly on the instruction that set it.

## If `record` isn't available: use `rr`

[`rr`](https://rr-project.org) records once to a trace, then replays deterministically with
full reverse support:

```bash
sudo apt-get install rr
rr record ./prog
rr replay            # a GDB session; forward AND reverse work here
```

> Reverse execution is a **host-only** superpower — it's not available on remote/embedded
> targets over OpenOCD, because the hardware doesn't record history.

## Now you try

```bash
make debug
```
```gdb
break main
run
record
next
next
reverse-next         # undo the last line
print state
reverse-stepi        # go back a single instruction
```

```bash
make clean
```
