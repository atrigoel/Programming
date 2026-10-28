# ARM Cortex-M0 Architecture — A Vendor-Agnostic Guide

> Focus: the ARM **core** itself (the parts every silicon vendor shares), not any
> specific microcontroller's peripherals. Everything here is defined by ARM in the
> *ARMv6-M Architecture Reference Manual* and the *Cortex-M0 Technical Reference Manual*.

---

## Table of Contents

- [ARM Cortex-M0 Architecture — A Vendor-Agnostic Guide](#arm-cortex-m0-architecture--a-vendor-agnostic-guide)
  - [Table of Contents](#table-of-contents)
  - [1. Big Picture](#1-big-picture)
  - [2. ARMv6-M and the Thumb Subset](#2-armv6-m-and-the-thumb-subset)
  - [3. Pipeline and Bus Interface](#3-pipeline-and-bus-interface)
  - [4. Memory Model](#4-memory-model)
  - [5. Core Registers](#5-core-registers)
  - [6. Special Registers](#6-special-registers)
    - [6.1 xPSR — Program Status Register](#61-xpsr--program-status-register)
    - [6.2 Interrupt Masking (PRIMASK only)](#62-interrupt-masking-primask-only)
    - [6.3 CONTROL Register](#63-control-register)
  - [7. Operating Modes and Privilege](#7-operating-modes-and-privilege)
  - [8. The Two Stacks](#8-the-two-stacks)
  - [9. Exceptions and Interrupts (NVIC)](#9-exceptions-and-interrupts-nvic)
    - [System exceptions](#system-exceptions)
  - [10. Exception Entry and EXC\_RETURN](#10-exception-entry-and-exc_return)
  - [11. Reset and Vector Table](#11-reset-and-vector-table)
  - [12. System Control Space](#12-system-control-space)
  - [13. Instruction Set Highlights](#13-instruction-set-highlights)
  - [14. Memory Barriers](#14-memory-barriers)
  - [15. Low-Power and Sleep](#15-low-power-and-sleep)
  - [16. Debug Architecture](#16-debug-architecture)
  - [17. What the Cortex-M0 Does *Not* Have](#17-what-the-cortex-m0-does-not-have)
  - [18. Bring-Up Mental Model](#18-bring-up-mental-model)
  - [19. Further References](#19-further-references)

---

## 1. Big Picture

The **Cortex-M0** is a small, low-power 32-bit microcontroller core implementing the
**ARMv6-M** architecture profile. It is ARM's entry-level M-profile core, designed to
displace 8-bit and 16-bit MCUs while keeping the 32-bit programmer's model. Key
characteristics:

- **32-bit** data path, 32-bit addresses → 4 GB linear address space.
- **Von Neumann**: a **single** internal bus (one AHB-Lite master) for instructions
  and data — simpler and smaller than the Harvard-style M3/M4.
- **Thumb only**, executing the **ARMv6-M Thumb subset**: mostly 16-bit instructions
  plus a handful of 32-bit ones (`BL`, `DSB`, `DMB`, `ISB`, `MRS`, `MSR`). No full
  Thumb-2.
- **3-stage pipeline** (fetch, decode, execute).
- Deterministic, low-latency **interrupt handling** via a **cut-down NVIC**.
- **Very small silicon footprint** (as low as ~12k gates) and low power — its main
  selling point.
- **No FPU, no MPU, no DSP, no bit-banding, no cache.**

Think of the M0 as "the smallest thing that can still run C comfortably with a modern
interrupt controller."

```
          +-------------------------------------------+
          |               Cortex-M0 core              |
          |  +-----------+   +---------+               |
          |  | 3-stage   |   |  NVIC   |  (32 IRQ,     |
          |  | pipeline  |   | (IRQ)   |   4 prio lvls)|
          |  +-----------+   +---------+               |
          |  +-----------+   +--------------+          |
          |  | Register  |   |  Debug (opt) |          |
          |  | bank+PSR  |   |  4 BP / 2 WP |          |
          |  +-----------+   +--------------+          |
          +---------------------|---------------------+
                          single AHB-Lite bus
                       (instructions + data + PPB)
```

---

## 2. ARMv6-M and the Thumb Subset

- **ARMv6-M** is the smallest "Microcontroller" profile. It is **binary
  upward-compatible** with ARMv7-M (M3/M4): code compiled for M0 runs on an M3/M4,
  but not vice-versa.
- The instruction set is the **Thumb subset defined by ARMv6-M**: roughly 56
  instructions. It is almost entirely 16-bit encodings (excellent code density),
  with only these 32-bit instructions: `BL`, `DMB`, `DSB`, `ISB`, `MRS`, `MSR`.
- The processor is **always in Thumb state** (the **T bit** is always 1). As with all
  Cortex-M, function pointers and vector entries carry **LSB = 1** to mark Thumb.
- Notable *missing* instructions vs. M3/M4: **no hardware divide** (`SDIV`/`UDIV`),
  **no `CBZ`/`CBNZ`**, **no `IT` blocks**, **no bitfield ops** (`BFI`, `UBFX`),
  **no `LDREX`/`STREX`** exclusives, limited conditional execution (only via branches).
- **Multiply** (`MUL`) is present; vendors choose a **fast (1-cycle)** or **small
  (32-cycle)** multiplier at synthesis time.

---

## 3. Pipeline and Bus Interface

**3-stage pipeline:** Fetch → Decode → Execute, the same depth as the M3/M4, so
branch penalties are similar. There is no branch prediction beyond simple speculative
fetch.

**Single bus interface (AHB-Lite):** Unlike the M3/M4's split I-Code/D-Code/System
buses, the M0 has **one** memory interface. Instruction fetches and data accesses
share it, which keeps the core tiny at the cost of some throughput. Internal core
peripherals (NVIC, SCB, SysTick) live on the **PPB** accessed through the same core.

---

## 4. Memory Model

The 4 GB space uses the standard M-profile fixed region layout. Vendors place their
Flash/RAM/peripherals inside these regions:

| Address range              | Region       | Typical use / attributes                  |
|----------------------------|--------------|-------------------------------------------|
| 0x0000_0000–0x1FFF_FFFF    | Code         | Flash / ROM; executable                   |
| 0x2000_0000–0x3FFF_FFFF    | SRAM         | On-chip RAM                               |
| 0x4000_0000–0x5FFF_FFFF    | Peripheral   | Vendor peripherals                        |
| 0x6000_0000–0x9FFF_FFFF    | External RAM | Off-chip memory                           |
| 0xA000_0000–0xDFFF_FFFF    | External dev | Off-chip peripherals                      |
| 0xE000_0000–0xE00F_FFFF    | PPB          | System Control Space (NVIC, SCB, SysTick) |
| 0xE010_0000–0xFFFF_FFFF    | Vendor       | Vendor-specific                           |

**Differences from ARMv7-M:**
- **No bit-band regions** — the SRAM/peripheral bit-band aliases of the M3/M4 do not
  exist. Set/clear bits with normal read-modify-write (not interrupt-safe unless you
  mask interrupts).
- **Memory ordering** is simpler; the architecture defines Normal, Device, and
  Strongly-ordered types, but the M0 has no cache and one bus master, so ordering
  concerns are minimal (still relevant with DMA/other masters).

**Endianness:** configurable little- or big-endian at design time; nearly all devices
are **little-endian**.

**Unaligned access:** ARMv6-M **does not support unaligned accesses** to normal
memory — an unaligned load/store faults (HardFault). Data must be naturally aligned.
(This is a real difference from the M3/M4, which allow unaligned access.)

---

## 5. Core Registers

The core has 16 architecturally visible 32-bit registers, **R0–R15**:

| Register | Name  | Role                                                            |
|----------|-------|-----------------------------------------------------------------|
| R0–R7    | Low   | General purpose; reachable by nearly all 16-bit Thumb instructions |
| R8–R12   | High  | General purpose; only a few instructions can touch these        |
| R13      | SP    | **Stack Pointer** (banked: MSP or PSP — see §8)                 |
| R14      | LR    | **Link Register** — return address on `BL` and exceptions       |
| R15      | PC    | **Program Counter** — reads as current instr + 4 (pipeline)     |

Because most 16-bit encodings only reach **R0–R7**, the M0 leans heavily on the low
registers; the compiler works harder to keep hot values there.

**Calling convention (AAPCS)** is identical to the rest of the Cortex-M family:
- **R0–R3**: argument / scratch / return values (caller-saved).
- **R4–R11**: callee-saved.
- **R12 (IP)**: scratch.
- **SP** 8-byte aligned at public interfaces; return value in R0 (R1 for 64-bit).

There are **no floating-point registers** (no FPU).

---

## 6. Special Registers

Accessed with **MRS** (read) and **MSR** (write) — two of the few 32-bit instructions.

### 6.1 xPSR — Program Status Register

A composite of three views:

- **APSR** (Application PSR): condition flags **N, Z, C, V** only. There is **no Q
  flag** and **no GE bits** (those belong to the DSP-capable M4).
- **IPSR** (Interrupt PSR): the currently executing exception number (0 = thread mode).
- **EPSR** (Execution PSR): holds the **T bit** (always 1). ARMv6-M has **no IT/ICI
  bits** (no If-Then blocks, no interruptible-continuable LDM/STM in the M3/M4 sense).

```
 31 30 29 28        24                    8..0
 N  Z  C  V         T (in EPSR)           Exception number (IPSR)
```

### 6.2 Interrupt Masking (PRIMASK only)

| Register  | Width | Effect                                                         |
|-----------|-------|----------------------------------------------------------------|
| PRIMASK   | 1 bit | =1 blocks **all** interrupts except NMI and HardFault         |

**That is the only masking register.** ARMv6-M has **no BASEPRI** and **no FAULTMASK**
(those are ARMv7-M features). Critical sections are all-or-nothing: `CPSID i` disables,
`CPSIE i` enables. You cannot mask "only priorities below N" the way you can on M3/M4.

### 6.3 CONTROL Register

| Bit | Name  | Meaning                                                    |
|-----|-------|------------------------------------------------------------|
| 0   | nPRIV | 0 = privileged Thread mode, 1 = unprivileged Thread mode   |
| 1   | SPSEL | 0 = use MSP, 1 = use PSP in Thread mode                    |

There is **no FPCA bit** (bit 2) because there is no FPU. After changing CONTROL
(especially SPSEL), issue an **ISB**.

---

## 7. Operating Modes and Privilege

Like all Cortex-M, the M0 has **two modes** and **two privilege levels**:

| | **Thread mode** | **Handler mode** |
|---|---|---|
| Entered by | Reset / exception return | Taking any exception/interrupt |
| Privilege | Privileged **or** unprivileged (CONTROL.nPRIV) | Always privileged |
| Stack | MSP or PSP (CONTROL.SPSEL) | Always MSP |

Note: while the privilege *machinery* exists, the M0 has **no MPU**, so unprivileged
mode cannot actually be sandboxed in memory. Privilege separation on a plain M0 is of
limited practical value — it becomes meaningful on the M0+ (which can have an MPU).

---

## 8. The Two Stacks

R13 (SP) is **banked** into two physical stack pointers, exactly as on the larger
cores:

- **MSP (Main Stack Pointer)** — used after reset, by all exception handlers, and by
  privileged thread code by default. Initial value = **first word of the vector
  table** at reset.
- **PSP (Process Stack Pointer)** — typically used by RTOS task code.

Thread mode's choice is selected by **CONTROL.SPSEL**; handler mode always uses MSP.
This is what lets even a tiny M0-based RTOS keep kernel and task stacks separate.

---

## 9. Exceptions and Interrupts (NVIC)

The M0 includes a **cut-down NVIC**. It keeps the family's best features (vectored,
nested, tail-chaining) but with tighter limits:

- **Up to 32 external interrupts** (IRQ0–IRQ31) — vs. up to 240 on M3/M4.
- **4 priority levels** only: the priority field implements just the **top 2 bits**
  (values 0, 0x40, 0x80, 0xC0). Contrast with up to 8 bits (256 levels) on M3/M4.
- **Vectored**: hardware jumps straight to the handler address.
- **Nested**: higher priority preempts lower.
- **Tail-chaining** and **late-arrival** optimizations are retained.
- **No priority *grouping*** into preempt/subpriority sub-fields (ARMv6-M has a single
  priority field only).

### System exceptions

| # | Exception    | Priority     | Notes                                   |
|---|--------------|--------------|-----------------------------------------|
| 1 | Reset        | -3 (highest) | Boot                                    |
| 2 | NMI          | -2           | Non-maskable                            |
| 3 | HardFault    | -1           | **The only fault handler** on ARMv6-M   |
| 11| SVCall (SVC) | configurable | Supervisor call (`SVC #n`)              |
| 14| PendSV       | configurable | Deferred context switch (RTOS)          |
| 15| SysTick      | configurable | System timer tick (optional on M0)      |
| 16+| IRQ0..IRQ31 | configurable | Vendor peripheral interrupts            |

**Crucial difference:** there are **no configurable fault handlers** (no MemManage,
BusFault, or UsageFault). *Every* fault — bad memory access, undefined instruction,
unaligned access, divide-by-zero attempt — escalates to the single **HardFault**.
Fault diagnosis is therefore coarser than on the M3/M4 (no CFSR/MMFAR/BFAR breakdown).

**Key idioms** (same as the family): **SVC** for system calls, **PendSV** for the
deferred RTOS context switch, **SysTick** for the OS tick.

---

## 10. Exception Entry and EXC_RETURN

On exception entry the hardware **stacks 8 words** automatically — identical layout to
the rest of the family:

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

There is **no extended (FP) stack frame** — no FPU means no S-registers to stack. So
EXC_RETURN has only the three "no FP frame" forms:

| EXC_RETURN | Return to | Stack used |
|------------|-----------|------------|
| 0xFFFFFFF1 | Handler   | MSP        |
| 0xFFFFFFF9 | Thread    | MSP        |
| 0xFFFFFFFD | Thread    | PSP        |

Because R0–R3, R12, LR, PC, xPSR are saved automatically, a C interrupt handler needs
no special prologue. PendSV swaps PSP and returns with `0xFFFFFFFD` to switch tasks.

---

## 11. Reset and Vector Table

At reset the core:

1. Reads **word 0** of the vector table (at **0x0000_0000** — fixed on the plain M0)
   → **initial MSP**.
2. Reads **word 1** → **reset vector** (`Reset_Handler`, LSB=1).
3. Begins in **privileged Thread mode on MSP**.

Vector table layout:

```
Offset  Content
0x00    Initial MSP value
0x04    Reset handler
0x08    NMI handler
0x0C    HardFault handler
0x10..0x28  reserved (no MemManage/BusFault/UsageFault)
0x2C    SVCall handler
0x38    PendSV handler
0x3C    SysTick handler
0x40+   IRQ0, IRQ1, ...
```

**No VTOR on the plain Cortex-M0:** the vector table is fixed at address 0. You cannot
relocate it at runtime (a bootloader must use vendor tricks like remapping Flash/RAM to
address 0). *This is one of the headline features the M0+ adds.*

---

## 12. System Control Space

The **PPB** at 0xE000_0000 hosts the core's own peripherals. On the M0 these are a
subset of the M3/M4 set:

| Base        | Block    | Contents                                                |
|-------------|----------|---------------------------------------------------------|
| 0xE000E010  | SysTick  | CTRL, LOAD, VAL, CALIB (present if SysTick fitted)      |
| 0xE000E100  | NVIC     | ISER/ICER (enable), ISPR/ICPR (pending), IPR (priority) |
| 0xE000ED00  | SCB      | CPUID, ICSR, AIRCR, SCR, CCR, SHPR2/3                    |

**SCB (System Control Block)** on ARMv6-M is smaller:
- **CPUID** — identifies the core.
- **ICSR** — pending/active status; set PendSV/SysTick pending here.
- **AIRCR** — endianness, **system reset request** (`SYSRESETREQ`). **No priority
  grouping field** here (unlike M3/M4).
- **SCR** — sleep control.
- **CCR** — a couple of configuration bits (stack alignment, unaligned-trap is fixed
  on: unaligned always faults).
- **SHPR2/SHPR3** — priorities of SVC, PendSV, SysTick.
- **There are no CFSR/HFSR/MMFAR/BFAR fault status registers** — only HardFault exists.

---

## 13. Instruction Set Highlights

The ARMv6-M Thumb subset, by category:

- **Data processing**: `MOV, MVN, ADD, ADC, SUB, SBC, RSB, MUL, AND, ORR, EOR, BIC,
  CMP, CMN, TST`. **No hardware divide.**
- **Shifts**: `LSL, LSR, ASR, ROR` (as separate instructions; not freely folded into
  others the way Thumb-2 allows).
- **Load/store**: `LDR, STR` (+B/H/SB/SH), `LDM, STM`, `PUSH, POP`. **No `LDRD/STRD`
  doubleword, no exclusives (`LDREX/STREX`).**
- **Branch**: `B` (conditional/unconditional), `BL`, `BX`, `BLX`. **No `CBZ/CBNZ`, no
  `TBB/TBH`.**
- **Extend/reverse**: `SXTB, SXTH, UXTB, UXTH, REV, REV16, REVSH`.
- **System**: `MRS, MSR, CPS, SVC, BKPT, NOP, WFI, WFE, SEV, DSB, DMB, ISB, YIELD`.
- **No** `IT` blocks, bitfield instructions, saturating/SIMD, or floating point.

The practical upshot: the compiler emits more instructions for things the M3/M4 do in
one (divide, wide multiply-accumulate, conditional sequences), so cycle-for-cycle the
M0 is slower on those — but its code is dense and its silicon is tiny.

---

## 14. Memory Barriers

The three barrier instructions exist on ARMv6-M and are among its few 32-bit encodings:

| Instr | Meaning                                                                   |
|-------|---------------------------------------------------------------------------|
| DMB   | **Data Memory Barrier** — orders memory accesses                           |
| DSB   | **Data Synchronization Barrier** — waits until prior accesses complete     |
| ISB   | **Instruction Synchronization Barrier** — flushes pipeline, refetch        |

**When you need them:** after writing CONTROL (SPSEL/privilege), after changing NVIC
priorities or pending state that must take effect immediately, and around shared memory
with DMA or other bus masters. On a single-master M0 with no cache, everyday data
ordering rarely needs a `DMB`.

---

## 15. Low-Power and Sleep

Low power is the M0's reason for existing. Architectural support:

- **WFI** (Wait For Interrupt) — sleep until an interrupt.
- **WFE** (Wait For Event) — sleep until an event.
- **Sleep-on-exit** (SCR bit) — automatically re-sleep when the last ISR returns; ideal
  for pure interrupt-driven designs.
- **SLEEPDEEP** (SCR bit) — request a deeper sleep mode; the vendor power controller
  decides what that gates.
- **SEVONPEND** — pending interrupts create wake-up events even when masked.

Registers: **SCB->SCR**. Actual power domains, clock gating, and wake sources are
vendor-specific.

---

## 16. Debug Architecture

The M0 has a **minimal, optional** debug unit (much smaller than M3/M4 CoreSight):

| Feature            | M0 support                                                 |
|--------------------|------------------------------------------------------------|
| Debug access       | **SWD** or **JTAG**                                        |
| Hardware breakpoints | up to **4** (BPU — a cut-down Flash Patch/Breakpoint)    |
| Watchpoints        | up to **2** (a cut-down DWT)                              |
| Instruction trace  | **None** on the plain M0 (no ETM; no MTB — MTB is an M0+ addition) |
| `printf` trace     | **No ITM/SWO** on the plain M0                             |
| Cycle counter      | Limited/absent (no full DWT CYCCNT like M3/M4)            |

You can halt, step, set a few breakpoints/watchpoints, and read/write memory — enough
for basic bring-up, but without the rich trace of the bigger cores.

---

## 17. What the Cortex-M0 Does *Not* Have

A quick reference of features present on M3/M4 but **absent** on the M0 (all because it
is ARMv6-M, not ARMv7-M):

- No **FPU** (all `float`/`double` is software).
- No **DSP/SIMD** and no **saturating arithmetic** (no Q flag).
- No **MPU** (added optionally on the M0+).
- No **hardware divide** (`SDIV`/`UDIV`).
- No **bit-banding**.
- No **BASEPRI** / **FAULTMASK** — only **PRIMASK**.
- No **configurable faults** — only **HardFault** (no CFSR/MMFAR/BFAR).
- No **VTOR** — vector table fixed at 0 (added optionally on the M0+).
- No **unaligned access** — must be naturally aligned.
- No **`IT` blocks**, `CBZ/CBNZ`, bitfield ops, or exclusives (`LDREX/STREX`).
- No **cache**, no **TCM**.
- Only **4 interrupt priority levels** and up to **32 IRQs**.

---

## 18. Bring-Up Mental Model

A useful order to think about when a Cortex-M0 boots:

```
1. Power/clock stable  ->  core reads MSP (vector[0]) and PC (vector[1]) from addr 0
2. Reset_Handler runs  ->  copy .data to RAM, zero .bss, set up clocks
3. set interrupt priorities (NVIC IPR, 4 levels) and enable IRQs (NVIC ISER)
4. (optional) drop privilege / switch to PSP for an RTOS (write CONTROL; ISB)
5. call main() -> scheduler / superloop
```

Note there is **no** FPU-enable, MPU-config, or VTOR-relocate step — those simply do
not exist here.

**Debugging a fault on the M0** (coarser than on M3/M4):
1. Break in **HardFault_Handler** (the only one).
2. Recover the **stacked PC** from the exception frame (MSP or PSP per EXC_RETURN bit 2)
   to find the offending instruction.
3. Inspect the instruction and operands manually — there is **no CFSR** to tell you the
   fault class, so common causes to check are: unaligned access, null/invalid pointer,
   bad function pointer (LSB not set), or stack overflow.

---

## 19. Further References

**Primary ARM documentation (authoritative, vendor-agnostic):**
- *ARM®v6-M Architecture Reference Manual* (ARM DDI 0419) — the definitive spec for the
  instruction subset, exception model, and system registers.
- *Cortex®-M0 Technical Reference Manual* (ARM DDI 0432) — the core implementation
  details (pipeline, bus, timing).
- *Cortex-M0 Devices Generic User Guide* (ARM DUI 0497) — the approachable
  programmer's-model document; covers NVIC, SCB, SysTick register-by-register.

**Books:**
- Joseph Yiu, *The Definitive Guide to ARM® Cortex®-M0 and Cortex-M0+ Processors* — the
  single best book on these entry-level cores; written by an ARM engineer.

**Software / hands-on:**
- **CMSIS** — ARM's standard headers (`core_cm0.h`) map the core registers described
  here; read it alongside this guide to connect concepts to code.
- **FreeRTOS Cortex-M0 port** — study the PendSV context switch; note how the port
  works around the missing BASEPRI (it must use PRIMASK for critical sections, unlike
  the M3/M4 port).

**Practical exercises to cement the concepts:**
1. Write a startup file + linker script by hand (vector table at 0, MSP init, .data/.bss).
2. Trigger a HardFault deliberately (unaligned access) and recover the stacked PC.
3. Implement a minimal PendSV context switch between two tasks using PSP.
4. Compare code size/cycles of an integer divide on the M0 (software) vs. an M3 (`SDIV`).
5. Measure the current draw of `WFI` sleep vs. a busy loop.

---

*This guide describes the ARM Cortex-M0 core as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
