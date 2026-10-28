# ARM Cortex-M0+ Architecture — A Vendor-Agnostic Guide

> Focus: the ARM **core** itself (the parts every silicon vendor shares), not any
> specific microcontroller's peripherals. Everything here is defined by ARM in the
> *ARMv6-M Architecture Reference Manual* and the *Cortex-M0+ Technical Reference Manual*.

---

## Table of Contents

- [ARM Cortex-M0+ Architecture — A Vendor-Agnostic Guide](#arm-cortex-m0-architecture--a-vendor-agnostic-guide)
  - [Table of Contents](#table-of-contents)
  - [1. Big Picture](#1-big-picture)
  - [2. How the M0+ Differs from the M0](#2-how-the-m0-differs-from-the-m0)
  - [3. ARMv6-M and the Thumb Subset](#3-armv6-m-and-the-thumb-subset)
  - [4. The 2-Stage Pipeline](#4-the-2-stage-pipeline)
  - [5. Memory Model](#5-memory-model)
  - [6. Single-Cycle I/O Port](#6-single-cycle-io-port)
  - [7. Core and Special Registers](#7-core-and-special-registers)
  - [8. Operating Modes, Privilege, and the Two Stacks](#8-operating-modes-privilege-and-the-two-stacks)
  - [9. The Optional MPU](#9-the-optional-mpu)
  - [10. Exceptions and Interrupts (NVIC)](#10-exceptions-and-interrupts-nvic)
  - [11. Exception Entry and EXC\_RETURN](#11-exception-entry-and-exc_return)
  - [12. Reset, Vector Table, and VTOR](#12-reset-vector-table-and-vtor)
  - [13. System Control Space](#13-system-control-space)
  - [14. Instruction Set Highlights](#14-instruction-set-highlights)
  - [15. Low-Power and Sleep](#15-low-power-and-sleep)
  - [16. Debug Architecture and the MTB](#16-debug-architecture-and-the-mtb)
  - [17. What the Cortex-M0+ Still Does *Not* Have](#17-what-the-cortex-m0-still-does-not-have)
  - [18. Bring-Up Mental Model](#18-bring-up-mental-model)
  - [19. Further References](#19-further-references)

---

## 1. Big Picture

The **Cortex-M0+** is an **energy-optimized evolution of the Cortex-M0**. It implements
the same **ARMv6-M** architecture and runs the *same instruction set*, so from a
programmer's model standpoint it is nearly identical to the M0. The differences are in
the **microarchitecture** (how it is built) and a few **optional features** that make it
markedly more capable and more efficient:

- **2-stage pipeline** (vs. the M0's 3-stage) → fewer wasted fetches on branches →
  **lower power** and slightly better performance per MHz.
- **Optional MPU** (8 regions) → real memory protection, absent on the plain M0.
- **Optional VTOR** → relocatable vector table (bootloaders, RAM vectors).
- **Single-cycle I/O port** (optional) → deterministic, fast GPIO.
- **Optional Micro Trace Buffer (MTB)** → low-cost instruction trace.
- Among the **most power-efficient 32-bit cores** ARM has made (often quoted around
  ~9 µA/MHz on modern silicon) — its headline selling point.

If the M0 is "smallest core that runs C," the M0+ is "smallest core that runs C **well**
and can protect memory and relocate its vectors." It has largely replaced the M0 in new
low-power designs.

```
          +-------------------------------------------+
          |              Cortex-M0+ core              |
          |  +-----------+   +---------+   +--------+  |
          |  | 2-stage   |   |  NVIC   |   |  MPU   |  |
          |  | pipeline  |   | (IRQ)   |   | (opt.) |  |
          |  +-----------+   +---------+   +--------+  |
          |  +-----------+   +---------+   +--------+  |
          |  | Register  |   |  VTOR   |   |  MTB   |  |
          |  | bank+PSR  |   | (opt.)  |   | (opt.) |  |
          |  +-----------+   +---------+   +--------+  |
          +----------|-------------------|------------+
                single AHB-Lite bus   single-cycle I/O port (opt.)
```

---

## 2. How the M0+ Differs from the M0

Because the M0 and M0+ share ARMv6-M and the same instruction subset, it is easiest to
learn the M0+ as **"M0 plus these changes."** Everything in the
[Cortex-M0 guide](cortex-m0-architecture-guide.md) applies unless listed here:

| Aspect                 | Cortex-M0            | Cortex-M0+                          |
|------------------------|----------------------|-------------------------------------|
| Pipeline               | 3-stage              | **2-stage** (lower power, fewer branch-fetch waste) |
| MPU                    | none                 | **optional 8-region MPU**           |
| VTOR (vector relocate) | none (table fixed @0)| **optional VTOR**                   |
| Single-cycle I/O port  | no                   | **optional** fast GPIO interface    |
| Instruction trace      | none                 | **optional MTB** (Micro Trace Buffer) |
| Power efficiency       | good                 | **best-in-class** for ARMv6-M       |
| Instruction set        | ARMv6-M subset       | **identical** ARMv6-M subset        |
| Interrupts / priorities| ≤32 IRQ, 4 levels    | ≤32 IRQ, 4 levels (**same**)        |

Everything else — registers, PRIMASK-only masking, single HardFault, no FPU/DSP, no
bit-banding, no hardware divide, aligned-access-only — is **the same as the M0**.

---

## 3. ARMv6-M and the Thumb Subset

Identical to the M0. The M0+ executes the same ~56-instruction **ARMv6-M Thumb subset**
(mostly 16-bit, with `BL`, `DMB`, `DSB`, `ISB`, `MRS`, `MSR` as the 32-bit encodings).
It is always in Thumb state; the same *missing* instructions apply: **no hardware
divide, no `IT`, no `CBZ/CBNZ`, no bitfield ops, no exclusives, no saturating/SIMD, no
floating point.** See [§2 of the M0 guide](cortex-m0-architecture-guide.md#2-armv6-m-and-the-thumb-subset).

---

## 4. The 2-Stage Pipeline

The one microarchitectural change with the widest effect is the **2-stage pipeline**
(Fetch → Decode+Execute) versus the M0's 3-stage design.

- **Fewer speculative fetches are thrown away** on taken branches, so less energy is
  wasted and branch-heavy code runs a touch faster per MHz.
- Combined with branch-target speculation, ARM quotes the M0+ at roughly **0.93
  DMIPS/MHz** (vs. ~0.87–0.9 for the M0) while using **less energy per instruction**.
- The programmer-visible behavior is unchanged: `PC` still reads as "current
  instruction + 4," and code written for the M0 runs identically.

This is why a vendor can drop an M0+ into an M0 socket and get **more work per joule**
without any software change.

---

## 5. Memory Model

The 4 GB fixed-region layout is **exactly the same as the M0** (and the rest of the
family) — see [§4 of the M0 guide](cortex-m0-architecture-guide.md#4-memory-model).
Same constraints apply:

- **No bit-banding** (ARMv6-M).
- **No unaligned access** — data must be naturally aligned; violations HardFault.
- **No cache, no TCM.**

The two memory-related *additions* on the M0+ are the **optional MPU** (§9) and, if the
vendor fits it, the memory-mapped **single-cycle I/O port** (§6).

---

## 6. Single-Cycle I/O Port

The M0+ optionally adds a dedicated **single-cycle I/O port** — a second, tightly
coupled memory interface intended for GPIO and other latency-critical registers.

- A load or store to an address in this port completes in **one cycle**, versus two or
  more cycles through the normal AHB-Lite system bus.
- Vendors map their **fast GPIO** registers here so bit-bang, software-driven protocols,
  and precise pin timing become **deterministic**.
- It partly compensates for the lack of bit-banding: toggling a pin is fast and
  jitter-free even though there is no bit-band alias.

Whether it exists, and which registers appear on it, is **vendor-specific** — check the
device reference manual. Architecturally it is just an extra fast-access region.

---

## 7. Core and Special Registers

**Identical to the M0.** Sixteen 32-bit registers R0–R15 (SP banked as MSP/PSP, LR, PC),
AAPCS calling convention, and the same special registers:

- **xPSR** with N/Z/C/V only (no Q, no GE, no IT/ICI).
- **PRIMASK** is the *only* interrupt mask (no BASEPRI, no FAULTMASK).
- **CONTROL** with nPRIV and SPSEL only (no FPCA — no FPU).

See [§5–6 of the M0 guide](cortex-m0-architecture-guide.md#5-core-registers) for the
full detail; nothing changes here.

---

## 8. Operating Modes, Privilege, and the Two Stacks

Modes (Thread/Handler), privilege levels, and the banked **MSP/PSP** stacks are the
same as the M0 — see [§7–8 of the M0 guide](cortex-m0-architecture-guide.md#7-operating-modes-and-privilege).

**The important practical difference:** on the M0+, privilege separation can be
**enforced** because an **MPU** may be present. Unprivileged Thread-mode tasks can be
restricted to specific memory regions, which makes the Thread/Handler + PSP/MSP
machinery genuinely useful for an RTOS or a safety-partitioned design — something the
plain M0 (no MPU) cannot do.

---

## 9. The Optional MPU

The M0+ may include the ARMv6-M **Memory Protection Unit** (it is *not* an MMU — no
address translation, no paging):

- Up to **8 regions**, each a power-of-two size ≥ 256 bytes, aligned to its size.
- Per-region **access permissions** (privileged/unprivileged × RO/RW/none),
  **execute-never (XN)**, and memory-type attributes.
- **Region priority:** higher-numbered regions win where regions overlap.
- **Sub-region disable:** each region splits into 8 sub-regions, individually enabled.
- Violations raise a **HardFault** (remember: ARMv6-M has **no separate MemManage
  fault** — the MPU violation escalates straight to HardFault, unlike the M3/M4).

Registers: **MPU_TYPE, MPU_CTRL, MPU_RNR, MPU_RBAR, MPU_RASR**. Always `DSB; ISB` after
reconfiguring.

> Note the ARMv6-M MPU is slightly more restrictive than the ARMv7-M MPU on the M3/M4:
> minimum region size is 256 bytes (vs. 32 bytes) and there is no separate fault status
> register to tell you which region tripped — you land in HardFault and inspect the
> stacked PC/address yourself.

Typical uses: sandbox RTOS tasks, mark Flash read-only/execute-only, catch stack
overflow or null-pointer writes.

---

## 10. Exceptions and Interrupts (NVIC)

The NVIC is the **same cut-down ARMv6-M unit as the M0**:

- **Up to 32 external interrupts** (IRQ0–IRQ31).
- **4 priority levels** (top 2 bits of the priority field implemented).
- **Vectored, nested, tail-chaining, late-arrival** — all retained.
- **No priority grouping** into preempt/subpriority.
- System exceptions: Reset, NMI, **HardFault (the only fault)**, SVCall, PendSV,
  SysTick, then IRQ0+. There are still **no configurable faults** (no MemManage/
  BusFault/UsageFault) even though an MPU may be present — MPU faults escalate to
  HardFault.

See [§9 of the M0 guide](cortex-m0-architecture-guide.md#9-exceptions-and-interrupts-nvic)
for the exception table and idioms (SVC, PendSV, SysTick); they are unchanged.

---

## 11. Exception Entry and EXC_RETURN

Identical to the M0: hardware stacks the same **8-word frame** (xPSR, PC, LR, R12,
R3–R0), there is **no FP frame**, and EXC_RETURN has the three non-FP forms
(`0xFFFFFFF1` handler/MSP, `0xFFFFFFF9` thread/MSP, `0xFFFFFFFD` thread/PSP). PendSV
returns with `0xFFFFFFFD` to switch tasks. See
[§10 of the M0 guide](cortex-m0-architecture-guide.md#10-exception-entry-and-exc_return).

---

## 12. Reset, Vector Table, and VTOR

Reset behavior matches the M0: read **MSP** from vector word 0 and **PC** from word 1,
then run in privileged Thread mode on MSP. The vector table layout is identical (Reset,
NMI, HardFault, then reserved slots, SVC, PendSV, SysTick, IRQs).

**The headline addition: the optional VTOR (Vector Table Offset Register).**

- On the plain M0 the vector table is **fixed at address 0** and cannot be moved.
- On the M0+, if VTOR is implemented, you can **relocate the vector table** at runtime
  by writing its base address to **SCB->VTOR**.
- This enables clean **bootloader → application** handoff (each image has its own table)
  and **RAM-based vector tables** (patch handlers at runtime, faster fetch on some
  memory systems).

If a design needs a bootloader, the M0+ with VTOR is far more convenient than the plain
M0 (which must resort to vendor-specific Flash/RAM remap-to-0 tricks).

---

## 13. System Control Space

The **PPB** at 0xE000_0000 hosts the same core peripherals as the M0 (SysTick, NVIC,
SCB), plus the **MPU** registers and **VTOR** when those options are fitted:

| Base        | Block    | Contents                                                |
|-------------|----------|---------------------------------------------------------|
| 0xE000E010  | SysTick  | CTRL, LOAD, VAL, CALIB                                   |
| 0xE000E100  | NVIC     | ISER/ICER, ISPR/ICPR, IPR                               |
| 0xE000ED00  | SCB      | CPUID, ICSR, **VTOR** (opt.), AIRCR, SCR, CCR, SHPR2/3   |
| 0xE000ED90  | MPU      | TYPE, CTRL, RNR, RBAR, RASR (opt.)                       |

There are still **no CFSR/HFSR/MMFAR/BFAR** fault status registers — only HardFault
exists on ARMv6-M.

---

## 14. Instruction Set Highlights

**Identical to the M0** — the ARMv6-M Thumb subset with the same omissions (no divide,
no `IT`, no `CBZ/CBNZ`, no bitfield, no exclusives, no saturating/SIMD, no FP). See
[§13 of the M0 guide](cortex-m0-architecture-guide.md#13-instruction-set-highlights).
The performance uplift on the M0+ comes from the **2-stage pipeline and single-cycle I/O
port**, not from any new instructions.

---

## 15. Low-Power and Sleep

Same architectural mechanisms as the M0 — **WFI, WFE, Sleep-on-exit, SLEEPDEEP,
SEVONPEND**, all via **SCB->SCR** — but the M0+ is engineered to be **even more
efficient**:

- The 2-stage pipeline wastes less energy per instruction.
- Faster wake/execute means the core can return to sleep sooner (**"race to sleep"**),
  lowering average current.
- This combination is why the M0+ dominates ultra-low-power designs (metering, wearables,
  sensor nodes). Actual sleep modes and current figures are vendor-specific.

---

## 16. Debug Architecture and the MTB

The M0+ keeps the M0's minimal debug (SWD/JTAG, a few hardware breakpoints and
watchpoints, halt/step) and **adds an optional instruction-trace mechanism the M0
lacks**:

- **MTB — Micro Trace Buffer:** a low-cost trace unit that records program flow (branch
  history) into a region of the device's own **SRAM**, which the debugger reads back.
- It gives you a **recent-execution history** ("how did I get to this HardFault?")
  without the expensive ETM/TPIU trace hardware of the M3/M4/M7.
- There is still **no ITM/SWO** `printf`-style trace and **no cycle-accurate DWT
  profiling** like the bigger cores — but the MTB is a big step up from the plain M0's
  "no trace at all."

| Feature              | M0        | M0+                                   |
|----------------------|-----------|---------------------------------------|
| Breakpoints/watchpts | few       | few (same)                            |
| Instruction trace    | none      | **optional MTB** (to SRAM)            |
| ITM/SWO printf trace | none      | none                                  |

---

## 17. What the Cortex-M0+ Still Does *Not* Have

Despite its additions, the M0+ is still an **ARMv6-M** core and inherits the same
fundamental limits as the M0:

- No **FPU**, no **DSP/SIMD**, no **saturating arithmetic** (no Q flag).
- No **hardware divide**.
- No **bit-banding**.
- No **BASEPRI/FAULTMASK** — only **PRIMASK**.
- No **configurable faults** — MPU and other faults all escalate to **HardFault** (no
  CFSR/MMFAR/BFAR).
- No **unaligned access**.
- No **`IT` blocks, `CBZ/CBNZ`, bitfield ops, or exclusives**.
- No **cache, no TCM**.
- Still only **4 priority levels** and up to **32 IRQs**.

For any of these you must step up to an **ARMv7-M** core (Cortex-M3 for divide/
bit-band/full NVIC/configurable faults, or M4/M7 for FPU + DSP).

---

## 18. Bring-Up Mental Model

Boot order for a Cortex-M0+, extending the M0 sequence with its extras:

```
1. Power/clock stable  ->  core reads MSP (vector[0]) and PC (vector[1])
2. Reset_Handler runs  ->  copy .data to RAM, zero .bss, set up clocks
3. (optional) relocate vector table via VTOR (bootloader / RAM vectors)
4. (optional) configure MPU regions; DSB; ISB
5. set interrupt priorities (NVIC IPR, 4 levels) and enable IRQs (NVIC ISER)
6. (optional) drop privilege / switch to PSP for an RTOS (write CONTROL; ISB)
7. call main() -> scheduler / superloop
```

Steps 3 and 4 are the genuinely new capabilities over the M0.

**Debugging a fault** is like the M0 (only HardFault exists, no CFSR): break in
`HardFault_Handler`, recover the stacked PC from the frame (MSP/PSP per EXC_RETURN bit
2), and reason about the cause manually — but now an **MPU** violation is a new
candidate cause to consider, and the **MTB** trace (if fitted) can show you the path
that led there.

---

## 19. Further References

**Primary ARM documentation (authoritative, vendor-agnostic):**
- *ARM®v6-M Architecture Reference Manual* (ARM DDI 0419) — shared with the M0; defines
  the instruction subset, exception model, and system registers.
- *Cortex®-M0+ Technical Reference Manual* (ARM DDI 0484) — the M0+ core implementation
  details (2-stage pipeline, single-cycle I/O port, MTB).
- *Cortex-M0+ Devices Generic User Guide* (ARM DUI 0662) — the approachable
  programmer's-model document; covers NVIC, SCB, MPU, VTOR register-by-register.

**Books:**
- Joseph Yiu, *The Definitive Guide to ARM® Cortex®-M0 and Cortex-M0+ Processors* —
  covers both cores together and is explicit about what the M0+ adds.

**Software / hands-on:**
- **CMSIS** — `core_cm0plus.h` maps the core registers (including VTOR and the MPU) used
  here.
- **FreeRTOS Cortex-M0 port** — the same port serves the M0+; add MPU-aware task
  isolation if the device has an MPU.

**Practical exercises to cement the concepts:**
1. Relocate the vector table with **VTOR** and jump from a bootloader to an application.
2. Configure an **MPU** region to make Flash execute-only and catch a write to it.
3. Toggle a pin through the **single-cycle I/O port** and compare timing/jitter with a
   normal system-bus GPIO write.
4. Enable the **MTB** and capture the branch history leading into a deliberate HardFault.
5. Compare average current of the same workload on an M0 vs. M0+ to see the pipeline/
   race-to-sleep benefit.

---

*This guide describes the ARM Cortex-M0+ core as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
