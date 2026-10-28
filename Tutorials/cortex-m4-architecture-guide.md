# ARM Cortex-M4 Architecture — A Vendor-Agnostic Guide

> Focus: the ARM **core** itself (the parts every silicon vendor shares), not any
> specific microcontroller's peripherals. Everything here is defined by ARM in the
> *ARMv7-M Architecture Reference Manual* and the *Cortex-M4 Technical Reference Manual*.

---

## Table of Contents

- [ARM Cortex-M4 Architecture — A Vendor-Agnostic Guide](#arm-cortex-m4-architecture--a-vendor-agnostic-guide)
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
    - [System exceptions (fixed low numbers)](#system-exceptions-fixed-low-numbers)
  - [10. Exception Entry and EXC\_RETURN](#10-exception-entry-and-exc_return)
    - [On exception entry (hardware, automatic)](#on-exception-entry-hardware-automatic)
    - [EXC\_RETURN](#exc_return)
  - [11. Reset and Vector Table](#11-reset-and-vector-table)
  - [12. System Control Space](#12-system-control-space)
  - [13. The FPU (FPv4-SP)](#13-the-fpu-fpv4-sp)
  - [14. The MPU](#14-the-mpu)
  - [15. DSP Extension](#15-dsp-extension)
  - [16. Bit-Banding](#16-bit-banding)
  - [17. Debug Architecture](#17-debug-architecture)
  - [18. Low-Power and Sleep](#18-low-power-and-sleep)
  - [19. Instruction Set Highlights](#19-instruction-set-highlights)
  - [20. Memory Barriers](#20-memory-barriers)
  - [21. Bring-Up Mental Model](#21-bring-up-mental-model)
  - [22. Further References](#22-further-references)

---

## 1. Big Picture

The **Cortex-M4** is a 32-bit microcontroller core implementing the **ARMv7-M**
architecture profile. Key characteristics:

- **32-bit** data path, 32-bit addresses → 4 GB linear address space.
- **Harvard-ish** internal buses (separate instruction/data fetch paths) but a
  **single unified 4 GB memory map** (a programmer sees one address space).
- **Thumb-2 only**: it does not execute the classic 32-bit ARM instruction set.
  It runs a mix of 16-bit and 32-bit Thumb instructions.
- **3-stage pipeline** (fetch, decode, execute) with branch speculation.
- Deterministic, low-latency **interrupt handling** via the built-in **NVIC**.
- Optional **FPU** (single precision), **MPU**, and **DSP** instructions.

The "M4" is essentially a **Cortex-M3 + DSP extension (+ optional single-precision
FPU)**. The variant with the FPU is often marketed as **Cortex-M4F**.

```
          +--------------------------------------------------+
          |                  Cortex-M4 core                  |
          |  +-----------+   +---------+    +--------------+  |
          |  | 3-stage   |   |  NVIC   |    |  FPU (opt.)  |  |
          |  | pipeline  |   | (IRQ)   |    |  FPv4-SP     |  |
          |  +-----------+   +---------+    +--------------+  |
          |  +-----------+   +---------+    +--------------+  |
          |  | Register  |   |  MPU    |    |  Debug (DWT, |  |
          |  | bank+PSR  |   | (opt.)  |    |  ITM, FPB)   |  |
          |  +-----------+   +---------+    +--------------+  |
          +----------|------------|--------------|-----------+
                   I-Code       D-Code         System
                    bus          bus            bus
```

---

## 2. ARMv7-M and Thumb-2

- **ARMv7-M** is the "Microcontroller" profile of the ARMv7 architecture (vs. -A
  Application and -R Real-time profiles).
- The core executes the **Thumb-2** instruction set: variable-length encoding
  where common instructions are 16-bit (code density) and more complex ones are
  32-bit (capability). There is **no ARM/Thumb interworking** — the processor is
  always in Thumb state. The **T bit** in the program status register is always 1;
  clearing it causes a fault.
- Because of this, **every function pointer / vector must have its LSB set to 1**
  to indicate Thumb state. Toolchains handle this automatically, but it explains
  why vector table entries are `address | 1`.

---

## 3. Pipeline and Bus Interfaces

**3-stage pipeline:** Fetch → Decode → Execute. Branches may incur a small penalty;
the core includes speculative fetching and a branch target forwarding path to
minimize stalls.

**Bus interfaces (AHB-Lite based):**

| Bus       | Purpose                                                        |
|-----------|----------------------------------------------------------------|
| I-Code    | Instruction fetches from Code region (0x0000_0000–0x1FFF_FFFF) |
| D-Code    | Data loads/stores from the Code region                        |
| System    | Instruction + data for the rest of the map (SRAM, peripherals)|
| PPB       | Private Peripheral Bus — internal core peripherals (NVIC etc.)|

Splitting I-Code and D-Code lets the core fetch an instruction and load data from
Flash simultaneously, improving throughput.

---

## 4. Memory Model

The 4 GB space is architecturally divided into fixed regions. Vendors place their
Flash/RAM/peripherals inside these regions, but the region *boundaries and default
attributes* are defined by ARM:

| Address range              | Region       | Typical use / attributes                  |
|----------------------------|--------------|-------------------------------------------|
| 0x0000_0000–0x1FFF_FFFF    | Code         | Flash / ROM; executable                   |
| 0x2000_0000–0x3FFF_FFFF    | SRAM         | On-chip RAM; bit-band alias here          |
| 0x4000_0000–0x5FFF_FFFF    | Peripheral   | Vendor peripherals; bit-band alias here   |
| 0x6000_0000–0x9FFF_FFFF    | External RAM | Off-chip memory                           |
| 0xA000_0000–0xDFFF_FFFF    | External dev | Off-chip peripherals                      |
| 0xE000_0000–0xE00F_FFFF    | PPB          | System Control Space (NVIC, SCB, SysTick) |
| 0xE010_0000–0xFFFF_FFFF    | Vendor       | Vendor-specific                           |

**Memory types / ordering attributes:**
- **Normal** — Flash/RAM; access can be reordered/merged by the core.
- **Device** — peripherals; accesses kept in order, not merged/cached.
- **Strongly-ordered** — PPB and similar; strict, non-speculative, completes before
  next access.

**Endianness:** Cortex-M4 supports little- or big-endian, but virtually all real
devices are configured **little-endian**.

**Unaligned access:** Most normal loads/stores may be unaligned; certain
instructions (exclusives, multi-word LDM/STM) require alignment. Unaligned faults
can be enabled via `CCR.UNALIGN_TRP`.

---

## 5. Core Registers

The core has 16 architecturally visible 32-bit registers, **R0–R15**:

| Register | Name  | Role                                                            |
|----------|-------|-----------------------------------------------------------------|
| R0–R7    | Low   | General purpose; accessible by all 16-bit Thumb instructions    |
| R8–R12   | High  | General purpose; mostly 32-bit instructions                     |
| R13      | SP    | **Stack Pointer** (banked: MSP or PSP — see §8)                 |
| R14      | LR    | **Link Register** — return address on `BL`/`BLX` and exceptions |
| R15      | PC    | **Program Counter** — reads as current instr + 4 (pipeline)     |

**Calling convention (AAPCS) essentials:**
- **R0–R3**: argument / scratch / return values (caller-saved).
- **R4–R11**: callee-saved (must be preserved across a function call).
- **R12 (IP)**: intra-procedure scratch.
- **SP** must be 8-byte aligned at public interfaces.
- Return value in R0 (and R1 for 64-bit).

If the FPU is present, **S0–S31** (or D0–D15) floating-point registers are added;
see §13.

---

## 6. Special Registers

Accessed with the **MRS** (read) and **MSR** (write) instructions — *not* with
ordinary MOV.

### 6.1 xPSR — Program Status Register

The PSR is a composite of three views, readable together or separately:

- **APSR** (Application PSR): condition flags
  - **N** Negative, **Z** Zero, **C** Carry, **V** Overflow, **Q** saturation
    (sticky, set by DSP/saturating instructions), plus **GE[3:0]** for SIMD.
- **IPSR** (Interrupt PSR): the **currently executing exception number** (0 = thread
  mode, non-zero = handler for that exception).
- **EPSR** (Execution PSR): the **T bit** (Thumb state, always 1) and
  IT/ICI bits for If-Then blocks and interrupted load/store multiples.

```
 31 30 29 28 27  26..24 ... 15..10 ...  8..0
 N  Z  C  V  Q     IT/ICI     IT/T       Exception number (IPSR)
```

### 6.2 Interrupt Masking Registers

| Register  | Width | Effect                                                             |
|-----------|-------|--------------------------------------------------------------------|
| PRIMASK   | 1 bit | =1 blocks **all** interrupts except NMI and HardFault             |
| FAULTMASK | 1 bit | =1 blocks all except NMI (also raises current priority to -1)     |
| BASEPRI   | 8 bit | Blocks all interrupts with priority **numerically ≥ BASEPRI**; 0 = disabled |

Use `CPSID i` / `CPSIE i` to set/clear PRIMASK quickly (critical sections),
and `CPSID f` / `CPSIE f` for FAULTMASK.

### 6.3 CONTROL Register

| Bit | Name  | Meaning                                                             |
|-----|-------|---------------------------------------------------------------------|
| 0   | nPRIV | 0 = privileged Thread mode, 1 = unprivileged Thread mode            |
| 1   | SPSEL | 0 = use MSP, 1 = use PSP in Thread mode                             |
| 2   | FPCA  | FP context active (set when FP instructions used; drives lazy stacking) |

After changing CONTROL (especially SPSEL), issue an **ISB** so the pipeline uses the
new setting.

---

## 7. Operating Modes and Privilege

Cortex-M4 has **two modes** and **two privilege levels**:

| | **Thread mode** | **Handler mode** |
|---|---|---|
| Entered by | Reset / exception return | Taking any exception/interrupt |
| Privilege | Privileged **or** unprivileged (CONTROL.nPRIV) | Always privileged |
| Stack | MSP or PSP (CONTROL.SPSEL) | Always MSP |

```
        Reset
          |
          v
   +---------------+   exception    +----------------+
   |  Thread mode  | -------------> |  Handler mode  |
   | (app code)    | <------------- |  (ISRs, faults)|
   +---------------+   EXC_RETURN   +----------------+
```

A typical RTOS runs the kernel/ISRs privileged on MSP and application tasks
unprivileged on PSP, using the MPU to sandbox tasks.

---

## 8. The Two Stacks

R13 (SP) is **banked** into two physical stack pointers:

- **MSP (Main Stack Pointer)** — used after reset, by all exception handlers, and
  by privileged thread code by default. Its initial value is loaded from the **first
  word of the vector table** at reset.
- **PSP (Process Stack Pointer)** — typically used by application/task code under an
  RTOS.

Which one Thread mode uses is selected by **CONTROL.SPSEL**. Handler mode always
uses MSP. You can read/write each explicitly via `MRS/MSR MSP`/`PSP`.

This separation lets an OS keep its kernel stack (MSP) isolated from per-task stacks
(PSP), so a task stack overflow doesn't corrupt the kernel.

---

## 9. Exceptions and Interrupts (NVIC)

The **Nested Vectored Interrupt Controller (NVIC)** is part of the core. Features:

- **Vectored**: hardware jumps directly to the handler address from the vector table
  — no software dispatch needed.
- **Nested**: a higher-priority exception can preempt a lower-priority handler.
- **Priority**: each exception has a configurable priority (except the fixed ones).
  Lower number = higher priority.
- **Tail-chaining** and **late-arrival** optimizations reduce latency between
  back-to-back interrupts (no redundant unstack/restack).

### System exceptions (fixed low numbers)

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

**Priority grouping:** The 8-bit priority field is split into **preempt priority**
(determines nesting) and **subpriority** (order among same preempt level). Note: many
vendors implement only the top 3–4 bits of the priority field; the lower bits read as
zero.

**Key idioms:**
- **SVC**: synchronous "system call" from unprivileged code into privileged kernel.
- **PendSV**: set-pending, lowest-priority handler used to perform the actual task
  switch *after* all higher-priority ISRs finish — the standard RTOS context-switch
  mechanism.
- **SysTick**: 24-bit down-counter for the OS time base.

---

## 10. Exception Entry and EXC_RETURN

### On exception entry (hardware, automatic)

The core **stacks 8 words** onto the current stack (this is the "stack frame"):

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

If the FPU context is active, an **extended frame** additionally stacks S0–S15 and
FPSCR (see lazy stacking, §13).

Because R0–R3, R12, LR, PC, xPSR are saved automatically, an ISR written in C needs
**no special prologue** — it just preserves R4–R11 like any function.

### EXC_RETURN

On entry, **LR is loaded with a special value called EXC_RETURN** (top bits all 1,
e.g. `0xFFFF_FFFxx`). Its low bits encode how to return:

| EXC_RETURN | Return to  | Stack used | FP frame |
|------------|-----------|------------|----------|
| 0xFFFFFFF1 | Handler   | MSP        | no       |
| 0xFFFFFFF9 | Thread    | MSP        | no       |
| 0xFFFFFFFD | Thread    | PSP        | no       |
| 0xFFFFFFE1 | Handler   | MSP        | yes      |
| 0xFFFFFFE9 | Thread    | MSP        | yes      |
| 0xFFFFFFED | Thread    | PSP        | yes      |

Executing a `BX LR` (or any branch) with this magic value in the PC triggers the
**exception return** sequence: the core unstacks the frame and resumes. This is how
context switches work — PendSV swaps PSP and returns with `0xFFFFFFFD`.

---

## 11. Reset and Vector Table

At reset the core:

1. Reads **word 0** of the vector table (at 0x0000_0000 by default) → **initial MSP**.
2. Reads **word 1** → **reset vector** (address of `Reset_Handler`, LSB=1).
3. Begins executing in **privileged Thread mode on MSP**.

Vector table layout (each entry is a 32-bit handler address):

```
Offset  Content
0x00    Initial MSP value        <-- not a handler, an address value
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
0x40+   IRQ0, IRQ1, ...          <-- vendor interrupts
```

The table can be relocated at runtime via **VTOR** (Vector Table Offset Register in
the SCB) — useful for bootloaders and RAM-based vector tables.

---

## 12. System Control Space

The **PPB** at 0xE000_0000 hosts the core's own peripherals. Important blocks:

| Base        | Block    | Contents                                                   |
|-------------|----------|------------------------------------------------------------|
| 0xE000E010  | SysTick  | CTRL, LOAD, VAL, CALIB                                      |
| 0xE000E100  | NVIC     | ISER/ICER (enable), ISPR/ICPR (pending), IPR (priority)   |
| 0xE000ED00  | SCB      | CPUID, ICSR, VTOR, AIRCR, SCR, CCR, SHPR, SHCSR, fault regs|
| 0xE000ED90  | MPU      | TYPE, CTRL, RNR, RBAR, RASR                                 |
| 0xE000EF30  | FPU      | CPACR (in SCB), FPCCR, FPCAR, FPDSCR                        |

**SCB (System Control Block)** highlights:
- **CPUID** — identifies the core.
- **ICSR** — pending/active exception status; set PendSV/SysTick pending here.
- **AIRCR** — priority grouping and the **system reset request** (`SYSRESETREQ`).
- **CCR** — configuration: div-by-0 trap, unaligned trap, stack alignment.
- **SHPR1–3** — priorities of system handlers.
- **CFSR / HFSR / MMFAR / BFAR** — **fault status/address registers**, essential for
  debugging HardFaults.

---

## 13. The FPU (FPv4-SP)

The Cortex-M4F includes an **FPv4-SP** single-precision floating-point unit:

- **32-bit (single precision, `float`) only** — `double` is emulated in software.
- Register bank **S0–S31** (viewable as D0–D15 pairs).
- **FPSCR** status/control register (rounding mode, exception flags, N/Z/C/V for FP
  compares).
- IEEE-754 compliant; hardware add, sub, mul, divide, sqrt, MAC.

**Enabling the FPU** (it is *off* at reset): set the CP10/CP11 access bits in
**CPACR** (`SCB->CPACR |= (0xF << 20);`) then issue DSB + ISB.

**Lazy stacking (FPCCR):** To keep interrupt latency low, on exception entry the core
reserves space for the FP registers but only actually saves S0–S15 **if and when** the
handler uses the FPU. This avoids paying the FP-stacking cost for ISRs that never touch
floats. Controlled by `LSPEN`/`ASPEN` in **FPCCR**.

---

## 14. The MPU

The optional **Memory Protection Unit** provides region-based protection (it is *not*
an MMU — no virtual address translation, no paging):

- Up to **8 regions** (0–7), each with a base, size (power-of-two, ≥32 bytes),
  and attributes.
- Per-region: access permissions (privileged/unprivileged, RO/RW/none),
  execute-never (XN), and memory type (normal/device/strongly-ordered, cache hints).
- **Region priority**: higher-numbered regions override lower where they overlap.
- Sub-region disable: each region can be split into 8 sub-regions individually enabled.
- Violations raise a **MemManage** fault.

Used to enforce task isolation (RTOS), catch null-pointer/stack-overflow writes, and
mark Flash as read-only / execute-only.

Registers: **MPU_TYPE, MPU_CTRL, MPU_RNR** (region number), **MPU_RBAR** (base addr),
**MPU_RASR** (size + attributes). Always `DSB; ISB` after reconfiguring.

---

## 15. DSP Extension

The "M4 vs M3" differentiator (besides FPU) is the **DSP instruction set**:

- **Single-cycle MAC** (multiply-accumulate) and 32×32→64 operations.
- **SIMD** on packed 8/16-bit data (e.g. `QADD8`, `SADD16`, `SMLAD`).
- **Saturating arithmetic** (`QADD`, `QSUB`, `SSAT`, `USAT`) — sets the **Q flag** on
  saturation, invaluable for fixed-point audio/control DSP.
- Packing/unpacking, count-leading-zeros, etc.

Combined with the FPU, the M4 targets motor control, audio, sensor fusion, and
digital filtering — ARM's **CMSIS-DSP** library is built on these instructions.

---

## 16. Bit-Banding

Two 1 MB regions (in SRAM and Peripheral space) have **32 MB alias regions** where
each **word** in the alias maps to a **single bit** in the base region:

```
alias_word_addr = alias_base + (byte_offset * 32) + (bit * 4)
```

Writing 0/1 to the alias word performs an **atomic** single-bit set/clear in the
underlying memory — no read-modify-write, so it is interrupt-safe for flag bits.
(Note: bit-banding is defined by the architecture but *optional* — not every M4
silicon implements it, so check the vendor datasheet.)

---

## 17. Debug Architecture

Cortex-M4 includes a rich, standardized debug subsystem (CoreSight-based):

| Block | Purpose                                                                   |
|-------|---------------------------------------------------------------------------|
| DWT   | Data Watchpoint & Trace — watchpoints, cycle counter (CYCCNT), profiling  |
| FPB   | Flash Patch & Breakpoint — hardware breakpoints, code patching            |
| ITM   | Instrumentation Trace Macrocell — `printf`-style trace (SWO output)       |
| TPIU  | Trace Port Interface Unit — funnels trace off-chip                        |
| ETM   | (optional) Embedded Trace Macrocell — full instruction trace              |

- **Debug access** via **SWD** (2-wire) or **JTAG**.
- **Halting** vs **DebugMonitor** debug modes.
- **DWT->CYCCNT** is a free 32-bit cycle counter — great for micro-benchmarking.
- **ITM** lets you emit trace with minimal overhead through the **SWO** pin.

---

## 18. Low-Power and Sleep

Architectural sleep support (actual power domains are vendor-specific):

- **WFI** (Wait For Interrupt) — sleep until an interrupt.
- **WFE** (Wait For Event) — sleep until an event (event register / `SEV`).
- **Sleep-on-exit** (SCR bit) — automatically re-enter sleep when the last ISR returns;
  ideal for pure interrupt-driven designs.
- **SLEEPDEEP** (SCR bit) — request a deeper sleep mode; the vendor's power controller
  interprets this to gate clocks/regulators.
- **SEVONPEND** — pending interrupts generate wake-up events even when masked.

Registers: **SCB->SCR** (System Control Register).

---

## 19. Instruction Set Highlights

A non-exhaustive tour of Thumb-2 categories useful to know:

- **Data processing**: `MOV, MVN, ADD, SUB, RSB, MUL, MLA, UMULL, SDIV, UDIV`
  (hardware divide!), `AND, ORR, EOR, BIC`.
- **Shifts**: `LSL, LSR, ASR, ROR, RRX` (often folded into other instructions).
- **Compare/test**: `CMP, CMN, TST, TEQ` (set flags only).
- **Bitfield**: `BFI, BFC, UBFX, SBFX, CLZ, RBIT, REV, REV16`.
- **Load/store**: `LDR, STR` (+B/H/SB/SH variants), `LDM/STM` (multiple),
  `PUSH/POP`, `LDRD/STRD` (doubleword).
- **Exclusive access** (atomics/synchronization): `LDREX, STREX, CLREX` — build
  spinlocks and lock-free structures.
- **Branch**: `B, BL, BX, BLX, CBZ/CBNZ` (compare-and-branch-zero), `TBB/TBH`
  (table branch for switch statements).
- **IT block**: `IT` makes up to 4 following instructions conditional without
  branching — cheap predication.
- **System**: `MRS, MSR, CPS, SVC, BKPT, NOP, WFI, WFE, SEV, DSB, DMB, ISB`.
- **Saturating/SIMD/FP**: see §13 and §15.

---

## 20. Memory Barriers

Three barrier instructions matter on Cortex-M4:

| Instr | Meaning                                                                    |
|-------|----------------------------------------------------------------------------|
| DMB   | **Data Memory Barrier** — orders memory accesses (before vs after)          |
| DSB   | **Data Synchronization Barrier** — waits until all prior accesses complete  |
| ISB   | **Instruction Synchronization Barrier** — flushes pipeline, refetch         |

**When you actually need them:**
- After writing a register that changes memory maps, priorities, CONTROL, or the FPU
  enable → use **DSB; ISB** so subsequent instructions see the new state.
- After enabling/disabling interrupts where ordering matters.
- Setting/clearing an interrupt as pending and needing it to take effect immediately.
- On single-core M4 you generally **don't** need DMB for normal data ordering (the
  core is in-order, single master), but it's required for correctness around shared
  memory with DMA/other bus masters.

---

## 21. Bring-Up Mental Model

A useful order to think about when a Cortex-M4 boots:

```
1. Power/clock stable  ->  core reads MSP (vector[0]) and PC (vector[1])
2. Reset_Handler runs  ->  copy .data to RAM, zero .bss, set up clocks
3. (optional) relocate vector table via VTOR
4. enable FPU (CPACR) if using floats; DSB/ISB
5. configure MPU regions if used; DSB/ISB
6. set interrupt priorities (NVIC IPR / SCB SHPR), grouping (AIRCR)
7. enable peripheral IRQs (NVIC ISER)
8. drop privilege / switch to PSP if running an RTOS (write CONTROL; ISB)
9. call main() -> scheduler / superloop
```

**Debugging a HardFault** (the rite of passage):
1. Break in HardFault_Handler.
2. Read **SCB->HFSR**; if `FORCED`, the real cause escalated from a configurable fault.
3. Read **SCB->CFSR** (MemManage/BusFault/UsageFault sub-fields).
4. If address valid bits set, read **MMFAR/BFAR** for the faulting address.
5. Recover the **stacked PC** from the exception frame (MSP or PSP per EXC_RETURN bit 2)
   to find the offending instruction.

---

## 22. Further References

**Primary ARM documentation (authoritative, vendor-agnostic):**
- *ARM®v7-M Architecture Reference Manual* (ARM DDI 0403) — the definitive spec for
  the instruction set, exception model, and system registers.
- *Cortex®-M4 Technical Reference Manual* (ARM DDI 0439) — the core implementation
  details (pipeline, buses, timing).
- *Cortex-M4 Devices Generic User Guide* (ARM DUI 0553) — the most approachable ARM
  document; covers programmer's model, NVIC, SCB, MPU, FPU register-by-register.
- *ARM Cortex-M Programming Guide to Memory Barrier Instructions* (application note).
- *AAPCS — Procedure Call Standard for the ARM Architecture* (for the calling convention).

**Books:**
- Joseph Yiu, *The Definitive Guide to ARM® Cortex®-M3 and Cortex®-M4 Processors* —
  the single best book on this core; written by an ARM engineer.
- Jonathan Valvano, *Embedded Systems: Introduction to ARM Cortex-M Microcontrollers*
  (series) — hands-on, teaching-oriented.
- Alex White / Trevor Martin, *The Designer's Guide to the Cortex-M Processor Family*.

**Software / hands-on:**
- **CMSIS** (Common Microcontroller Software Interface Standard) — ARM's standard
  headers (`core_cm4.h`) that map all the core registers described here; read this
  header alongside this guide to connect concepts to code.
- **CMSIS-DSP** — optimized DSP library using the M4's DSP/FPU instructions.
- **CMSIS-RTOS / FreeRTOS Cortex-M4 port** — study the PendSV context switch to see
  MSP/PSP, EXC_RETURN, and the stack frame in action.

**Practical exercises to cement the concepts:**
1. Write a startup file + linker script by hand (vector table, MSP init, .data/.bss).
2. Trigger and decode a HardFault deliberately (e.g., unaligned access, div-by-0).
3. Implement a minimal PendSV context switch between two tasks using PSP.
4. Toggle a GPIO via bit-banding and compare with read-modify-write.
5. Use DWT->CYCCNT to measure a function's cycle count.
6. Enable the FPU and observe lazy stacking with an FP operation inside an ISR.

---

*This guide describes the ARM Cortex-M4 core as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
