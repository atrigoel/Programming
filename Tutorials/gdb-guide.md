# GDB: Advanced Debugging for Host and Embedded Systems

**GDB** (the GNU Debugger) lets you stop a program at any point, inspect and change its
state, and watch how it got there. Most people learn `break`, `run`, `next`, `print` — and
stop. This guide picks up from those basics and covers the features that turn GDB from a
"print statement replacement" into a precision instrument: conditional and hardware
breakpoints, watchpoints, reverse debugging, scripting, TUI, core dumps, and — for
embedded — driving a Cortex-M chip through **OpenOCD**.

```
        ┌──────────── host debugging ────────────┐   ┌──── embedded debugging ────┐
  gdb ──► your process (ptrace)                    gdb ──(:3333)──► OpenOCD ──► MCU
        breakpoints run in-process                        remote protocol, HW breaks
```

> Toolchain note: on the host you use `gdb`; for ARM Cortex you use `arm-none-eabi-gdb`
> or `gdb-multiarch`. **Every command in this guide is identical across them** — only the
> connection step differs. The embedded specifics start at [§12](#12-embedded-gdb-with-openocd).
>
> **Runnable examples** for the host techniques below live in
> [examples/gdb/](examples/gdb/README.md) — each ships a `demo.gdb` you run with
> `make demo`, then repeat interactively. No board required.

## Contents

- [GDB: Advanced Debugging for Host and Embedded Systems](#gdb-advanced-debugging-for-host-and-embedded-systems)
  - [Contents](#contents)
  - [1. Compiling for debuggability](#1-compiling-for-debuggability)
  - [2. Ways to start GDB](#2-ways-to-start-gdb)
  - [3. Breakpoints beyond `break main`](#3-breakpoints-beyond-break-main)
  - [4. Conditional \& counted breakpoints](#4-conditional--counted-breakpoints)
  - [5. Watchpoints (data breakpoints)](#5-watchpoints-data-breakpoints)
  - [6. Stepping, frames, and the stack](#6-stepping-frames-and-the-stack)
  - [7. Examining data: `print`, `x`, and formats](#7-examining-data-print-x-and-formats)
  - [8. Breakpoint command lists \& `dprintf`](#8-breakpoint-command-lists--dprintf)
  - [9. Convenience variables \& expressions](#9-convenience-variables--expressions)
  - [10. Reverse debugging \& recording](#10-reverse-debugging--recording)
  - [11. Core dumps \& post-mortem](#11-core-dumps--post-mortem)
  - [12. Embedded GDB with OpenOCD](#12-embedded-gdb-with-openocd)
  - [13. Hardware breakpoints \& watchpoints on Cortex-M](#13-hardware-breakpoints--watchpoints-on-cortex-m)
  - [14. Inspecting memory-mapped peripherals (SVD)](#14-inspecting-memory-mapped-peripherals-svd)
  - [15. Debugging a HardFault](#15-debugging-a-hardfault)
  - [16. Semihosting \& RTOS awareness](#16-semihosting--rtos-awareness)
  - [17. Scripting: `.gdbinit`, hooks, and Python](#17-scripting-gdbinit-hooks-and-python)
  - [18. The TUI and useful front-ends](#18-the-tui-and-useful-front-ends)
  - [19. Troubleshooting](#19-troubleshooting)
  - [20. Command quick reference](#20-command-quick-reference)
  - [Further References](#further-references)

---

## 1. Compiling for debuggability

The debugger can only show you what the compiler put in the binary. Two flags matter most:

| Flag | Effect |
|------|--------|
| `-g` / `-g3` | Emit debug info. `-g3` also includes **macro** definitions |
| `-Og` | Optimize but keep debugging coherent — **the sweet spot** |
| `-O0` | No optimization; code maps 1:1 to source (easiest stepping) |
| `-O2`/`-Os` | Optimized: variables get `<optimized out>`, lines reorder |

```bash
gcc -Og -g3 main.c -o main              # host
arm-none-eabi-gcc -Og -g3 -mcpu=cortex-m4 ... -o fw.elf   # embedded
```

Why not just `-O0`? Optimized builds behave differently (timing, register allocation,
inlining), and bugs sometimes only appear with `-O2`. `-Og` lets you debug something close
to the real release build. When a variable shows `<optimized out>`, either rebuild that
file at `-O0` or read it from the register/stack slot the compiler actually used.

> Keep the `.elf` (with symbols) for GDB even if you flash a stripped `.bin`. The symbols
> live in the ELF, never on the chip.

---

## 2. Ways to start GDB

```bash
gdb ./prog                     # load symbols, don't run yet
gdb --args ./prog --flag x     # pass arguments to the program
gdb -p 4242                    # attach to a running PID
gdb ./prog core                # open a core dump (post-mortem, §11)
gdb -q ./prog                  # quiet: skip the license banner
gdb -x script.gdb ./prog       # run a command script on start
gdb -batch -ex run -ex bt ./prog   # non-interactive one-liner
```

Inside GDB, `run` (`r`) starts the program, `start` sets a temporary breakpoint on `main`
and runs to it. To debug something that only fails under a script, attach mid-flight:

```gdb
attach 4242         # take control of PID 4242
detach              # release it, let it keep running
```

Useful startup settings (put these in `~/.gdbinit`, see [§17](#17-scripting-gdbinit-hooks-and-python)):

```gdb
set pagination off             # no "---Type <return>---" prompts
set history save on            # persist command history
set print pretty on            # readable struct printing
set confirm off                # don't ask "are you sure?"
```

---

## 3. Breakpoints beyond `break main`

A breakpoint can target far more than a function name:

```gdb
break main                 # function
break main.c:42            # file:line
break 42                   # line in the current file
break *0x08000c34          # exact address (great for embedded/no symbols)
break foo if x > 10        # conditional (see §4)
tbreak setup               # temporary — deletes itself after one hit
break Widget::draw         # C++ method
rbreak ^usb_               # regex: break on every function starting usb_
```

Manage them:

```gdb
info breakpoints           # list all, with hit counts
disable 2                  # keep but deactivate breakpoint #2
enable 2
delete 2                   # remove it
delete                     # remove all
ignore 3 5                 # skip the next 5 hits of breakpoint #3
```

**Breakpoint on a shared library not yet loaded?** GDB makes it *pending*:

```gdb
set breakpoint pending on
break libfoo_function
```

**Catchpoints** stop on events rather than locations — invaluable for tricky bugs:

```gdb
catch throw                # C++ exception thrown
catch syscall write        # any write() syscall
catch signal SIGSEGV       # (also see `handle`, §19)
```

---

## 4. Conditional & counted breakpoints

Stopping every iteration of a hot loop is useless. Make GDB do the filtering.

```gdb
break process_item if item->id == 0x4001    # only when the condition holds
condition 4 count > 1000                     # add/replace condition on bp #4
condition 4                                  # remove the condition
```

Conditions are full expressions evaluated in the program's context — call functions,
dereference pointers, compare strings via helpers:

```gdb
break handle_packet if pkt->len > 1500
break parse if $_streq(name, "target")       # convenience function
break *0x0800123a if $r0 == 0                # embedded: test a register
```

**Ignore counts** are faster than conditions when you just want the Nth hit (GDB doesn't
evaluate an expression each time):

```gdb
ignore 2 99      # let breakpoint #2 pass 99 times, stop on the 100th
```

Combine with a temporary breakpoint to catch "the moment it goes wrong":

```gdb
watch total                       # §5
break checkpoint if total < 0     # first time the invariant breaks
```

---

## 5. Watchpoints (data breakpoints)

A **watchpoint** stops the program when a *value changes* — you don't need to know *where*
in the code it happens. This is the single most powerful feature for "who is corrupting my
variable?" bugs.

```gdb
watch counter          # stop when `counter` is written with a new value
rwatch config          # stop when `config` is READ
awatch flags           # stop on read OR write (access)
watch *(int*)0x2000010 # watch a raw address (embedded / no symbol)
watch buf[i]           # watch a specific element
info watchpoints
```

When it triggers, GDB shows the old and new value and the line responsible:

```
Hardware watchpoint 2: counter
Old value = 41
New value = 42
increment () at main.c:18
```

**Scope matters.** A watchpoint on a local variable is auto-deleted when its frame returns.
For a stack variable that keeps getting clobbered, watch the *address*:

```gdb
print &local           # e.g. 0x7fffffffe30c
watch *(int*)0x7fffffffe30c
```

**Software vs hardware watchpoints:**

| | Hardware | Software |
|--|----------|----------|
| Speed | Full speed (CPU debug unit does it) | Very slow (single-steps + checks) |
| Count | Limited (x86 ≈ 4; Cortex-M4 DWT ≈ 4) | Unlimited |
| Trigger | Set automatically when supported | GDB falls back if HW runs out |

Force one or the other if needed:

```gdb
set can-use-hw-watchpoints 0     # force software (debugging the debugger)
```

On embedded, hardware watchpoints are backed by the **DWT** comparators — see [§13](#13-hardware-breakpoints--watchpoints-on-cortex-m).

---

## 6. Stepping, frames, and the stack

```gdb
next   (n)     # execute one source line, stepping OVER calls
step   (s)     # execute one source line, stepping INTO calls
stepi  (si)    # one machine instruction (essential for asm/embedded)
nexti  (ni)    # one instruction, over calls
finish         # run until the current function returns (shows return value)
until 55       # run until line 55 (skip a loop without a breakpoint)
return 0       # force an immediate return with a value (skip rest of fn)
advance foo    # run to `foo` OR the current frame returning, whichever first
```

The **call stack** is your map of "how did I get here":

```gdb
backtrace  (bt)    # full call stack
bt 5               # innermost 5 frames
bt full            # each frame + its local variables
frame 3   (f 3)    # select frame #3 to inspect its locals
up / down          # move one frame toward caller / callee
info args          # arguments of the selected frame
info locals        # locals of the selected frame
```

Selecting a frame changes what `print` sees — climb the stack to inspect a caller's
variables without re-running. `info frame` shows saved registers and the return address,
which is how you unwind a corrupted stack manually on embedded.

---

## 7. Examining data: `print`, `x`, and formats

`print` (`p`) evaluates any C/C++ expression, including calling functions:

```gdb
print x                 # value of x
print/x x               # in hex   (/d dec, /t binary, /c char, /a address)
print arr               # whole array
print arr@10            # array-slice: 10 elements starting at arr
print *p@5              # 5 elements starting at what p points to
print sizeof(struct Foo)
print my_func(3)        # actually CALLS my_func in the inferior
print $pc              # program counter register
set var x = 99          # modify a variable
set var arr[2] = 0
```

`x` (**examine memory**) is `x/NFU addr` — **N** count, **F** format, **U** unit size:

```gdb
x/8xw 0x20000000        # 8 hex words
x/16xb &buf             # 16 hex bytes of buf
x/s name                # a C string
x/8i $pc                # 8 instructions at the program counter (disassemble)
x/4dg &val              # 4 signed giant (8-byte) decimals
```

Units: `b`=byte `h`=half(2) `w`=word(4) `g`=giant(8). Formats: `x`hex `d`dec `u`unsigned
`t`binary `c`char `s`string `i`instruction `f`float `a`address.

Other inspection tools:

```gdb
info registers          # all core registers  (info all-registers on embedded)
info registers r0 pc    # specific ones
p/x $sp                 # a single register in hex
ptype struct Foo        # print the full type definition
whatis x                # the type of an expression
info line main          # address range of a source line
disassemble main        # disassemble a function
disassemble /s main     # interleave source with assembly
info symbol 0x08000c34  # which symbol is at this address (embedded staple)
```

---

## 8. Breakpoint command lists & `dprintf`

Attach a **command list** to a breakpoint so GDB acts automatically when it hits — the
foundation of scripted debugging. Ending with `continue` gives you tracing without stopping:

```gdb
break parse_header
commands
  silent                 # don't print the usual stop banner
  printf "hdr: type=%d len=%d\n", hdr->type, hdr->len
  continue               # resume automatically
end
```

`dprintf` (**dynamic printf**) is the same idea in one line — insert a print at a location
without editing/recompiling code:

```gdb
dprintf main.c:88, "x=%d y=%d\n", x, y
dprintf process, "entered process, arg=%d\n", n
info dprintf
```

This is a game-changer for embedded: add logging to flashed firmware **without a UART, a
rebuild, or a reflash**. GDB inserts a breakpoint, prints, and continues. (For zero-halt
logging while the core runs, use ITM/SWO instead — see the
[OpenOCD guide §10](openocd-guide.md).)

---

## 9. Convenience variables & expressions

GDB has its own variables (prefixed `$`) that live only in the debugger — perfect for
scripting and remembering values across commands:

```gdb
set $i = 0                     # your own variable
print $i++                     # usable in loops
set $p = head                  # save a pointer
print $p->next->value
print $               # the last value printed
print $$              # the value before that
print $eax / $pc / $sp / $r0   # registers are convenience vars too
```

Walk a linked list interactively without writing code:

```gdb
set $n = head
while $n != 0
  printf "id=%d\n", $n->id
  set $n = $n->next
end
```

Built-in convenience *functions* help in conditions:

```gdb
break x if $_streq(s, "quit")     # string compare
print $_regex(name, "usb_.*")     # regex match
print $_caller_is("main")         # who called us
```

`$_exitcode`, `$_siginfo`, and `$_thread` are set automatically and are handy in scripts.

---

## 10. Reverse debugging & recording

GDB can record execution and then **step backwards** — you hit a crash, then rewind to see
what led there. On native Linux/x86 this uses `record`:

```gdb
start
record                 # begin recording (full software record)
continue               # run until the crash / breakpoint
reverse-continue (rc)  # run BACKWARDS to the previous breakpoint
reverse-step   (rs)    # step backward into
reverse-next   (rn)    # step backward over
reverse-stepi
reverse-finish         # back to where the current function was called
```

Classic workflow — a variable is wrong and you want the last write:

```gdb
record
continue                       # stop at the crash
watch bad_var
reverse-continue               # runs backward, stops at the write that set it
```

`record` (in-process) is limited and slow for big programs. For serious use, record with
**`rr`** (https://rr-project.org) once, then replay deterministically under GDB as many
times as you like — reverse-execution included:

```bash
rr record ./prog
rr replay          # drops you into a GDB session; forward & reverse both work
```

> Reverse execution is **not** available on remote/embedded targets (OpenOCD) — the
> hardware doesn't record. Use it on the host, or use a simulator/`rr`.

---

## 11. Core dumps & post-mortem

When a program has already crashed, a **core dump** captures its final state so you can
debug it after the fact.

```bash
ulimit -c unlimited                 # allow cores in this shell
./prog                              # crashes → writes a core file
# where did it go? modern systemd may capture it:
coredumpctl list
coredumpctl gdb <PID>               # opens GDB on the saved core
```

Open it manually and investigate:

```gdb
gdb ./prog core
(gdb) bt full          # the stack at the moment of death
(gdb) frame 2
(gdb) info locals
(gdb) print *this
```

You can't `continue` a core (there's no live process), but every inspection command works.
For embedded there's no OS to write a core; instead you catch the fault live —
see [§15](#15-debugging-a-hardfault).

---

## 12. Embedded GDB with OpenOCD

On a microcontroller there is no OS and no `ptrace`. GDB instead speaks the **GDB Remote
Serial Protocol** over TCP to a debug server (**OpenOCD**), which drives the chip's on-chip
debug logic over SWD/JTAG. For the OpenOCD side (installation, `.cfg` files, flashing,
SWO/ITM trace, CoreSight internals) see the dedicated [OpenOCD guide](openocd-guide.md);
this section focuses on the **GDB** half.

```
 arm-none-eabi-gdb ──(TCP :3333, remote protocol)──► OpenOCD ──(USB/SWD)──► Cortex-M
```

**Terminal 1** — start the server and leave it running:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

**Terminal 2** — connect GDB to it:

```bash
arm-none-eabi-gdb build/fw.elf
```
```gdb
target extended-remote :3333   # connect (extended = allows program restart)
monitor reset halt             # OpenOCD: reset chip, hold core stopped
load                           # flash the .elf you opened into the MCU
monitor reset halt             # clean start from the reset vector
break main
continue
```

`target remote` vs `target extended-remote`: **extended** lets you `run`/`kill` and
re-`load` without dropping the connection — prefer it for iterative work.

**`monitor` sends a command straight to OpenOCD** (bypassing GDB). The essentials:

| From GDB | OpenOCD action |
|----------|----------------|
| `monitor reset halt` | Reset and stop at the reset vector |
| `monitor reset run` | Reset and let it run |
| `monitor halt` | Stop the running core now |
| `monitor reg` | Dump core registers (OpenOCD's view) |
| `monitor mdw 0x40023800 4` | Read 4 memory words |
| `monitor flash write_image erase fw.elf` | Flash without GDB's `load` |

**The flash-edit-debug loop** you'll run hundreds of times:

```gdb
monitor reset halt
load
monitor reset halt
continue
# ... find bug, fix code, rebuild fw.elf outside GDB ...
# back in the same GDB session:
file build/fw.elf      # reload symbols for the new build
load                   # reflash
monitor reset halt
```

Automate the connect/flash sequence in a `.gdbinit` so `arm-none-eabi-gdb` just works — see
[examples/openocd/gdbinit](examples/openocd/README.md) and [§17](#17-scripting-gdbinit-hooks-and-python).

---

## 13. Hardware breakpoints & watchpoints on Cortex-M

Code in **flash can't hold a software breakpoint** (GDB can't overwrite the instruction
with a trap the way it does in RAM). The Cortex-M debug unit provides a small number of
**hardware** comparators instead:

| Resource | Cortex-M block | Typical count (M4) | Used for |
|----------|----------------|--------------------|----------|
| Hardware breakpoints | **FPB** (Flash Patch & Breakpoint) | 6 | `hbreak`, flash breakpoints |
| Watchpoints | **DWT** (Data Watchpoint & Trace) | 4 | `watch`/`rwatch`/`awatch` |

GDB usually picks hardware automatically on a remote target, but you can be explicit:

```gdb
hbreak main            # force a HARDWARE breakpoint
thbreak setup          # temporary hardware breakpoint
break main.c:42        # GDB auto-uses HW here because it's a remote/flash target
```

You will eventually hit the limit:

```
Cannot insert breakpoint 7.
Cannot set hardware breakpoint: resource not available.
```

That means all FPB comparators are in use — `delete` or `disable` some. The same applies to
watchpoints: only ~4 DWT comparators exist, so `watch` on a 5th variable fails. Watch a
narrower region (a single word, not a whole struct) to stay within budget:

```gdb
watch *(uint32_t*)&status      # one word → one DWT comparator
```

The DWT also gives you a free **cycle counter** — priceless for timing without extra code:

```gdb
# DWT_CYCCNT lives at 0xE0001004; enable via DWT_CTRL bit0 (0xE0001000)
set {int}0xE0001000 = 1        # enable CYCCNT
set {int}0xE0001004 = 0        # zero it
# ... run a stretch of code ...
print *(unsigned int*)0xE0001004   # cycles elapsed
```

---

## 14. Inspecting memory-mapped peripherals (SVD)

On embedded, half of debugging is reading **peripheral registers** (is the UART enabled? is
the DMA flag set?). Raw addresses work but are unreadable:

```gdb
x/x 0x40023830                 # RCC_AHB1ENR — but which bits mean what?
```

The fix is an **SVD file** (CMSIS System View Description) — an XML map of every peripheral,
register, and bitfield for your exact chip, shipped by the silicon vendor. With a plug-in,
GDB shows registers **by name with decoded fields**.

**Using the `cmsis-svd`/PyCortexMDebug plug-in:**

```gdb
source /path/to/PyCortexMDebug/cmdebug/svd_gdb.py
svd_load STM32F407.svd
svd RCC                        # list all registers in the RCC peripheral
svd RCC AHB1ENR                # decode one register into named bitfields
svd GPIOA ODR                  # e.g. see which output pins are high
svd/x TIM2 CNT                 # in hex
```

Output looks like:

```
RCC.AHB1ENR = 0x00100001
  GPIOAEN  = 1
  CRCEN    = 0
  DMA2EN   = 1
  ...
```

Editors like VS Code (Cortex-Debug) read the same SVD to give you a live register tree — but
the same data is available right here in raw GDB. Get the SVD from your vendor's CMSIS pack
or the `cmsis-svd` repository. See [Cortex-M4 guide](cortex-m4-architecture-guide.md) for
what the registers mean.

---

## 15. Debugging a HardFault

When a Cortex-M hits an illegal memory access, bad instruction, or unaligned access, it
traps into the **HardFault** handler. By default that's an infinite loop, and you're stuck
with no clue why. GDB + the fault registers recover the story.

**Step 1 — catch it.** Break on the handler (name depends on your startup code):

```gdb
break HardFault_Handler
continue
```

**Step 2 — read the fault status registers** in the System Control Block (SCB):

```gdb
# Configurable Fault Status Register (CFSR) @ 0xE000ED28 — the "what happened"
print/x *(uint32_t*)0xE000ED28     # CFSR (UsageFault:MemManage:BusFault)
print/x *(uint32_t*)0xE000ED2C     # HFSR  (HardFault status)
print/x *(uint32_t*)0xE000ED34     # MMFAR (faulting data address, if valid)
print/x *(uint32_t*)0xE000ED38     # BFAR  (bus fault address, if valid)
```

Key CFSR bits: `IMPRECISERR`/`PRECISERR` (bad data access), `BFARVALID` (BFAR holds the
address), `UNDEFINSTR` (bad opcode), `UNALIGNED` (unaligned access).

**Step 3 — recover the offending instruction.** On exception entry the core pushes a stack
frame: `R0-R3, R12, LR, PC, xPSR`. The stacked **PC** is the instruction that faulted. Which
stack? `LR` (EXC_RETURN) bit 2 selects MSP (0) or PSP (1):

```gdb
print/x $lr                        # EXC_RETURN; bit2 → which SP was active
# if bit2 == 0 the frame is on MSP:
set $frame = $msp                  # else: set $frame = $psp
print/x ((uint32_t*)$frame)[6]     # stacked PC — the faulting instruction
info symbol ((uint32_t*)$frame)[6] # turn that address into a function+line
x/i   ((uint32_t*)$frame)[6]       # disassemble it
```

Now `info symbol` / `x/i` tells you the exact line that blew up. Many projects add a C
fault handler that grabs these values; GDB lets you do it live without one. See the
[Cortex-M4 guide](cortex-m4-architecture-guide.md) for the full exception model.

---

## 16. Semihosting & RTOS awareness

**Semihosting** lets firmware use the host's I/O through the debugger — `printf` shows up in
the OpenOCD console, and the target can read files or the clock, with **no UART**. It's
slow (halts the core per call) but perfect for early bring-up.

Enable it (firmware must be built with a semihosting-aware libc / `--specs=rdimon.specs`):

```gdb
monitor arm semihosting enable
continue
# printf() output from the MCU now appears in OpenOCD's terminal
```

**RTOS awareness** — when debugging FreeRTOS/Zephyr/etc., tell OpenOCD which RTOS so GDB
sees **per-task stacks** instead of just the current CPU state:

```tcl
# in the OpenOCD target .cfg, before "init":
$_TARGETNAME configure -rtos auto      ; # or -rtos FreeRTOS
```
Then in GDB, threads map to tasks:

```gdb
info threads           # one entry per RTOS task
thread 3               # switch to task 3's context
bt                     # its call stack
```

Without this, a backtrace only ever shows the task that happened to be running when you
halted.

---

## 17. Scripting: `.gdbinit`, hooks, and Python

GDB reads `~/.gdbinit` (global) and a `./.gdbinit` in the working directory (project-local,
if `set auto-load safe-path` permits). Put boilerplate there.

**A ready-to-use embedded `.gdbinit`:**

```gdb
set pagination off
set print pretty on
set history save on

# one command to connect + flash + reset
define reflash
  target extended-remote :3333
  monitor reset halt
  load
  monitor reset halt
end
document reflash
Connect to OpenOCD, flash the current ELF, and halt at reset.
end

# handy alias
define rr
  monitor reset halt
end
```

Then just type `reflash` after launching `arm-none-eabi-gdb fw.elf`.

**User-defined commands** take arguments (`$arg0`…) and can loop:

```gdb
define hexdump
  x/16xb $arg0
end
# usage:  hexdump &buffer
```

**Hooks** run automatically around other commands:

```gdb
define hook-stop            # runs every time the program stops
  printf "PC=0x%x\n", $pc
end
```

**Python scripting** is available in modern GDB for anything the command language can't do —
custom pretty-printers, walking data structures, automated analysis:

```python
python
import gdb
class MaxStackDepth(gdb.Command):
    """Print the current backtrace depth."""
    def __init__(self):
        super().__init__("stackdepth", gdb.COMMAND_USER)
    def invoke(self, arg, from_tty):
        n = 0
        f = gdb.newest_frame()
        while f:
            n += 1
            f = f.older()
        print("depth =", n)
MaxStackDepth()
end
```

Now `stackdepth` is a real GDB command. Pretty-printers registered this way make
`print my_vector` show contents instead of raw pointers — this is how libstdc++'s
containers print nicely.

---

## 18. The TUI and useful front-ends

GDB's built-in **Text User Interface** splits the terminal into source + command panes —
far nicer than blind stepping:

```gdb
tui enable          # or start gdb with:  gdb -tui  /  press Ctrl-x Ctrl-a
layout src          # source pane
layout asm          # disassembly pane
layout regs         # registers + source
layout split        # source + asm
focus cmd           # send arrow keys to the command window
Ctrl-x o            # switch active pane
Ctrl-l              # redraw if the display gets corrupted
```

In TUI, breakpoints and the current line are marked in the source pane, and it updates as
you step. If the screen garbles (common over SSH), `Ctrl-l` redraws.

Beyond raw GDB:

| Front-end | Good for |
|-----------|----------|
| **`gdb -tui`** | Zero-install, works over SSH and on the target |
| **`gef` / `pwndbg`** | Rich Python extensions (memory, heap, registers at a glance) |
| **VS Code + Cortex-Debug** | GUI embedded debugging (SVD tree, RTOS, SWO) over OpenOCD |
| **`cgdb`** | Curses front-end with vim-like keys |
| **`rr`** | Record/replay for reverse debugging ([§10](#10-reverse-debugging--recording)) |

All of them are *front-ends over the same GDB* — the commands in this guide still apply.

---

## 19. Troubleshooting

| Symptom | Cause / Fix |
|---------|-------------|
| `No symbol "x" in current context` | Not built with `-g`, or `x` is out of scope — select the right `frame` |
| Variable shows `<optimized out>` | Optimized build — use `-Og`/`-O0`, or read the register it lives in |
| Breakpoint "will be pending" | Symbol not loaded yet — `set breakpoint pending on`, or wait for the library |
| `Cannot access memory at 0x…` | Bad pointer, or (embedded) peripheral not clocked / wrong address |
| Can't set breakpoint (embedded) | Out of hardware breakpoints — `delete` some ([§13](#13-hardware-breakpoints--watchpoints-on-cortex-m)) |
| Watchpoint too slow | Fell back to software — reduce watched size, or free a DWT comparator |
| Stepping jumps around erratically | Optimized code reorders lines — rebuild at `-Og`/`-O0` |
| `Remote 'g' packet reply is too long` | GDB/OpenOCD target mismatch — use the matching `arm-none-eabi-gdb` |
| Program ignores `SIGINT` / can't stop | `handle SIGINT stop` and check terminal ownership |
| `bt` shows `??` / garbage frames | Corrupted stack or no unwind info — check `-fno-omit-frame-pointer` |

Signal handling is its own lever — decide what GDB does per signal:

```gdb
info signals
handle SIGPIPE nostop noprint pass   # ignore SIGPIPE, deliver it to the program
handle SIGSEGV stop print            # always stop on segfault (the default)
```

---

## 20. Command quick reference

```gdb
# --- run control ---
run / start / continue(c)      attach PID / detach
next(n) step(s) stepi(si) finish until N return

# --- breakpoints ---
break LOC [if COND]   tbreak   hbreak   rbreak REGEX   dprintf LOC,FMT,ARGS
info breakpoints   enable/disable/delete N   ignore N COUNT   condition N EXPR

# --- data ---
watch/rwatch/awatch EXPR      print(p)/x EXPR   x/NFU ADDR   set var X = V
info registers   info locals   info args   ptype/whatis EXPR   display EXPR

# --- stack ---
backtrace(bt) [full]   frame(f) N   up/down   info frame

# --- reverse (host) ---
record   reverse-continue(rc)   reverse-step(rs)   reverse-next(rn)

# --- embedded (with OpenOCD) ---
target extended-remote :3333   monitor reset halt   load   file ELF
monitor reg   monitor mdw ADDR N   monitor arm semihosting enable

# --- misc ---
tui enable   layout src/asm/regs   define NAME…end   source FILE.gdb
```

---

## Further References

- **GDB manual** — https://sourceware.org/gdb/current/onlinedocs/gdb/ (the authoritative reference)
- **`help`** inside GDB — `help`, `help breakpoints`, `apropos watch`
- **RR (record & replay)** — https://rr-project.org
- **OpenOCD guide** — [openocd-guide.md](openocd-guide.md) — the server side of embedded debugging
- **Cortex-M4 architecture** — [cortex-m4-architecture-guide.md](cortex-m4-architecture-guide.md) — registers, exceptions, faults
- **GCC guide** — [gcc-guide.md](gcc-guide.md) — `-g`/`-Og` and building debuggable binaries
- **PyCortexMDebug (SVD in GDB)** — https://github.com/bnahill/PyCortexMDebug
- **CMSIS-SVD files** — https://github.com/cmsis-svd/cmsis-svd
- **`gef`** https://github.com/hugsy/gef · **`pwndbg`** https://github.com/pwndbg/pwndbg
