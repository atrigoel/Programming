# ARM Cortex-M0 / M0+ / M3 / M4 / M7 — Differences at a Glance

> A fast, vendor-agnostic comparison of the mainstream ARM Cortex-M cores. Read this
> after (or alongside) the per-core guides to see **what changes as you move up the
> family** and **which core to pick for a job**. Every fact here is defined by ARM in
> the relevant Architecture Reference Manuals — peripherals are always vendor-specific.

Per-core guides:
[M0](cortex-m0-architecture-guide.md) ·
[M0+](cortex-m0plus-architecture-guide.md) ·
[M3](cortex-m3-architecture-guide.md) ·
[M4](cortex-m4-architecture-guide.md) ·
[M7](cortex-m7-architecture-guide.md)

---

## Table of Contents

- [1. The One-Paragraph Summary](#1-the-one-paragraph-summary)
- [2. The Two Architecture Families: ARMv6-M vs. ARMv7-M](#2-the-two-architecture-families-armv6-m-vs-armv7-m)
- [3. Master Comparison Table](#3-master-comparison-table)
- [4. Instruction Set Differences](#4-instruction-set-differences)
- [5. Interrupts, NVIC, and Priorities](#5-interrupts-nvic-and-priorities)
- [6. Fault Handling Differences](#6-fault-handling-differences)
- [7. Math: Divide, DSP, and Floating Point](#7-math-divide-dsp-and-floating-point)
- [8. Memory System: Buses, Cache, TCM, Bit-Banding](#8-memory-system-buses-cache-tcm-bit-banding)
- [9. Memory Protection (MPU)](#9-memory-protection-mpu)
- [10. Debug and Trace](#10-debug-and-trace)
- [11. Performance and Power](#11-performance-and-power)
- [12. Software Portability Across the Family](#12-software-portability-across-the-family)
- [13. How to Choose a Core](#13-how-to-choose-a-core)
- [14. Migration Gotchas](#14-migration-gotchas)
- [15. Further References](#15-further-references)

---

## 1. The One-Paragraph Summary

All five cores share the **same fundamental M-profile programmer's model**: Thumb-only
execution, R0–R15 with banked MSP/PSP, Thread/Handler modes, a vectored NVIC, an 8-word
exception stack frame, and the SVC/PendSV/SysTick RTOS idioms. They split into **two
architecture generations**:

- **ARMv6-M** — the small, low-power cores: **M0** and **M0+**. Reduced instruction set,
  4 interrupt priority levels, only PRIMASK masking, a single HardFault, no divide, no
  bit-banding, no FPU/DSP.
- **ARMv7-M** — the mainstream/performance cores: **M3**, **M4**, **M7**. Full Thumb-2
  (hardware divide, IT blocks, bitfields, exclusives), up to 256 priority levels,
  BASEPRI/FAULTMASK, configurable faults with status registers.

Within ARMv7-M: **M4 = M3 + DSP + optional single-precision FPU**, and **M7 = M4's ISA
scaled up** with a dual-issue pipeline, caches, TCM, a 64-bit bus, and optional
double-precision FPU.

---

## 2. The Two Architecture Families: ARMv6-M vs. ARMv7-M

```
                     ARM Cortex-M mainstream line

   ARMv6-M (small/low-power)          ARMv7-M / ARMv7E-M (mainstream/perf)
   ┌───────────┬───────────┐          ┌──────────┬──────────┬───────────┐
   │   M0      │   M0+     │          │   M3     │   M4     │    M7     │
   ├───────────┼───────────┤          ├──────────┼──────────┼───────────┤
   │ 3-stage   │ 2-stage   │          │ 3-stage  │ 3-stage  │ 6-stage   │
   │ pipe      │ pipe      │          │ pipe     │ pipe     │ dual-issue│
   │ no MPU    │ opt MPU   │          │ opt MPU  │ opt MPU  │ opt MPU16 │
   │ table @0  │ opt VTOR  │          │ VTOR     │ VTOR     │ VTOR      │
   │           │ +MTB,fastIO│         │ +bitband │ +DSP     │ +cache,TCM│
   │           │           │          │          │ +FPU(SP) │ +FPU SP/DP│
   └───────────┴───────────┘          └──────────┴──────────┴───────────┘
        Thumb subset (~56)                    full Thumb-2
   PRIMASK only, 4 prio, HardFault    PRIMASK/BASEPRI/FAULTMASK, ≤256 prio,
   only, no divide, no bit-band       configurable faults, HW divide, bit-band*
                                      (*bit-band on M3 only; dropped on M4/M7)
```

The **single most important dividing line** in this family is **ARMv6-M vs. ARMv7-M**.
Almost every capability difference (divide, BASEPRI, configurable faults, bit-banding,
IT blocks) follows from which side of that line a core sits on. The M4/M7 additions
(DSP, FPU, cache, TCM) are *further* differentiators layered on top of ARMv7-M.

---

## 3. Master Comparison Table

| Feature | **M0** | **M0+** | **M3** | **M4** | **M7** |
|---|---|---|---|---|---|
| Architecture | ARMv6-M | ARMv6-M | ARMv7-M | ARMv7E-M | ARMv7E-M |
| Pipeline | 3-stage | 2-stage | 3-stage | 3-stage | **6-stage dual-issue** |
| Branch prediction | no | no | no | no | **yes** |
| Perf (DMIPS/MHz) | ~0.87 | ~0.93 | ~1.25 | ~1.25 | **~2.14** |
| Instruction set | Thumb subset | Thumb subset | full Thumb-2 | Thumb-2 + DSP | Thumb-2 + DSP |
| Hardware divide | no | no | **yes** | yes | yes |
| DSP / SIMD | no | no | no | **yes** | yes |
| Saturating (Q flag) | no | no | no | **yes** | yes |
| FPU | no | no | no | **opt. SP** | **opt. SP or DP** |
| Bus | single AHB-Lite | single AHB-Lite | I/D/System AHB | I/D/System AHB | **64-bit AXI + AHBP/S** |
| Cache | no | no | no | no | **opt. I + D** |
| TCM | no | no | no | no | **ITCM + DTCM** |
| Bit-banding | no | no | **yes** | no† | no† |
| Unaligned access | no | no | yes | yes | yes |
| Max external IRQs | 32 | 32 | 240 | 240 | 240 |
| Priority levels | 4 | 4 | up to 256 | up to 256 | up to 256 |
| Priority grouping | no | no | **yes** | yes | yes |
| Interrupt masking | PRIMASK | PRIMASK | +BASEPRI, FAULTMASK | +BASEPRI, FAULTMASK | +BASEPRI, FAULTMASK |
| Configurable faults | no (HardFault only) | no | **MemManage/Bus/Usage** | yes | yes |
| Fault status regs (CFSR…) | no | no | **yes** | yes | yes |
| MPU (optional) | no | **opt. 8 rgn** | opt. 8 rgn | opt. 8 rgn | **opt. 16 rgn** |
| MPU min region | — | 256 B | 32 B | 32 B | 32 B |
| VTOR (relocate vectors) | no | **optional** | yes | yes | yes |
| Instruction trace | no | **opt. MTB** | opt. ETM | opt. ETM | opt. ETM |
| ITM/SWO printf trace | no | no | yes | yes | yes |
| Cycle counter (DWT) | no | no | yes | yes | yes |

† Bit-banding is defined by ARMv7-M and implemented on the **M3**, but ARM **dropped it
on the M4 and M7** — do not rely on it there; use `LDREX/STREX` or interrupt masking.

---

## 4. Instruction Set Differences

Everything below the line is what the **ARMv7-M cores (M3/M4/M7) add over the ARMv6-M
cores (M0/M0+)**:

**Present on all five (ARMv6-M baseline):**
`MOV/ADD/SUB/MUL`, logic ops, shifts, `LDR/STR`, `LDM/STM`, `PUSH/POP`, `B/BL/BX/BLX`,
`SXTB/UXTB/REV`, `MRS/MSR/CPS/SVC/BKPT/WFI/WFE/SEV/DSB/DMB/ISB`.

**Added by ARMv7-M (M3/M4/M7 only):**
- **Hardware divide** `SDIV`/`UDIV`.
- **`IT`** (If-Then) conditional blocks — cheap predication without branches.
- **`CBZ`/`CBNZ`** compare-and-branch-zero; **`TBB`/`TBH`** table branches (switch).
- **Bitfield**: `BFI, BFC, UBFX, SBFX`, plus `CLZ, RBIT`.
- **`LDRD`/`STRD`** doubleword; **`LDREX`/`STREX`/`CLREX`** exclusives (lock-free sync).

**Added by the DSP extension (M4/M7 only, "ARMv7E-M"):**
- **Saturating arithmetic** `QADD, QSUB, SSAT, USAT` (sets the **Q flag**).
- **SIMD** on packed 8/16-bit data `QADD8, SADD16, SMLAD, …`, plus `SEL`/GE bits.
- **Single-cycle MAC** and 32×32→64 multiply-accumulate.

**Added by the FPU (M4F single-precision / M7 single-or-double):**
- Floating-point load/store, arithmetic, compare, convert on **S/D registers** + FPSCR.

Practical effect: the same C code compiles to **fewer, faster instructions** as you move
up. A 32-bit integer divide is a library call on M0/M0+ but one `SDIV` on M3+. A
biquad filter is scalar C on M3 but uses DSP MAC/SIMD on M4/M7.

---

## 5. Interrupts, NVIC, and Priorities

| | M0 / M0+ | M3 / M4 / M7 |
|---|---|---|
| External IRQs | up to **32** | up to **240** |
| Priority levels | **4** (top 2 bits) | up to **256** (up to 8 bits; vendors use 3–4) |
| Priority grouping | **none** | **preempt + subpriority** (via AIRCR) |
| Masking | **PRIMASK only** | PRIMASK + **BASEPRI** + **FAULTMASK** |
| Vectored / nested / tail-chain | yes | yes |

The big practical gap is **BASEPRI**. On the M3/M4/M7 an RTOS or driver can mask *only*
interrupts below a chosen priority, leaving urgent ones live — the foundation of
low-latency critical sections. On the M0/M0+ you only have **all-or-nothing** PRIMASK, so
their critical sections briefly block *every* maskable interrupt. RTOS ports differ
accordingly (the Cortex-M0 FreeRTOS port uses PRIMASK; the M3+ port uses BASEPRI).

---

## 6. Fault Handling Differences

| | M0 / M0+ | M3 / M4 / M7 |
|---|---|---|
| Fault handlers | **HardFault only** | HardFault + **MemManage + BusFault + UsageFault** |
| Fault status regs | **none** | **CFSR, HFSR, MMFAR, BFAR** |
| Diagnosis | manual (inspect stacked PC) | precise (read CFSR → class, MMFAR/BFAR → address) |

On the ARMv6-M cores **every** error — bad pointer, unaligned access, undefined
instruction, MPU violation (M0+), attempted divide-by-zero trap — escalates to the single
**HardFault**, and there is no status register to tell you which. You recover the stacked
PC and reason it out. On the ARMv7-M cores you enable the specific fault, read **CFSR**
for the class and **MMFAR/BFAR** for the faulting address — dramatically faster debugging.
See the fault-debug recipes in the [M3](cortex-m3-architecture-guide.md#20-bring-up-mental-model)
and [M4](cortex-m4-architecture-guide.md#21-bring-up-mental-model) guides.

---

## 7. Math: Divide, DSP, and Floating Point

```
 capability        M0/M0+    M3       M4         M7
 ─────────────────────────────────────────────────────────
 integer divide    software  SDIV     SDIV       SDIV
 32x32 MAC 1-cyc   no        no       yes (DSP)  yes (DSP)
 SIMD 8/16-bit     no        no       yes        yes
 saturating (Q)    no        no       yes        yes
 float (single)    software  software software*  hardware (FPv5-SP)
 double            software  software software   hardware† (FPv5-DP)
```
\* On the M4 **without** the FPU option (`float` is software); the **M4F** has hardware
single-precision. † Only on M7 parts fitted with the **double-precision** FPv5-DP option.

Rules of thumb:
- Need **fast integer math / control code** but no floats → **M3** is plenty.
- Need **`float` DSP** (audio, motor control, sensor fusion) → **M4F**.
- Need **heavy DSP throughput or hardware `double`** → **M7**.
- **`double` in C on an M0–M4 is always software emulation** — slow; prefer `float` (and
  mind the `f` suffix on literals) unless you're on an FPv5-DP M7.

---

## 8. Memory System: Buses, Cache, TCM, Bit-Banding

| | M0 / M0+ | M3 / M4 | M7 |
|---|---|---|---|
| Internal bus | single AHB-Lite | split I-Code / D-Code / System | **64-bit AXI + AHBP + AHBS** |
| Cache | none | none | **optional I-cache + D-cache** |
| TCM | none | none | **ITCM + DTCM** (deterministic) |
| Bit-banding | none | M3 yes / M4 no | none |

The M0/M0+ share one bus (smallest, simplest). The M3/M4 split instruction and data
paths so a fetch and a data load can overlap. The **M7** is a different league: a 64-bit
AXI master feeding **caches** for throughput, plus **TCM** for determinism, plus a slave
port so DMA can reach the TCMs. The M7's caches bring the **cache-coherency/maintenance**
responsibility that none of the smaller cores have (see §14).

**Bit-banding** is a quirk: architecturally an ARMv7-M feature, it is implemented on the
**M3** but **dropped on the M4 and M7**. Never assume atomic bit set/clear via bit-band
on M4/M7 — use `LDREX/STREX` or mask interrupts.

---

## 9. Memory Protection (MPU)

| | M0 | M0+ | M3 | M4 | M7 |
|---|---|---|---|---|---|
| MPU available | no | **optional** | optional | optional | optional |
| Regions | — | 8 | 8 | 8 | **16** |
| Min region size | — | 256 B | 32 B | 32 B | 32 B |
| Violation → | — | **HardFault** | **MemManage** | MemManage | MemManage |

All MPUs are protection-only (no address translation — not an MMU). Note two family
quirks: the **M0+** MPU has a coarser 256-byte minimum and its violations escalate to
**HardFault** (no MemManage on ARMv6-M), whereas the M3/M4/M7 have a dedicated
**MemManage** fault with status bits. The **M7** doubles the region count to 16 and, in
practice, its MPU doubles as the mechanism for marking regions **non-cacheable** (vital
for DMA buffers).

---

## 10. Debug and Trace

| Capability | M0 | M0+ | M3 / M4 / M7 |
|---|---|---|---|
| SWD / JTAG | yes | yes | yes |
| HW breakpoints / watchpoints | few | few | more |
| DWT cycle counter (CYCCNT) | no | no | **yes** |
| ITM / SWO `printf` trace | no | no | **yes** |
| Instruction trace | none | **MTB** (to SRAM) | **ETM** (optional) |

Debug richness scales with the core. The M0 has bare-minimum halt/step + a couple of
breakpoints. The M0+ adds the **MTB** for cheap branch-history trace. The M3/M4/M7 have
full CoreSight: a free-running **cycle counter**, **ITM/SWO** for low-overhead `printf`,
and optional **ETM** full instruction trace (the M7 often needs wider trace ports for its
speed).

---

## 11. Performance and Power

```
   performance  ─────────────────────────────────────────▶  higher
   M0  <  M0+  <  M3  ≈  M4  <<  M7
   ~0.87  ~0.93   1.25  1.25    2.14   DMIPS/MHz (before clock differences)

   power efficiency (low-power focus) ◀───────────────────  M0+ best
   M0+  <  M0  <  M3  <  M4  <  M7
   (M0+ is the ultra-low-power champion; M7 is the power-hungry performer)
```

- **M0+** is the efficiency king (2-stage pipe + race-to-sleep) — sensor nodes, metering,
  wearables.
- **M3/M4** sit in the balanced middle — general-purpose MCUs.
- **M7** trades power for raw speed and memory bandwidth — high-end control, audio/DSP,
  graphics-capable MCUs. It runs at far higher clocks than the others as well, so real
  throughput gaps exceed the per-MHz numbers.

---

## 12. Software Portability Across the Family

Binary and source compatibility flow **upward, not downward**:

- **ARMv6-M binary (M0/M0+) runs on any ARMv7-M core (M3/M4/M7)** — the instruction set
  is a strict subset. The reverse is **not** true.
- **M4 code runs on M7 unchanged** (same ARMv7E-M ISA) — the M7's gains are
  microarchitectural, not new instructions (except optional hardware `double`).
- **CMSIS** abstracts the differences: `core_cm0.h`, `core_cm0plus.h`, `core_cm3.h`,
  `core_cm4.h`, `core_cm7.h` present a consistent API; `#ifdef` guards expose FPU/DSP/
  cache helpers only where they exist.
- **RTOS ports differ by architecture family**: the Cortex-M0 port (PRIMASK critical
  sections, no bit-band) vs. the M3/M4/M7 port (BASEPRI critical sections, optional
  lazy-FP stacking). Picking the right port matters when you change cores.

Writing portable firmware means: avoid assuming BASEPRI/bit-banding/hardware-divide
exist, keep DSP/FP behind CMSIS or `#ifdef`, and (for the M7) never assume DMA buffers
are coherent without cache maintenance.

---

## 13. How to Choose a Core

| If you need… | Pick |
|---|---|
| Lowest cost / smallest die, 8/16-bit replacement | **M0** |
| Lowest power, want MPU + VTOR, still tiny | **M0+** |
| General 32-bit MCU: fast integer/control, full NVIC, good debug, no floats | **M3** |
| Everything the M3 has **plus** `float` DSP (audio, motor, sensor fusion) | **M4 / M4F** |
| Maximum performance, caches/TCM, high clocks, or hardware `double` | **M7** |

Quick heuristics:
- **Batteries + simple logic?** → M0+.
- **Doing real-time integer control, no math library needed?** → M3.
- **Filtering / FFT / PID with floats?** → M4F.
- **Hundreds of MHz, big buffers, DSP-heavy, or graphics?** → M7.
- Unsure between M4 and M7? If you don't need caches/TCM/hardware-`double`, the **M4F is
  simpler to get right** (no cache-coherency pitfalls).

---

## 14. Migration Gotchas

Moving code between cores surfaces predictable issues:

**Going down (M3/M4 → M0/M0+):**
- No hardware divide → integer divide becomes a slow library call. Audit hot loops.
- No BASEPRI → critical sections must use PRIMASK (disable all IRQs); revisit latency.
- No bit-banding → replace bit-band writes with masked read-modify-write.
- Only HardFault → lose CFSR diagnostics; add manual fault dumping.
- No unaligned access → packed structs / casts that were fine on M3/M4 now **fault**.
- No FPU/DSP → CMSIS-DSP falls back to slow scalar code; `float` math balloons in size.

**Going up (M4 → M7):**
- **Cache coherency** is the #1 bug source: DMA buffers in cacheable memory return stale
  data unless you **clean/invalidate** the D-cache (or place buffers in **DTCM** / a
  non-cacheable MPU region). Code that "just worked" on the M4 can break silently.
- **Alignment** of DMA buffers to 32-byte cache lines becomes important.
- **Timing determinism** changes: the dual-issue, cached pipeline is faster on average
  but less cycle-predictable — move hard-real-time code/data into **ITCM/DTCM**.
- Bit-banding you may have used on an **M3** is **gone** on the M4/M7.

**General:**
- Priority-register width varies (vendors implement 2–8 top bits) — always set priorities
  via CMSIS, not raw magic numbers.
- FPU is **off at reset** on M4F/M7 — enable CP10/CP11 in CPACR before any `float` op, or
  you take a UsageFault.

---

## 15. Further References

- Per-core guides in this repo:
  [M0](cortex-m0-architecture-guide.md),
  [M0+](cortex-m0plus-architecture-guide.md),
  [M3](cortex-m3-architecture-guide.md),
  [M4](cortex-m4-architecture-guide.md),
  [M7](cortex-m7-architecture-guide.md) — each ends with its own ARM-doc references.
- *ARM®v6-M Architecture Reference Manual* (DDI 0419) — M0/M0+.
- *ARM®v7-M Architecture Reference Manual* (DDI 0403) — M3/M4/M7 (incl. the E-M DSP
  extension).
- The *Devices Generic User Guide* for each core (DUI series) — the most approachable
  register-level docs.
- Joseph Yiu's *Definitive Guide to ARM Cortex-M0/M0+*, *…M3 and M4*, and *…M7* — the
  standard books; excellent for the family comparisons summarized here.
- **CMSIS** headers (`core_cm0.h` … `core_cm7.h`) — read side by side to see exactly
  which registers/helpers appear on which core.

---

*This guide compares the ARM Cortex-M cores as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
