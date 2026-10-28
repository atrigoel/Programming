# ARM Cortex-M7 Architecture — A Vendor-Agnostic Guide

> Focus: the ARM **core** itself (the parts every silicon vendor shares), not any
> specific microcontroller's peripherals. Everything here is defined by ARM in the
> *ARMv7-M Architecture Reference Manual* and the *Cortex-M7 Technical Reference Manual*.

---

## Table of Contents

- [ARM Cortex-M7 Architecture — A Vendor-Agnostic Guide](#arm-cortex-m7-architecture--a-vendor-agnostic-guide)
  - [Table of Contents](#table-of-contents)
  - [1. Big Picture](#1-big-picture)
  - [2. ARMv7-M / ARMv7E-M and Thumb-2](#2-armv7-m--armv7e-m-and-thumb-2)
  - [3. The Superscalar 6-Stage Pipeline](#3-the-superscalar-6-stage-pipeline)
  - [4. Bus Interfaces: AXI, AHB, and TCM](#4-bus-interfaces-axi-ahb-and-tcm)
  - [5. Caches (I-Cache and D-Cache)](#5-caches-i-cache-and-d-cache)
  - [6. Tightly-Coupled Memory (TCM)](#6-tightly-coupled-memory-tcm)
  - [7. Memory Model](#7-memory-model)
  - [8. Core and Special Registers](#8-core-and-special-registers)
  - [9. The FPU (FPv5, Single or Double Precision)](#9-the-fpu-fpv5-single-or-double-precision)
  - [10. The DSP Extension](#10-the-dsp-extension)
  - [11. Operating Modes, Privilege, and the Two Stacks](#11-operating-modes-privilege-and-the-two-stacks)
  - [12. Exceptions and Interrupts (NVIC)](#12-exceptions-and-interrupts-nvic)
  - [13. Exception Entry and EXC\_RETURN](#13-exception-entry-and-exc_return)
  - [14. Reset, Vector Table, and System Control Space](#14-reset-vector-table-and-system-control-space)
  - [15. The MPU](#15-the-mpu)
  - [16. Cache Coherency and Maintenance — the Big Gotcha](#16-cache-coherency-and-maintenance--the-big-gotcha)
  - [17. Debug and Trace Architecture](#17-debug-and-trace-architecture)
  - [18. Low-Power and Sleep](#18-low-power-and-sleep)
  - [19. Memory Barriers](#19-memory-barriers)
  - [20. What the Cortex-M7 Adds over the M4](#20-what-the-cortex-m7-adds-over-the-m4)
  - [21. Bring-Up Mental Model](#21-bring-up-mental-model)
  - [22. Further References](#22-further-references)

---

## 1. Big Picture

The **Cortex-M7** is ARM's **highest-performance microcontroller core** in the classic
M-profile line. It implements the **ARMv7-M** architecture (specifically the **ARMv7E-M**
variant with the DSP extension, same as the M4), so it is **fully binary-compatible with
the M4** — but its microarchitecture is dramatically more powerful:

- **Superscalar, 6-stage, dual-issue pipeline** with **branch prediction** — it can
  retire up to two instructions per cycle, versus one at a time on the M3/M4.
- Optional **instruction and data caches** (I-cache / D-cache) — the first M-profile
  core with real caches.
- **Tightly-Coupled Memory (TCM)** — ITCM/DTCM for deterministic, single-cycle,
  cache-free access to hot code and data.
- **64-bit AXI** system bus (plus AHBP peripheral and AHBS slave ports) — far more
  bandwidth than the 32-bit AHB-Lite of the smaller cores.
- Optional **FPU (FPv5)** in **single or double precision** — the double-precision
  option (`double` in hardware) is a first for Cortex-M.
- Full **DSP/SIMD** extension (like the M4).
- **MPU** with up to **16 regions** (vs. 8 on M3/M4).
- Runs at much higher clocks (hundreds of MHz), delivering ~**2.14 DMIPS/MHz** (vs.
  ~1.25 on the M4).

In short: the M7 is "**the M4's programmer's model, scaled up for real performance** —
caches, TCM, dual-issue, double-precision FP." Everything you know from the M3/M4
transfers; what's new is mostly about **memory system management** (caches/TCM) and
squeezing the wider pipeline.

```
          +-----------------------------------------------------------+
          |                       Cortex-M7 core                      |
          |  +------------------+   +---------+   +----------------+   |
          |  | 6-stage superscalar |  NVIC   |   | FPU FPv5       |   |
          |  | dual-issue + BP   |  (IRQ)    |   | (SP or DP)     |   |
          |  +------------------+   +---------+   +----------------+   |
          |  +---------+ +---------+ +---------+  +----------------+   |
          |  | I-Cache | | D-Cache | |  MPU    |  | Debug (DWT,ITM,|   |
          |  | (opt.)  | | (opt.)  | | 16 rgn  |  |  ETM, CoreSight)| |
          |  +---------+ +---------+ +---------+  +----------------+   |
          |  +---------+ +---------+                                   |
          |  |  ITCM   | |  DTCM   |  (tightly-coupled, deterministic) |
          |  +---------+ +---------+                                   |
          +--------------------|--------------------------------------+
                       64-bit AXI  +  AHBP (periph)  +  AHBS (slave)
```

---

## 2. ARMv7-M / ARMv7E-M and Thumb-2

- The M7 implements **ARMv7E-M** — ARMv7-M **plus the DSP extension** — exactly the same
  architecture level as the Cortex-M4. So the **instruction set is identical to the M4**:
  full Thumb-2, hardware divide, `IT`, `CBZ/CBNZ`, `TBB/TBH`, bitfield ops, exclusives,
  **saturating/SIMD DSP** instructions, and (with the FPU) floating-point instructions.
- Always Thumb state, **T bit** always 1; function pointers/vectors carry **LSB = 1**.
- Because it is the same ISA as the M4, **M4 binaries run on the M7 unchanged**. The
  gains come from the microarchitecture (dual-issue, caches, TCM, higher clock), not new
  instructions — with the one exception of the **optional double-precision FP** (see §9).

---

## 3. The Superscalar 6-Stage Pipeline

The single biggest performance jump over the M3/M4 is the pipeline:

- **6 stages**, **dual-issue (superscalar)**: under the right conditions the M7 issues
  **two instructions per cycle** (e.g. an ALU op alongside a load), where the 3-stage
  M3/M4 issue one.
- **Branch prediction:** a dynamic branch predictor hides much of the deeper pipeline's
  branch penalty (a longer pipeline would otherwise pay more for mispredicts).
- **In-order** issue but with **dual-issue pairing rules** — not every instruction pair
  can co-issue; the compiler and instruction scheduling matter more than on the M4.
- The deeper pipeline means **more work per cycle and much higher achievable clocks**,
  but also makes timing **less trivially deterministic** than the short M0/M3/M4
  pipelines — which is exactly why the M7 pairs itself with **TCM** (§6) for code/data
  that must be deterministic.

Result: roughly **2.14 DMIPS/MHz** and **5.01 CoreMark/MHz**, versus ~1.25 DMIPS/MHz on
the M4 — before counting the higher clock speeds the M7 can reach.

---

## 4. Bus Interfaces: AXI, AHB, and TCM

The M7 replaces the M3/M4's 32-bit AHB-Lite trio with a higher-bandwidth set:

| Interface | Width | Purpose                                                          |
|-----------|-------|------------------------------------------------------------------|
| **AXIM**  | 64-bit| Main system master — Flash, external RAM, most memory (cacheable) |
| **AHBP**  | 32-bit| Peripheral port — direct, non-cached path to peripherals         |
| **AHBS**  | 32-bit| Slave port — lets a DMA/other master reach the TCMs              |
| **ITCM**  | 64-bit| Instruction Tightly-Coupled Memory interface                     |
| **DTCM**  | 2×32-bit| Data Tightly-Coupled Memory interface (dual banks)             |

The **64-bit AXI** master feeds the caches with much more bandwidth than a 32-bit bus.
The **AHBP** gives peripherals a predictable, non-cacheable path so peripheral accesses
never get tangled up in cache behavior. The **AHBS** slave port lets a DMA engine read/
write the TCMs.

---

## 5. Caches (I-Cache and D-Cache)

The M7 is the **first Cortex-M with real caches** (optional, sizes chosen by the vendor,
typically 4–64 KB each):

- **I-Cache** — caches instruction fetches; large speedup when running from slow Flash.
- **D-Cache** — caches data; write-back or write-through, configurable.
- Both are enabled via **CMSIS** helpers (`SCB_EnableICache()`, `SCB_EnableDCache()`)
  which drive the **CCR** cache-enable bits after invalidating the caches.
- Caches make average performance far higher, **but introduce coherency concerns** with
  DMA and multi-master access that simply do not exist on the cache-less M0–M4 (see §16 —
  this is the M7's defining new pitfall).

The cache line size is **32 bytes**; alignment and cleaning/invalidation always happen
in line-sized units.

---

## 6. Tightly-Coupled Memory (TCM)

Alongside caches, the M7 offers **Tightly-Coupled Memory** — fast SRAM wired directly to
the core with **single-cycle, deterministic** access and **no caching involved**:

- **ITCM** (Instruction TCM) — put latency-critical code / ISRs here for jitter-free
  fetch.
- **DTCM** (Data TCM) — put hot data, stacks, or DMA buffers here for deterministic
  access (and, for DMA buffers, to sidestep D-cache coherency headaches).
- TCM gives you the **determinism of a small MCU** while the caches give you the
  **throughput of a big one** — you choose per code/data section via the linker script.
- The **AHBS** slave port lets DMA reach the TCMs while the core runs from cache.

Deciding what lives in **Flash (cached) vs. ITCM vs. DTCM vs. cached SRAM** is a core
part of M7 system design — and a concern that does not exist on the simpler cores.

---

## 7. Memory Model

The 4 GB fixed-region layout is the standard M-profile map (same regions as the M3/M4):

| Address range              | Region       | Notes for the M7                              |
|----------------------------|--------------|-----------------------------------------------|
| 0x0000_0000–0x1FFF_FFFF    | Code         | Flash / ROM; **ITCM often aliased here**      |
| 0x2000_0000–0x3FFF_FFFF    | SRAM         | On-chip RAM; **DTCM often here**              |
| 0x4000_0000–0x5FFF_FFFF    | Peripheral   | Vendor peripherals (reach via AHBP)           |
| 0x6000_0000–0x9FFF_FFFF    | External RAM | Off-chip memory (cacheable via AXI)           |
| 0xA000_0000–0xDFFF_FFFF    | External dev | Off-chip peripherals                          |
| 0xE000_0000–0xE00F_FFFF    | PPB          | System Control Space (NVIC, SCB, SysTick)     |
| 0xE010_0000–0xFFFF_FFFF    | Vendor       | Vendor-specific                               |

- **Cacheability/shareability** of each region is governed by the memory type and by the
  MPU — you must mark DMA buffers appropriately (or place them in DTCM / non-cacheable
  regions).
- **No bit-banding** on the M7. Although it is ARMv7-M, ARM **dropped bit-banding** from
  the M7 (and M4) implementations; use exclusives (`LDREX/STREX`) or interrupt masking
  for atomic bit updates.
- **Unaligned access** to normal memory is supported (as on M3/M4).

---

## 8. Core and Special Registers

The programmer-visible register model is **the same as the M4** (ARMv7E-M):

- Sixteen 32-bit registers **R0–R15** (SP banked MSP/PSP, LR, PC); AAPCS calling
  convention.
- **xPSR** with **N, Z, C, V, Q** flags and **GE[3:0]** bits (Q and GE are present
  because of the DSP extension, just like the M4).
- Full masking set: **PRIMASK, FAULTMASK, BASEPRI**.
- **CONTROL** with **nPRIV, SPSEL, and FPCA** (FPCA present because the FPU exists).
- FPU register bank **S0–S31 / D0–D15** (and **D16 aliasing** rules depending on
  single/double-precision option) plus **FPSCR**.

If you know the M4 register model, you already know the M7's. See the
[Cortex-M4 guide §5–6](cortex-m4-architecture-guide.md#5-core-registers) for the full
detail — it applies unchanged.

---

## 9. The FPU (FPv5, Single or Double Precision)

The M7's FPU is **FPv5**, one generation beyond the M4's FPv4-SP, and is offered in two
configurations:

- **FPv5-SP** — single precision only (`float` in hardware, `double` emulated), like the
  M4 but a newer revision with a few extra instructions.
- **FPv5-DP** — **single *and* double precision** (`double` in hardware!). This is the
  headline FP feature unique to the M7 in this family — no M0/M3/M4 can do hardware
  `double`.
- Register bank **S0–S31 / D0–D15** (SP) or the full **D0–D15** double set (DP), plus
  **FPSCR** (rounding mode, exception flags, FP condition flags).
- IEEE-754 compliant: hardware add/sub/mul/div/sqrt/MAC, and for FPv5 additional
  fused/rounding instructions.

**Enabling the FPU** (off at reset): set CP10/CP11 access bits in **CPACR**
(`SCB->CPACR |= (0xF << 20);`) then **DSB; ISB** — identical to the M4.

**Lazy stacking (FPCCR):** as on the M4, on exception entry the core reserves space for
FP registers but only actually stacks them if the handler uses the FPU, keeping
interrupt latency low.

---

## 10. The DSP Extension

The M7 includes the **same DSP/SIMD extension as the M4** (that is what "ARMv7E-M"
means):

- **Single-cycle MAC** and 32×32→64 operations.
- **SIMD** on packed 8/16-bit data (`QADD8`, `SADD16`, `SMLAD`, etc.).
- **Saturating arithmetic** (`QADD`, `QSUB`, `SSAT`, `USAT`) — sets the **Q flag**.
- Packing/unpacking, `CLZ`, etc.

Combined with the wider pipeline and (optionally) double-precision FP, the M7 targets
heavy DSP, motor control, audio, and sensor-fusion workloads at much higher throughput
than the M4. **CMSIS-DSP** runs here too, just faster.

---

## 11. Operating Modes, Privilege, and the Two Stacks

Identical to the M4: two modes (Thread/Handler), two privilege levels, banked **MSP/PSP**
stacks, selected by **CONTROL**. A typical RTOS runs the kernel/ISRs privileged on MSP
and tasks unprivileged on PSP with the **MPU** for isolation. See
[Cortex-M4 guide §7–8](cortex-m4-architecture-guide.md#7-operating-modes-and-privilege).

---

## 12. Exceptions and Interrupts (NVIC)

The M7 has the **full ARMv7-M NVIC**, like the M3/M4:

- **Up to 240 external interrupts**, up to **256 priority levels**, **priority
  grouping** (preempt/subpriority via AIRCR).
- **Vectored, nested, tail-chaining, late-arrival.**
- Full **configurable faults**: Reset, NMI, HardFault, **MemManage, BusFault,
  UsageFault**, SVCall, DebugMonitor, PendSV, SysTick, then IRQs — with
  **CFSR/HFSR/MMFAR/BFAR** status registers.

See [Cortex-M4 guide §9](cortex-m4-architecture-guide.md#9-exceptions-and-interrupts-nvic).
One M7 nuance: the deeper pipeline is designed so **interrupt latency stays low and
deterministic** (same 12-cycle-class entry as the M3/M4) despite the higher performance —
placing ISRs in **ITCM** keeps that latency jitter-free.

---

## 13. Exception Entry and EXC_RETURN

Same as the M4. Hardware stacks the **8-word basic frame** (xPSR, PC, LR, R12, R3–R0);
if FP context is active, an **extended frame** additionally stacks S0–S15 and FPSCR
(lazy-stacked). EXC_RETURN therefore has **both** the non-FP forms (`0xFFFFFFF1/F9/FD`)
and the FP forms (`0xFFFFFFE1/E9/ED`), exactly like the M4F. See
[Cortex-M4 guide §10](cortex-m4-architecture-guide.md#10-exception-entry-and-exc_return).

---

## 14. Reset, Vector Table, and System Control Space

Reset and the vector table work exactly as on the M3/M4: MSP from word 0, reset vector
from word 1, privileged Thread mode on MSP, relocatable via **VTOR**. The **PPB** at
0xE000_0000 hosts SysTick, the full NVIC, the SCB (with **CFSR/HFSR/MMFAR/BFAR** fault
registers), the MPU, and the FPU control registers — plus M7-specific **cache control**
bits in the SCB (CCR I/D-cache enables, and cache maintenance operation registers). See
[Cortex-M4 guide §11–12](cortex-m4-architecture-guide.md#11-reset-and-vector-table).

---

## 15. The MPU

The M7's optional **MPU** is the ARMv7-M protection unit like the M3/M4, but larger:

- **Up to 16 regions** (vs. 8 on the M3/M4) — more granular partitioning for complex
  systems.
- Per-region permissions, execute-never, memory type, **and cacheability/shareability
  attributes** — on the M7 the MPU is also how you mark regions **non-cacheable** (e.g.
  for DMA buffers that live in cacheable SRAM).
- Region priority, sub-region disable, MemManage fault on violation — as on the M3/M4.

Because the M7 has caches, **using the MPU to control cacheability is a routine part of
DMA setup**, not just a protection feature. Registers: **MPU_TYPE, CTRL, RNR, RBAR,
RASR**; `DSB; ISB` after changes.

---

## 16. Cache Coherency and Maintenance — the Big Gotcha

This is the **defining new concern** on the M7 that does not exist on the M0–M4. With a
**D-cache**, the core and other bus masters (DMA, another CPU) can disagree about
memory:

- **CPU → DMA (TX):** if the CPU wrote a buffer that sits in cacheable memory, the data
  may still be **in the D-cache**, not in RAM. Before starting a DMA read of it, you must
  **clean** (flush) the cache so RAM is up to date: `SCB_CleanDCache_by_Addr(...)`.
- **DMA → CPU (RX):** after DMA writes a buffer in cacheable memory, the CPU's cache may
  hold **stale** old data. Before reading it you must **invalidate** the cache lines:
  `SCB_InvalidateDCache_by_Addr(...)`.
- Operations work on **32-byte cache lines**, so DMA buffers should be **32-byte aligned
  and line-sized** to avoid corrupting neighbours during invalidate.

**Common strategies:**
1. Place DMA buffers in **DTCM** or a **non-cacheable MPU region** — no maintenance
   needed (simplest, very common).
2. Keep buffers cacheable for CPU speed and do explicit **clean/invalidate** around each
   DMA transfer (more code, best CPU throughput).

Forgetting cache maintenance produces the classic M7 bug: "my DMA works on the M4 but
returns stale/garbage data on the M7." Internalizing this is the single most important
step when moving up to the M7.

---

## 17. Debug and Trace Architecture

The M7 has the full CoreSight debug/trace suite, a superset of the M3/M4:

| Block | Purpose                                                                   |
|-------|---------------------------------------------------------------------------|
| DWT   | Data Watchpoint & Trace — watchpoints, cycle counter (CYCCNT), profiling  |
| FPB   | Flash Patch & Breakpoint — hardware breakpoints                           |
| ITM   | Instrumentation Trace Macrocell — `printf`-style trace (SWO)              |
| TPIU  | Trace Port Interface Unit — funnels trace off-chip                        |
| ETM   | (optional) Embedded Trace Macrocell — full instruction trace              |

Debug access via **SWD** or **JTAG**; **DWT->CYCCNT** for cycle measurement; **ITM/SWO**
for low-overhead trace. Trace bandwidth needs are higher given the M7's speed, so
vendors often fit wider trace ports (TPIU) or an ETM.

---

## 18. Low-Power and Sleep

Same architectural mechanisms as the rest of the family — **WFI, WFE, Sleep-on-exit,
SLEEPDEEP, SEVONPEND** via **SCB->SCR**. The M7 is a **performance** core, not an
ultra-low-power one; at equal process it draws considerably more than an M0+/M4, so
low-power designs use it in bursts ("race to sleep") with aggressive clock/power gating
managed by vendor-specific power controllers.

---

## 19. Memory Barriers

The three barriers **DMB / DSB / ISB** behave as on the M4 — but they matter **more** on
the M7 because of caches, the write buffer, dual-issue, and multiple bus masters:

- After **cache maintenance** operations, a **DSB** (and often **ISB**) ensures the
  operation completes before dependent accesses.
- After enabling the FPU, MPU, or caches, or changing CONTROL → **DSB; ISB**.
- Around **shared memory with DMA** the combination of correct cache maintenance **and**
  barriers is required for correctness.

On the M7 you cannot be as cavalier about barriers as on a single-master, cache-less M3.

---

## 20. What the Cortex-M7 Adds over the M4

Both are ARMv7E-M (same ISA, same DSP, FPU option, full NVIC, configurable faults). The
M7 differences are all **microarchitecture and memory system**:

| Aspect              | Cortex-M4                     | Cortex-M7                                   |
|---------------------|-------------------------------|---------------------------------------------|
| Pipeline            | 3-stage, single-issue          | **6-stage, dual-issue (superscalar) + branch prediction** |
| Performance         | ~1.25 DMIPS/MHz               | **~2.14 DMIPS/MHz**, much higher clocks     |
| Caches              | none                          | **optional I-cache + D-cache**              |
| TCM                 | none                          | **ITCM + DTCM** (deterministic)             |
| System bus          | 32-bit AHB-Lite               | **64-bit AXI** + AHBP + AHBS                |
| FPU                 | FPv4-SP (single only)         | **FPv5, single *or* double precision**      |
| MPU regions         | up to 8                       | **up to 16**                                |
| Bit-banding         | dropped (not present)         | dropped (not present)                       |
| New concerns        | —                             | **cache coherency / maintenance for DMA**   |

If you know the M4, learning the M7 is mostly about mastering **caches, TCM, and cache
maintenance** — plus enjoying hardware `double` if your part has FPv5-DP.

---

## 21. Bring-Up Mental Model

A useful order when a Cortex-M7 boots (extends the M4 sequence with cache/TCM steps):

```
1. Power/clock stable  ->  core reads MSP (vector[0]) and PC (vector[1])
2. Reset_Handler runs  ->  copy .data (incl. ITCM/DTCM sections) to RAM, zero .bss,
                           set up clocks
3. (optional) relocate vector table via VTOR
4. configure MPU: set cacheable/non-cacheable + shareable regions
   (esp. mark DMA-buffer regions non-cacheable if not using DTCM); DSB; ISB
5. invalidate then enable I-cache and D-cache (SCB_EnableICache/DCache); DSB; ISB
6. enable FPU (CPACR) if using floats; DSB; ISB
7. set interrupt priorities (NVIC IPR / SCB SHPR), grouping (AIRCR)
8. enable peripheral IRQs (NVIC ISER)
9. (optional) drop privilege / switch to PSP for an RTOS (write CONTROL; ISB)
10. call main() -> scheduler / superloop
```

Steps 4–5 (MPU cacheability + cache enable) are the genuinely new M7 work.

**Debugging a HardFault** is the same as the M3/M4 (read HFSR → CFSR → MMFAR/BFAR,
recover stacked PC). But add **cache coherency** to your list of suspects for
"data is wrong but no fault" bugs involving DMA.

---

## 22. Further References

**Primary ARM documentation (authoritative, vendor-agnostic):**
- *ARM®v7-M Architecture Reference Manual* (ARM DDI 0403) — the ISA, exception model, and
  system registers (shared with M3/M4).
- *Cortex®-M7 Technical Reference Manual* (ARM DDI 0489) — the core implementation:
  pipeline, caches, TCM, AXI/AHB buses, cache maintenance operations.
- *Cortex-M7 Devices Generic User Guide* (ARM DUI 0646) — the approachable programmer's-
  model document; covers NVIC, SCB (incl. cache control), MPU, FPU register-by-register.
- *AAPCS — Procedure Call Standard for the ARM Architecture* (calling convention).

**Books:**
- Joseph Yiu, *The Definitive Guide to ARM® Cortex®-M7 Processors* (and the general
  Cortex-M books) — the best coverage of the caches/TCM/coherency material.

**Software / hands-on:**
- **CMSIS** — `core_cm7.h` maps the core registers **and** the cache-maintenance helpers
  (`SCB_EnableICache`, `SCB_EnableDCache`, `SCB_CleanDCache_by_Addr`,
  `SCB_InvalidateDCache_by_Addr`); read these alongside this guide.
- **CMSIS-DSP** — runs on the M7's DSP/FPU; benchmark it against the M4 to see the
  dual-issue/clock gains.
- **FreeRTOS Cortex-M7 port** — same as the M4F port (BASEPRI critical sections, lazy FP
  stacking); the extra work is cache-aware buffer placement for peripherals.

**Practical exercises to cement the concepts:**
1. Place an ISR in **ITCM** and a hot buffer in **DTCM** via the linker script; measure
   the jitter difference vs. running from cached Flash.
2. Set up a DMA transfer with a **cacheable** buffer and deliberately omit cache
   maintenance to reproduce the stale-data bug, then fix it with clean/invalidate.
3. Enable I-cache and D-cache and measure the CoreMark uplift vs. caches off.
4. Compare a `double`-heavy computation on an FPv5-DP M7 (hardware) vs. an M4 (software).
5. Use DWT->CYCCNT to observe dual-issue: compare an instruction mix that pairs well vs.
   one that serializes.

---

*This guide describes the ARM Cortex-M7 core as defined by ARM. Peripheral details
(GPIO, UART, clock trees, flash controllers) are vendor-specific — always cross-check
with your silicon vendor's reference manual for memory addresses and peripheral behavior.*
