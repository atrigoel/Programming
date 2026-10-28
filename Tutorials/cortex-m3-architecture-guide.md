# ARM Cortex-M3 Architecture — A Vendor-Agnostic Guide

> Focus: the ARM **core** itself (the parts every silicon vendor shares), not any
> specific microcontroller's peripherals. Everything here is defined by ARM in the
> *ARMv7-M Architecture Reference Manual* and the *Cortex-M3 Technical Reference Manual*.

---

## Table of Contents

- [ARM Cortex-M3 Architecture — A Vendor-Agnostic Guide](#arm-cortex-m3-architecture--a-vendor-agnostic-guide)
  - [Table of Contents](#table-of-contents)
  - [1. Big Picture](#1-big-picture)
  - [2. ARMv7-M and Thumb-2](#2-armv7-m-and-thumb-2)
  - [3. Pipeline and Bus Interfaces](#3-pipeline-and-bus-interfaces)
  - [4. Memory Model](#4-memory-model)
  - [5. Core Registers](#5-core-registers)
  - [6. Special Registers](#6-special-registers)
    - [6.1 xPSR — Program Status Register](#61-xpsr--program-status-register)
    - [6.2 Interrupt Masking Registers](#62-interrupt-masking-registers)
    - [6.3 CONTROL Register](#63-control-register)
  - [7. Operating Modes and Privilege](#7-operating-modes-and-privilege)
  - [8. The Two Stacks](#8-the-two-stacks)
  - [9. Exceptions and Interrupts (NVIC)](#9-exceptions-and-interrupts-nvic)
    - [System exceptions](#system-exceptions)
  - [10. Exception Entry and EXC\_RETURN](#10-exception-entry-and-exc_return)
  - [11. Reset and Vector Table](#11-reset-and-vector-table)
  - [12. System Control Space](#12-system-control-space)
  - [13. The MPU](#13-the-mpu)
  - [14. Bit-Banding](#14-bit-banding)
  - [15. Debug Architecture](#15-debug-architecture)
  - [16. Low-Power and Sleep](#16-low-power-and-sleep)
  - [17. Instruction Set Highlights](#17-instruction-set-highlights)
  - [18. Memory Barriers](#18-memory-barriers)
  - [19. What the Cortex-M3 Has (vs. M0/M0+) and Lacks (vs. M4)](#19-what-the-cortex-m3-has-vs-m0m0-and-lacks-vs-m4)
  - [20. Bring-Up Mental Model](#20-bring-up-mental-model)
  - [21. Further References](#21-further-references)

---

## 1. Big Picture

The **Cortex-M3** is the original mainstream 32-bit microcontroller core implementing
the **ARMv7-M** architecture profile. It is the core that established the "modern"
Cortex-M programmer's model — full Thumb-2, a powerful NVIC, configurable faults,
bit-banding, and an optional MPU. Key characteristics:

- **32-bit** data path and addresses → 4 GB linear address space.
- **Harvard-ish** internal buses (separate instruction/data paths) but a **single
  unified 4 GB memory map**.
- **Full Thumb-2**: a rich mix of 16-bit and 32-bit instructions, including
  **hardware divide** (`SDIV`/`UDIV`), `IT` blocks, `CBZ/CBNZ`, bitfield ops, and
  exclusives (`LDREX/STREX`).
- **3-stage pipeline** with branch speculation.
- Deterministic, low-latency **interrupt handling** via the full **NVIC** (up to 240
  IRQs, up to 256 priority levels).
- **Configurable faults** (MemManage, BusFault, UsageFault) with fault-status
  registers for precise diagnosis.
- Optional **MPU**; architectural **bit-banding**.
- **No FPU and no DSP/SIMD** — that is exactly what the M4 adds on top of the M3.

In one line: **the M4 is "an M3 + DSP extension (+ optional FPU)."** So the M3 is the
best core to learn the ARMv7-M model on, because everything you learn transfers directly
up to the M4/M7 (minus the floating-point/DSP parts).

```
          +--------------------------------------------------+
          |                  Cortex-M3 core                  |
          |  +-----------+   +---------+    +--------------+  |
          |  | 3-stage   |   |  NVIC   |    |  MPU (opt.)  |  |
          |  | pipeline  |   | (IRQ)   |    |              |  |
          |  +-----------+   +---------+    +--------------+  |
          |  +-----------+   +---------+    +--------------+  |
          |  | Register  |   |  HW     |    |  Debug (DWT, |  |
          |  | bank+PSR  |   | divide  |    |  ITM, FPB)   |  |
          |  +-----------+   +---------+    +--------------+  |
          +----------|------------|--------------|-----------+
                   I-Code       D-Code         System
                    bus          bus            bus
```

---

## 2. ARMv7-M and Thumb-2

- **ARMv7-M** is the "Microcontroller" profile of ARMv7 (vs. -A Application and -R
  Real-time). It is a **superset of ARMv6-M** (M0/M0+): any M0 binary runs on an M3, but
  not the reverse.
- The core executes the **full Thumb-2** instruction set: variable-length encoding
  mixing 16-bit (density) and 32-bit (capability) instructions. There is **no
  ARM/Thumb interworking** — always Thumb state, **T bit** always 1.
- Because of Thumb state, **every function pointer / vector entry has LSB = 1**.
  Toolchains handle this automatically (it's why vector entries are `address | 1`).
- Compared with the M0/M0+ subset, Thumb-2 on the M3 **adds** (among others): hardware
  `SDIV/UDIV`, `IT` (If-Then) predication, `CBZ/CBNZ`, `TBB/TBH` table branches,
  bitfield `BFI/BFC/UBFX/SBFX`, `CLZ/RBIT`, `LDRD/STRD`, and `LDREX/STREX/CLREX`
  exclusives for lock-free synchronization.

---

## 3. Pipeline and Bus Interfaces

**3-stage pipeline:** Fetch → Decode → Execute, with speculative fetching and a branch
target forwarding path to minimize stalls. Same depth as the M0, but the M3 executes
far more per cycle thanks to the richer instruction set.

**Bus interfaces (AHB-Lite based)** — unlike the single-bus M0/M0+, the M3 has the
family's split-bus design:

| Bus       | Purpose                                                         |
|-----------|-----------------------------------------------------------------|
| I-Code    | Instruction fetches from Code region (0x0000_0000–0x1FFF_FFFF)  |
| D-Code    | Data loads/stores from the Code region                          |
| System    | Instruction + data for the rest of the map (SRAM, peripherals)  |
| PPB       | Private Peripheral Bus — internal core peripherals (NVIC etc.)  |

Splitting I-Code and D-Code lets the core fetch an instruction and load data from Flash
simultaneously, improving throughput over the M0/M0+.

---

## 4. Memory Model

The 4 GB space uses the standard M-profile fixed-region layout:

| Address range              | Region       | Typical use / attributes                  |
|----------------------------|--------------|-------------------------------------------|
| 0x0000_0000–0x1FFF_FFFF    | Code         | Flash / ROM; executable                   |
| 0x2000_0000–0x3FFF_FFFF    | SRAM         | On-chip RAM; **bit-band alias** here      |
| 0x4000_0000–0x5FFF_FFFF    | Peripheral   | Vendor peripherals; **bit-band alias** here |
| 0x6000_0000–0x9FFF_FFFF    | External RAM | Off-chip memory                           |
| 0xA000_0000–0xDFFF_FFFF    | External dev | Off-chip peripherals                      |
| 0xE000_0000–0xE00F_FFFF    | PPB          | System Control Space (NVIC, SCB, SysTick) |
| 0xE010_0000–0xFFFF_FFFF    | Vendor       | Vendor-specific                           |

**Memory types / ordering attributes** (defined by ARMv7-M):
- **Normal** — Flash/RAM; accesses may be reordered/merged by the core.
- **Device** — peripherals; kept in order, not merged/cached.
- **Strongly-ordered** — PPB and similar; strict, non-speculative.

**Endianness:** configurable, but essentially all devices are **little-endian**.

**Unaligned access:** unlike the M0/M0+, the M3 **supports unaligned** loads/stores to
normal memory for most instructions (exclusives and LDM/STM still require alignment).
Unaligned trapping can be forced on via `CCR.UNALIGN_TRP`.

---

## 5. Core Registers

Sixteen architecturally visible 32-bit registers, **R0–R15**:

| Register | Name  | Role                                                            |
|----------|-------|-----------------------------------------------------------------|
| R0–R7    | Low   | General purpose; reachable by all 16-bit Thumb instructions      |
| R8–R12   | High  | General purpose; mostly 32-bit instructions                     |
| R13      | SP    | **Stack Pointer** (banked: MSP or PSP — see §8)                 |
| R14      | LR    | **Link Register** — return address on `BL`/`BLX` and exceptions |
| R15      | PC    | **Program Counter** — reads as current instr + 4 (pipeline)     |

**Calling convention (AAPCS)**: R0–R3 args/scratch/return (caller-saved), R4–R11
callee-saved, R12 (IP) scratch, SP 8-byte aligned at public interfaces, return value in
R0 (R1 for 64-bit). Identical across the whole family.

There are **no floating-point registers** on the M3 (no FPU) — that is the M4/M7
addition.

---

## 6. Special Registers

Accessed with **MRS** (read) and **MSR** (write).

### 6.1 xPSR — Program Status Register

Composite of three views:

- **APSR** (Application PSR): condition flags **N, Z, C, V**. The M3 has **no Q flag and
  no GE bits** — those belong to the DSP-capable M4 (a subtle but real difference).
- **IPSR** (Interrupt PSR): the currently executing exception number (0 = thread mode).
- **EPSR** (Execution PSR): the **T bit** (always 1) plus **IT/ICI** bits for If-Then
  blocks and interrupted load/store multiples (present because the M3 has full Thumb-2,
  unlike the M0/M0+).

```
 31 30 29 28    27  26..24 ...  15..10 ...  8..0
 N  Z  C  V     (reserved) IT/ICI  IT/T      Exception number (IPSR)
```

### 6.2 Interrupt Masking Registers

The M3 restores the **full ARMv7-M masking set** absent on the M0/M0+:

| Register  | Width | Effect                                                             |
|-----------|-------|--------------------------------------------------------------------|
| PRIMASK   | 1 bit | =1 blocks **all** interrupts except NMI and HardFault             |
| FAULTMASK | 1 bit | =1 blocks all except NMI (also raises current priority to -1)     |
| BASEPRI   | 8 bit | Blocks all interrupts with priority **numerically ≥ BASEPRI**; 0 = disabled |

**BASEPRI** is the big one: it lets you mask only lower-priority interrupts while
letting high-priority ones through — the basis of efficient RTOS critical sections
(the M0/M0+, which have only PRIMASK, cannot do this). Use `CPSID i`/`CPSIE i` for
PRIMASK and `CPSID f`/`CPSIE f` for FAULTMASK.

### 6.3 CONTROL Register

| Bit | Name  | Meaning                                                    |
|-----|-------|------------------------------------------------------------|
| 0   | nPRIV | 0 = privileged Thread mode, 1 = unprivileged Thread mode   |
| 1   | SPSEL | 0 = use MSP, 1 = use PSP in Thread mode                    |

There is **no FPCA bit** (bit 2) — that appears only on FPU-equipped cores (M4F/M7).
After changing CONTROL (especially SPSEL), issue an **ISB**.

---

## 7. Operating Modes and Privilege

Two modes and two privilege levels:

| | **Thread mode** | **Handler mode** |
|---|---|---|
| Entered by | Reset / exception return | Taking any exception/interrupt |
| Privilege | Privileged **or** unprivileged (CONTROL.nPRIV) | Always privileged |
| Stack | MSP or PSP (CONTROL.SPSEL) | Always MSP |

A typical RTOS runs the kernel/ISRs privileged on MSP and application tasks
unprivileged on PSP, using the **MPU** to sandbox tasks — and on the M3 that sandboxing
actually works, backed by proper **MemManage** faults for diagnosis.

---

## 8. The Two Stacks

R13 (SP) is **banked** into two physical stack pointers:

- **MSP (Main Stack Pointer)** — used after reset, by all exception handlers, and by
  privileged thread code by default. Initial value = **first word of the vector table**
  at reset.
- **PSP (Process Stack Pointer)** — typically used by RTOS task code.

Thread mode's choice is selected by **CONTROL.SPSEL**; handler mode always uses MSP.
This keeps the kernel stack isolated from per-task stacks.

---

## 9. Exceptions and Interrupts (NVIC)

The M3 has the **full-featured NVIC** (the M0/M0+ have a cut-down version):

- **Up to 240 external interrupts** (IRQ0–IRQ239) — vs. 32 on the M0/M0+.
- **Up to 256 priority levels** (up to 8 bits of priority; vendors commonly implement
  3–4 top bits). The M0/M0+ have only 4 levels.
- **Priority grouping:** the priority field splits into **preempt priority** (controls
  nesting) and **subpriority** (ordering among equal preempt levels), configured via
  **AIRCR**. The M0/M0+ have no grouping.
- **Vectored, nested, tail-chaining, late-arrival** — all present.

### System exceptions

| # | Exception      | Priority        | Notes                                   |
|---|----------------|-----------------|-----------------------------------------|
| 1 | Reset          | -3 (highest)    | Boot                                    |
| 2 | NMI            | -2              | Non-maskable                            |
| 3 | HardFault      | -1              | Catch-all fault                         |
| 4 | MemManage      | configurable    | MPU violations                          |
| 5 | BusFault       | configurable    | Bus errors                              |
| 6 | UsageFault     | configurable    | Undefined instr, div-by-0, etc.         |
| 11| SVCall (SVC)   | configurable    | Supervisor call (`SVC #n`)              |
| 12| DebugMonitor   | configurable    | Debug events                            |
| 14| PendSV         | configurable    | Deferred context switch (RTOS)          |
| 15| SysTick        | configurable    | System timer tick                       |
| 16+| IRQ0..IRQn    | configurable    | Vendor peripheral interrupts            |

**Configurable faults are a key M3 advantage over the M0/M0+:** instead of everything
escalating to HardFault, MemManage/BusFault/UsageFault can be enabled and handled
individually, and their **fault-status registers** (CFSR/HFSR/MMFAR/BFAR) pinpoint the
cause.

**Key idioms:** **SVC** for system calls, **PendSV** (lowest priority) for the deferred
RTOS context switch, **SysTick** (24-bit down-counter) for the OS tick.

---

## 10. Exception Entry and EXC_RETURN

On exception entry the hardware **stacks 8 words** automatically:

```
   High address
   +---------+
   | xPSR    |
   | PC      |  (return address)
   | LR      |
   | R12     |
   | R3      |
   | R2      |
   | R1      |
   | R0      |
   +---------+
   Low address  <- SP after stacking
```

Because the M3 has **no FPU**, there is **no extended (FP) stack frame** — so EXC_RETURN
has only the three non-FP forms:

| EXC_RETURN | Return to | Stack used |
|------------|-----------|------------|
| 0xFFFFFFF1 | Handler   | MSP        |
| 0xFFFFFFF9 | Thread    | MSP        |
| 0xFFFFFFFD | Thread    | PSP        |

(The FP-frame EXC_RETURN values `0xFFFFFFE1/E9/ED` exist only on M4F/M7.) A C interrupt
handler needs no special prologue; PendSV swaps PSP and returns with `0xFFFFFFFD`.

---

## 11. Reset and Vector Table

At reset the core:

1. Reads **word 0** of the vector table (at 0x0000_0000 by default) → **initial MSP**.
2. Reads **word 1** → **reset vector** (`Reset_Handler`, LSB=1).
3. Begins in **privileged Thread mode on MSP**.

Vector table layout:

```
Offset  Content
0x00    Initial MSP value
0x04    Reset handler
0x08    NMI handler
0x0C    HardFault handler
0x10    MemManage handler
0x14    BusFault handler
0x18    UsageFault handler
...     reserved
0x2C    SVCall handler
0x38    PendSV handler
0x3C    SysTick handler
0x40+   IRQ0, IRQ1, ...
```

The table can be relocated at runtime via **VTOR** (in the SCB) — standard on the M3
(unlike the plain M0), useful for bootloaders and RAM-based vector tables.

---

## 12. System Control Space

The **PPB** at 0xE000_0000 hosts the core's own peripherals:

| Base        | Block    | Contents                                                   |
|-------------|----------|------------------------------------------------------------|
| 0xE000E010  | SysTick  | CTRL, LOAD, VAL, CALIB                                      |
| 0xE000E100  | NVIC     | ISER/ICER, ISPR/ICPR, IPR (priority)                       |
| 0xE000ED00  | SCB      | CPUID, ICSR, VTOR, AIRCR, SCR, CCR, SHPR, SHCSR, fault regs |
| 0xE000ED90  | MPU      | TYPE, CTRL, RNR, RBAR, RASR (optional)                     |

**SCB (System Control Block)** highlights:
- **CPUID** — identifies the core.
- **ICSR** — pending/active status; set PendSV/SysTick pending here.
- **AIRCR** — **priority grouping** and the **system reset request** (`SYSRESETREQ`).
- **CCR** — configuration: div-by-0 trap, unaligned trap, stack alignment.
- **SHPR1–3** — priorities of system handlers.
- **CFSR / HFSR / MMFAR / BFAR** — **fault status/address registers**, essential for
  debugging HardFaults. (These do **not** exist on the M0/M0+.)

---

## 13. The MPU

The optional ARMv7-M **Memory Protection Unit** (not an MMU — no translation, no
paging):

- Up to **8 regions**, each a power-of-two size **≥ 32 bytes** (finer than the M0+'s
  256-byte minimum), aligned to its size.
- Per-region access permissions (privileged/unprivileged, RO/RW/none), execute-never
  (XN), and memory type (normal/device/strongly-ordered, cache hints).
- **Region priority:** higher-numbered regions override where they overlap.
- **Sub-region disable:** split each region into 8 individually enabled sub-regions.
- Violations raise a **MemManage fault** (a *dedicated* fault with its own status in
  CFSR — on the M0+ the same violation would just be a HardFault).

Registers: **MPU_TYPE, MPU_CTRL, MPU_RNR, MPU_RBAR, MPU_RASR**. Always `DSB; ISB` after
reconfiguring. Used for RTOS task isolation, null-pointer/stack-overflow detection, and
marking Flash read-only/execute-only.

---

## 14. Bit-Banding

The M3 implements ARMv7-M **bit-banding**: two 1 MB regions (in SRAM and Peripheral
space) each have a **32 MB alias region** where each **word** in the alias maps to a
**single bit** in the base region:

```
alias_word_addr = alias_base + (byte_offset * 32) + (bit * 4)
```

Writing 0/1 to the alias word performs an **atomic** single-bit set/clear in the
underlying memory — no read-modify-write, so it is interrupt-safe for flag bits. (The
M0/M0+ have **no** bit-banding; there you must mask interrupts around read-modify-write.)
Bit-banding is architecturally defined but *optional* — check the vendor datasheet.

---

## 15. Debug Architecture

The M3 has the full CoreSight-based debug subsystem (much richer than the M0/M0+):

| Block | Purpose                                                                   |
|-------|---------------------------------------------------------------------------|
| DWT   | Data Watchpoint & Trace — watchpoints, cycle counter (CYCCNT), profiling  |
| FPB   | Flash Patch & Breakpoint — hardware breakpoints, code patching            |
| ITM   | Instrumentation Trace Macrocell — `printf`-style trace (SWO output)       |
| TPIU  | Trace Port Interface Unit — funnels trace off-chip                        |
| ETM   | (optional) Embedded Trace Macrocell — full instruction trace              |

- **Debug access** via **SWD** (2-wire) or **JTAG**.
- **DWT->CYCCNT** is a free 32-bit cycle counter — great for micro-benchmarking (the
  M0/M0+ have no such counter).
- **ITM** emits trace with minimal overhead through the **SWO** pin.

---

## 16. Low-Power and Sleep

Architectural sleep support (power domains are vendor-specific):

- **WFI** (Wait For Interrupt), **WFE** (Wait For Event).
- **Sleep-on-exit** (SCR bit) — auto re-sleep when the last ISR returns.
- **SLEEPDEEP** (SCR bit) — request a deeper sleep mode.
- **SEVONPEND** — pending interrupts wake even when masked.

Registers: **SCB->SCR**. The M3 targets performance more than the ultra-low-power M0+,
so at equal process it usually draws more current per MHz than an M0+.

---

## 17. Instruction Set Highlights

Full Thumb-2, by category (superset of the M0/M0+ set):

- **Data processing**: `MOV, MVN, ADD, SUB, RSB, MUL, MLA, UMULL, SMULL, SDIV, UDIV`
  (**hardware divide!**), `AND, ORR, EOR, BIC`.
- **Shifts**: `LSL, LSR, ASR, ROR, RRX` (often folded into other instructions).
- **Compare/test**: `CMP, CMN, TST, TEQ`.
- **Bitfield**: `BFI, BFC, UBFX, SBFX, CLZ, RBIT, REV, REV16`.
- **Load/store**: `LDR, STR` (+B/H/SB/SH), `LDM/STM`, `PUSH/POP`, `LDRD/STRD`.
- **Exclusive access**: `LDREX, STREX, CLREX` — spinlocks and lock-free structures.
- **Branch**: `B, BL, BX, BLX, CBZ/CBNZ, TBB/TBH`.
- **IT block**: `IT` makes up to 4 following instructions conditional without branching.
- **System**: `MRS, MSR, CPS, SVC, BKPT, NOP, WFI, WFE, SEV, DSB, DMB, ISB`.

What the M3 **does not** have (vs. the M4): **no saturating/SIMD DSP instructions**
(`QADD`, `SMLAD`, `SSAT`, etc.) and **no floating-point** instructions.

---

## 18. Memory Barriers

Three barrier instructions:

| Instr | Meaning                                                                   |
|-------|---------------------------------------------------------------------------|
| DMB   | **Data Memory Barrier** — orders memory accesses                           |
| DSB   | **Data Synchronization Barrier** — waits until prior accesses complete     |
| ISB   | **Instruction Synchronization Barrier** — flushes pipeline, refetch        |

**When you need them:** after writing registers that change memory maps, priorities,
CONTROL, or (on FPU cores) the FPU enable → **DSB; ISB**. Around setting/clearing pending
interrupts that must take effect immediately, and around shared memory with DMA/other
bus masters. On a single-core, in-order M3 you rarely need `DMB` for ordinary data
ordering.

---

## 19. What the Cortex-M3 Has (vs. M0/M0+) and Lacks (vs. M4)

**Adds over the M0/M0+ (all from being ARMv7-M):**
- Full **Thumb-2**: hardware **divide**, `IT`, `CBZ/CBNZ`, `TBB/TBH`, bitfield ops,
  exclusives (`LDREX/STREX`).
- Full **NVIC**: up to 240 IRQs, up to 256 priority levels, **priority grouping**.
- **BASEPRI** and **FAULTMASK** (not just PRIMASK).
- **Configurable faults** (MemManage/BusFault/UsageFault) with **CFSR/MMFAR/BFAR**.
- **Bit-banding**; **unaligned access**; **VTOR** as standard.
- Finer **MPU** (32-byte min region) with a dedicated MemManage fault.
- Rich **debug/trace** (DWT CYCCNT, ITM/SWO, FPB, optional ETM); split I/D buses.

**Lacks vs. the M4 (what the M4 adds):**
- **No FPU** — all floating point is software.
- **No DSP/SIMD** — no single-cycle MAC, no packed 8/16-bit ops, no **saturating
  arithmetic** and hence **no Q flag / GE bits**.

So: learn the ARMv7-M model on the M3, then the M4 is "the same, plus math."

---

## 20. Bring-Up Mental Model

A useful order to think about when a Cortex-M3 boots:

```
1. Power/clock stable  ->  core reads MSP (vector[0]) and PC (vector[1])
2. Reset_Handler runs  ->  copy .data to RAM, zero .bss, set up clocks
3. (optional) relocate vector table via VTOR
4. configure MPU regions if used; DSB; ISB
5. set interrupt priorities (NVIC IPR / SCB SHPR) and grouping (AIRCR)
6. enable peripheral IRQs (NVIC ISER)
7. (optional) drop privilege / switch to PSP for an RTOS (write CONTROL; ISB)
8. call main() -> scheduler / superloop
```

(No FPU-enable step — that only exists on M4F/M7.)

**Debugging a HardFault** (much easier than on the M0/M0+, thanks to fault status regs):
1. Break in **HardFault_Handler**.
2. Read **SCB->HFSR**; if `FORCED`, the real cause escalated from a configurable fault.
3. Read **SCB->CFSR** (MemManage/BusFault/UsageFault sub-fields) for the fault class.
4. If the address-valid bits are set, read **MMFAR/BFAR** for the faulting address.
5. Recover the **stacked PC** from the exception frame (MSP or PSP per EXC_RETURN bit 2)
   to find the offending instruction.

---

## 21. Further References

**Primary ARM documentation (authoritative, vendor-agnostic):**
- *ARM®v7-M Architecture Reference Manual* (ARM DDI 0403) — the definitive spec for the
  instruction set, exception model, and system registers (shared with M4).
- *Cortex®-M3 Technical Reference Manual* (ARM DDI 0337) — the core implementation
  details (pipeline, buses, timing).
- *Cortex-M3 Devices Generic User Guide* (ARM DUI 0552) — the approachable programmer's-
  model document; covers NVIC, SCB, MPU register-by-register.
- *AAPCS — Procedure Call Standard for the ARM Architecture* (calling convention).

**Books:**
- Joseph Yiu, *The Definitive Guide to ARM® Cortex®-M3 and Cortex®-M4 Processors* — the
  single best book on these cores; written by an ARM engineer and covers M3 and M4
  side by side (ideal for seeing exactly what the M4 adds).
- Jonathan Valvano, *Embedded Systems: Introduction to ARM Cortex-M Microcontrollers*.

**Software / hands-on:**
- **CMSIS** — `core_cm3.h` maps all the core registers described here; read it alongside
  this guide to connect concepts to code.
- **FreeRTOS Cortex-M3 port** — study the PendSV context switch and note how it uses
  **BASEPRI** for critical sections (the M0 port cannot, having only PRIMASK).

**Practical exercises to cement the concepts:**
1. Write a startup file + linker script by hand (vector table, MSP init, .data/.bss).
2. Trigger and decode a fault deliberately (div-by-0 with `CCR.DIV_0_TRP`, or an
   MPU violation) and read CFSR/MMFAR to identify it.
3. Implement a minimal PendSV context switch between two tasks using PSP + BASEPRI.
4. Toggle a GPIO via bit-banding and compare with read-modify-write.
5. Use DWT->CYCCNT to measure a function's cycle count and compare `SDIV` (hardware)
   with an M0's software divide.

---

*This guide describes the ARM Cortex-M3 core as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
