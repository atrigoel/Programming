# GDB Example 01 — Breakpoints & Stepping

Learn the core loop of interactive debugging: stop somewhere, see how you got there, and
move through the code deliberately. Covers [gdb-guide §3](../../../gdb-guide.md#3-breakpoints-beyond-break-main)
and [§6](../../../gdb-guide.md#6-stepping-frames-and-the-stack).

## Run the scripted demo

```bash
make demo
```

It breaks inside the loop, prints the backtrace, steps **into** `square()`, `finish`es back
out, then does one `next` (stepping **over** the call) so you can see the difference.

## What to notice

- **`break main.c:12`** stops on a *line*, not just a function — the most common form.
- **`backtrace`** shows the chain `square → sum_of_squares → main`. `info args` / `info
  locals` show that frame's variables.
- **`step` vs `next`**: `step` descends into `square()`; `next` runs it to completion and
  stops on the next line. This distinction is the #1 thing people get wrong.
- **`finish`** runs until the current function returns and prints the **return value** — the
  fast way back out after you've seen enough.

## Now you try (interactive)

```bash
make debug
```
```gdb
break square              # break on the function instead of a line
run
backtrace                 # note square is now frame #0
frame 1                   # climb to sum_of_squares...
print i                   # ...and inspect ITS locals
finish                    # return, see the value
tbreak sum_of_squares     # temporary breakpoint (auto-deletes)
info breakpoints          # see hit counts and conditions
continue
```

Try `until 13` to run to the end of the loop without setting a breakpoint, and `stepi` to
move a single machine instruction (then `x/4i $pc` to see the disassembly).

```bash
make clean
```
