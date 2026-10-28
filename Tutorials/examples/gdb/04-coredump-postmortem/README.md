# GDB Example 04 — Core Dumps & Post-Mortem

Debug a crash *after* it happened — no live process, just the corpse. This is how you
investigate failures from CI or the field. Covers
[gdb-guide §11](../../../gdb-guide.md#11-core-dumps--post-mortem).

`main.c` walks a linked list but forgets the `NULL` check, so it dereferences a null
`next` and dies with `SIGSEGV`.

## Run the scripted demo

```bash
make demo
```

It runs the program (which crashes), finds the resulting `core` file, opens it with the
`prog` symbols, and prints the stack and the null pointer that caused it.

Expected highlight:

```
Program terminated with signal SIGSEGV, Segmentation fault.
#0  sum_list (head=0x...) at main.c:17
17          s += p->v;
p = 0x0            <-- the null pointer we dereferenced
```

## Where did my core file go?

Two common setups:

- **Core in the current directory** — needs `ulimit -c unlimited` (the `demo`/`crash`
  targets set it). File is named `core` or `core.<pid>`.
- **Routed to systemd** — many distros set `/proc/sys/kernel/core_pattern` to
  `systemd-coredump`. Then use:

```bash
make crash                 # trigger the crash
coredumpctl list           # find it
coredumpctl gdb prog       # open GDB on the saved core
```

Check your setting with `cat /proc/sys/kernel/core_pattern`.

## What to notice

- **`bt full`** is the whole point: the complete stack *plus* every frame's locals, frozen
  at the instant of the crash.
- You can `print`, `frame`, `info locals` — every **inspection** command works — but you
  can't `continue` (there's no live process to resume).
- On an MCU there's no OS to write a core; you catch the fault live in the HardFault handler
  instead — see [gdb-guide §15](../../../gdb-guide.md#15-debugging-a-hardfault).

## Now you try

```bash
make debug          # live session
```
```gdb
run                 # let it crash under GDB directly
bt full
frame 0
print p             # 0x0
list                # see the un-guarded loop
```

```bash
make clean
```
