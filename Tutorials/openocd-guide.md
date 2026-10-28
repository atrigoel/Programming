# OpenOCD: On-Chip Debugging for Cortex Microcontrollers on Linux

**OpenOCD** (Open On-Chip Debugger) is the free bridge between your PC's debugger (GDB)
and the debug hardware inside a microcontroller. It talks to a **debug probe**
(ST-Link, J-Link, CMSIS-DAP, FTDI…), which in turn drives the chip's **CoreSight** debug
logic over **SWD** or **JTAG**. With it you can flash firmware, halt the core, set
breakpoints, inspect memory/registers, and stream real-time trace.

```
 arm-none-eabi-gdb ──(TCP :3333)──► OpenOCD ──(USB)──► debug probe ──(SWD/JTAG)──► MCU
                                       │                                   │
                                  telnet :4444                        CoreSight
                                  (monitor cmds)                   (DAP, ITM, DWT, ETM…)
```

## Contents

- [OpenOCD: On-Chip Debugging for Cortex Microcontrollers on Linux](#openocd-on-chip-debugging-for-cortex-microcontrollers-on-linux)
  - [Contents](#contents)
  - [1. Installation](#1-installation)
  - [2. Probe access on Linux (udev)](#2-probe-access-on-linux-udev)
  - [3. Config file model](#3-config-file-model)
  - [4. Starting OpenOCD](#4-starting-openocd)
  - [5. The three ports (GDB, telnet, TCL)](#5-the-three-ports-gdb-telnet-tcl)
  - [6. Flashing firmware](#6-flashing-firmware)
  - [7. Debugging with GDB](#7-debugging-with-gdb)
  - [8. Transports: SWD vs JTAG](#8-transports-swd-vs-jtag)
  - [9. CoreSight: the debug architecture](#9-coresight-the-debug-architecture)
  - [10. Trace: SWO / ITM (printf-style)](#10-trace-swo--itm-printf-style)
  - [11. Trace: ETM instruction trace](#11-trace-etm-instruction-trace)
  - [12. Watchpoints via DWT](#12-watchpoints-via-dwt)
  - [13. Troubleshooting](#13-troubleshooting)
  - [Further References](#further-references)

---

## 1. Installation

**Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install openocd gdb-multiarch
openocd --version
```
`gdb-multiarch` works for ARM; if you have the GNU Arm toolchain,
`arm-none-eabi-gdb` is equivalent and often preferred.

**Build from source** (for the newest chip/probe support):
```bash
git clone https://github.com/openocd-org/openocd
cd openocd
./bootstrap
./configure --enable-stlink --enable-jlink --enable-cmsis-dap
make -j"$(nproc)"
sudo make install
```
The stock config scripts land in `/usr/share/openocd/scripts/` (or
`/usr/local/share/openocd/scripts/` for a source build).

---

## 2. Probe access on Linux (udev)

Out of the box, USB probes are root-only. Install udev rules so your user can use them
without `sudo` — OpenOCD even ships them:

```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger
sudo usermod -aG plugdev $USER      # then log out/in
```
See the [udev guide](udev-guide.md) for how these rules work and how to give a probe a
stable name.

---

## 3. Config file model

OpenOCD is configured by **two kinds of script**, combined with `-f`:

1. **Interface/adapter config** — *how* you connect (which probe).
   Found in `scripts/interface/`, e.g. `stlink.cfg`, `jlink.cfg`, `cmsis-dap.cfg`.
2. **Target config** — *what* you connect to (which chip/board).
   Found in `scripts/target/` (chip) and `scripts/board/` (full boards), e.g.
   `stm32f4x.cfg`, `board/st_nucleo_f4.cfg`.

```bash
# Probe + chip separately
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Or a board file that pulls in both
openocd -f board/st_nucleo_f4.cfg
```

A board file typically does `source [find interface/stlink.cfg]` then sets the transport
and sources the target. You can write your own project `openocd.cfg` that sources the
stock files and adds settings (adapter speed, reset config).

Common settings you'll set in a project config:
```tcl
source [find interface/stlink.cfg]
transport select swd            ; # or "hla_swd" for ST-Link HLA
source [find target/stm32f4x.cfg]

adapter speed 4000              ; # kHz; lower if the link is flaky
reset_config srst_only          ; # how the reset line is wired
```

---

## 4. Starting OpenOCD

Run it in one terminal; it stays resident, waiting for GDB/telnet:
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```
Successful start prints something like:
```
Info : STLINK V2J39 ... connected
Info : Target voltage: 3.28
Info : stm32f4x.cpu: Cortex-M4 r0p1 processor detected
Info : starting gdb server for stm32f4x.cpu on 3333
Info : Listening on port 3333 for gdb connections
```
That "Cortex-M4 ... detected" line means the probe reached the chip's DAP successfully.

---

## 5. The three ports (GDB, telnet, TCL)

OpenOCD listens on three TCP ports:

| Port | Protocol | Use |
|------|----------|-----|
| **3333** | GDB remote | `target extended-remote :3333` from GDB |
| **4444** | telnet | Interactive `monitor` commands for quick pokes |
| **6666** | TCL RPC | Scripted automation |

Quick telnet session (no GDB needed):
```bash
telnet localhost 4444
> reset halt          # reset the chip and stop the core
> flash write_image erase build/fw.elf
> reset run
> reg                 # dump core registers
> mdw 0x20000000 4    # memory display, 4 words
```
Any of these telnet commands is also available inside GDB prefixed with `monitor`
(e.g. `monitor reset halt`).

---

## 6. Flashing firmware

**From the command line** (one-shot, no interactive session):
```bash
openocd -f board/st_nucleo_f4.cfg \
    -c "program build/fw.elf verify reset exit"
```
`program` is the workhorse: it halts, erases, writes, `verify`s, `reset`s, and `exit`s.
It accepts `.elf`, `.hex`, or `.bin` (a `.bin` needs a load address:
`program fw.bin 0x08000000 ...`).

**From telnet:**
```
> reset halt
> flash write_image erase build/fw.elf
> verify_image build/fw.elf
> reset run
```

**List flash banks / erase:**
```
> flash banks
> flash erase_sector 0 0 last
```

Use the `.elf` from the [gcc](examples/gcc/05-cross-embedded/README.md) or
[cmake](examples/cmake/03-cross-cortex-m4/README.md) cross examples as your test image.

---

## 7. Debugging with GDB

Terminal 1 — OpenOCD (leave running):
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

Terminal 2 — GDB:
```bash
arm-none-eabi-gdb build/fw.elf
```
```gdb
target extended-remote :3333   # connect to OpenOCD's GDB server
monitor reset halt             # reset chip, hold core stopped
load                           # flash the .elf you opened
monitor reset halt             # start from a clean reset
break main
continue
# ... step/inspect ...
info registers
next
print counter
```

Handy `monitor` (OpenOCD) commands from inside GDB:
| Command | Effect |
|---------|--------|
| `monitor reset halt` | Reset and stop at the reset vector |
| `monitor reset run` | Reset and let it run |
| `monitor halt` | Stop the core now |
| `monitor mdw <addr> <n>` | Dump memory words |
| `monitor reg` | Dump core registers |

Automate all of this with a `.gdbinit` (see
[examples/openocd/gdbinit](examples/openocd/README.md)).

For the **GDB** side in depth — hardware breakpoints/watchpoints, HardFault analysis, SVD
peripheral inspection, semihosting, RTOS awareness, and scripting — see the
[GDB guide](gdb-guide.md).

---

## 8. Transports: SWD vs JTAG

These are the two physical wire protocols between the probe and the chip's Debug Port.

| | **JTAG** | **SWD** (Serial Wire Debug) |
|--|----------|------------------------------|
| Pins | 4–5 (TCK, TMS, TDI, TDO, +TRST) | 2 (SWCLK, SWDIO) |
| Origin | IEEE 1149.1 boundary scan, reused for debug | ARM's 2-wire debug for Cortex |
| Daisy-chain | Yes (multiple devices) | No (point-to-point) |
| Trace pin | — | Adds **SWO** as a 3rd pin (SWD + SWO = "SWV") |
| Typical use | Older/multi-device boards | **The default for Cortex-M** |

Select in OpenOCD:
```tcl
transport select swd     ; # or "jtag"
```
Cortex-M chips support both; SWD is preferred because it frees pins and still supports the
single-wire **SWO** trace output. (Note: **SWD** is the debug link; **SWO** is the trace
output; **SWV**/Serial Wire Viewer = SWD debug + SWO trace together. "SWI" usually refers
to a software interrupt/`SVC`, *not* a debug transport.)

---

## 9. CoreSight: the debug architecture

Everything OpenOCD does on a Cortex core goes through **CoreSight**, ARM's on-chip debug &
trace framework. The pieces you'll hear about:

```
                 ┌─────────────── Debug Access Port (DAP) ───────────────┐
   probe ──SWD/JTAG──► DP (Debug Port) ──► AP (Access Port) ──► system bus ──► CPU + peripherals
                 └───────────────────────────────────────────────────────┘
   Trace sources ─► ITM ─┐
                   DWT ──┤─► TPIU ─► SWO pin (1-bit)  or  trace port (4-bit) ─► probe
                   ETM ──┘         └─► ETB (on-chip buffer) on some parts
```

| Block | Full name | What it does |
|-------|-----------|--------------|
| **DAP** | Debug Access Port | The gateway the probe talks to; contains the DP + APs |
| **DP** | Debug Port | The SWD/JTAG-facing side of the DAP |
| **AP** | Access Port | Bridges to the on-chip bus (AHB-AP reaches memory/CPU) |
| **FPB** | Flash Patch & Breakpoint | Hardware breakpoints in flash-resident code |
| **DWT** | Data Watchpoint & Trace | Watchpoints, cycle counter, data/PC trace, profiling |
| **ITM** | Instrumentation Trace Macrocell | `printf`-style software trace (32 stimulus ports) |
| **ETM** | Embedded Trace Macrocell | Full **instruction** trace (every branch taken) |
| **TPIU** | Trace Port Interface Unit | Funnels ITM/DWT/ETM trace out to SWO or a trace port |
| **ETB** | Embedded Trace Buffer | On-chip RAM buffer for trace (when no external port) |

Key distinctions:
- **Halting debug** (breakpoints, single-step, memory access) uses the **DAP + FPB/DWT** —
  available on every Cortex-M via SWD/JTAG. No extra pins.
- **Trace** (ITM/DWT/ETM) is *non-intrusive* observation while the core runs, but needs the
  **TPIU** to get data off-chip via **SWO** (low bandwidth) or a parallel **trace port**
  (high bandwidth, more pins). Not all boards route these pins.

---

## 10. Trace: SWO / ITM (printf-style)

The most useful trace for everyday work: **ITM over SWO**. Your firmware writes bytes to an
ITM stimulus port; they leave the chip on the single **SWO** pin; OpenOCD captures them.
It's `printf` debugging without a UART and with minimal CPU cost.

**Firmware side** — write to ITM stimulus port 0 (CMSIS provides `ITM_SendChar`):
```c
#include "stm32f4xx.h"          /* or core_cm4.h for ITM registers */

int _write(int fd, char *buf, int len) {   /* retarget printf to ITM */
    for (int i = 0; i < len; i++)
        ITM_SendChar(buf[i]);
    return len;
}
```

**OpenOCD side** — enable the TPIU/SWO and route ITM to a file or the internal decoder.
Modern OpenOCD (0.11+) uses the `tpiu` command:
```tcl
# Args: swo output, protocol, TRACECLKIN (CPU clock Hz), SWO baud (0 = auto)
tpiu create swo -dap [dap names] -ap-num 0
tpiu configure -protocol uart -traceclk 16000000 -pin-freq 2000000 -output swo.log
itm ports on          ; # enable ITM stimulus ports
```
Older syntax you'll still see in the wild:
```tcl
tpiu config internal swo.log uart off 16000000 2000000
itm port 0 on
```
Then read the trace:
```bash
# From telnet you can also do:  itm ports on
tail -f swo.log                     # raw bytes from port 0
# or use a decoder that splits ITM ports, e.g. orbuculum/orbcat
```

**Key parameters that must be right:**
- **TRACECLKIN** = the actual CPU/HCLK frequency. If it's wrong, SWO bytes are garbage.
- **SWO baud** must match what the probe can sample (ST-Link caps SWO speed).
- SWO only works over **SWD** (it reuses a pin), not classic 4-wire JTAG on most parts.

**DWT via ITM** also gives you free goodies over the same link: a cycle counter
(`DWT->CYCCNT`) for timing, periodic PC sampling for statistical profiling, and event
counters — all emitted as ITM/DWT packets.

---

## 11. Trace: ETM instruction trace

**ETM** records the **exact instruction execution path** — every branch — so you can
reconstruct precisely what the CPU did, invaluable for hard-to-reproduce faults and
timing analysis. It is high-bandwidth:

- **Output path:** ETM → TPIU → either a **4-bit parallel trace port** (TRACECLK +
  TRACEDATA[0:3]) captured by a trace-capable probe (J-Trace, some ST-Link variants), or
  an on-chip **ETB** you read back over the DAP.
- **Pins:** the parallel trace port needs those 4–5 dedicated pins routed on the PCB;
  many low-cost boards don't expose them, which is why SWO/ITM is far more common.
- OpenOCD can configure the TPIU for parallel trace, but *capturing* ETM usually needs a
  trace-port-capable probe and a decoder (e.g. `orbuculum`, Segger tools, or a logic
  analyzer front-end).

Rule of thumb:
- Need occasional `printf`/timing → **ITM/SWO** (cheap, 1 pin, works with ST-Link).
- Need full instruction history → **ETM** (needs trace pins/ETB + capable probe).

---

## 12. Watchpoints via DWT

The **DWT** gives Cortex-M hardware **watchpoints** — break when a specific address is
read/written, without slowing the core. In GDB (OpenOCD maps these to DWT comparators):
```gdb
watch counter          # break when 'counter' is written
rwatch status_reg      # break when read
awatch flag            # break on read or write
```
Cortex-M has a small, fixed number of comparators (often 2–4), so watchpoints are limited.
The same DWT also powers `DWT->CYCCNT` cycle counting and the profiling/trace features
mentioned above.

---

## 13. Troubleshooting

- **`Error: open failed` / `unable to find CMSIS-DAP device`** → probe permissions. Install
  the udev rules ([§2](#2-probe-access-on-linux-udev)) and replug; test with `lsusb`.
- **`Error: init mode failed (unable to connect to the target)`** → wrong transport, wrong
  target `.cfg`, or the chip is in a low-power/locked state. Try
  `reset_config srst_only` and lower `adapter speed`.
- **`target not halted`** before flashing → issue `reset halt` first.
- **SWO output is garbage** → `TRACECLKIN` doesn't match the real CPU clock, or the SWO
  baud exceeds the probe's limit. Fix the clock value first.
- **Under-voltage / `Target voltage: 0.0`** → the board isn't powered or the probe's
  VTREF pin isn't connected; the probe reads target voltage to level-shift.
- **Flash write fails on option-byte-locked parts** → the chip has read-out protection
  (RDP); a mass-erase to lift RDP wipes the device.
- **Wrong core detected / halts at wrong address** → mismatched target script; pick the
  exact family file (`stm32f4x.cfg`, `stm32l4x.cfg`, …) or a board file.

Ready-to-use config and GDB scripts are in
[examples/openocd/](examples/openocd/README.md).

---

## Further References
- OpenOCD User's Guide: <https://openocd.org/doc/html/index.html>
- ARM CoreSight Technical Introduction and the *ARMv7-M Architecture Reference Manual*
  (ITM/DWT/FPB register details).
- *The Definitive Guide to ARM Cortex-M3/M4 Processors* (Joseph Yiu) — debug/trace chapters.
- `man openocd`; the stock scripts in `/usr/share/openocd/scripts/`.
