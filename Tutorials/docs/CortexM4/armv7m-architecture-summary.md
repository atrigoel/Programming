# ARMv7-M Architecture — Explained

This guide summarizes the ARMv7-M Architecture Reference Manual
(`docs/DDI0403E_e_armv7m_arm.pdf`) in practical terms. It explains the programming
model, instruction execution, memory system, exceptions, protection, and debugging
concepts that firmware developers encounter.

> **Architecture versus processor:** ARMv7-M specifies the rules software can rely on.
> A Cortex-M processor implements that architecture, and a chip vendor connects the
> processor to Flash, RAM, peripherals, and other hardware. Optional features and
> implementation-defined limits differ between processors and chips. Check the
> processor and device manuals before relying on a particular feature or capacity.

## 1. The big picture

ARMv7-M is the ARMv7 profile for microcontrollers and other deeply embedded systems.
It is designed for compact code, low-latency response to events, and predictable
control of memory-mapped hardware. Cortex-M3 and Cortex-M4 are familiar processors
that implement ARMv7-M; the architecture manual describes the contract they follow,
not every detail of either processor.

The main ingredients are:

- A 32-bit programmer-visible data model and address space.
- Thumb instruction execution, including 16-bit and 32-bit instruction encodings.
- A unified memory map, even when a processor uses separate internal instruction and
  data paths.
- An exception model that automatically saves basic execution state.
- A nested interrupt controller and system-control registers.
- Optional features, such as an MPU or floating-point support, where implemented.

ARMv7-M is not ARM's application profile. It does not, by itself, imply an operating
system, virtual memory, or a particular set of on-chip peripherals.

## 2. Instruction execution: Thumb

An ARMv7-M processor executes Thumb instructions. It does not switch between the
classic ARM instruction state and Thumb state. Instructions may use compact 16-bit
encodings or wider 32-bit Thumb encodings, balancing code size with richer operations.

The architecture includes integer arithmetic, loads and stores, branches, and
conditional execution mechanisms. Implementations supporting the relevant instruction
features can also provide operations such as integer divide, bitfield manipulation,
table branches, and exclusive loads and stores. The exact instruction set is determined
by the architecture version and extensions implemented by a processor.

The least significant bit in a code pointer or exception-handler address indicates
Thumb state. It is not part of the actual instruction address. Toolchains normally
set and interpret this bit for function pointers and vector entries automatically.
Do not clear or invent it by hand unless writing low-level startup or linker code and
you understand the required format.

### Exclusive accesses and synchronization

The `LDREX`/`STREX` family supports synchronization patterns in which software reads a
value and then conditionally writes it if the monitored value has not been changed.
These instructions are building blocks for locks and atomic operations, not a blanket
guarantee that every access to every memory type is atomic. Correct use depends on the
memory region, competing agents, and the software's synchronization design.

## 3. Registers and status

The core programming model exposes sixteen 32-bit registers:

| Register(s) | Common name | Purpose |
|---|---|---|
| R0-R12 | General-purpose | Arithmetic, addresses, arguments, and temporary values |
| R13 | SP | Stack pointer; the active stack is the Main or Process Stack Pointer |
| R14 | LR | Link register; holds return information for calls and exceptions |
| R15 | PC | Program counter |

The Program Status Register is commonly viewed as three parts:

- **APSR** holds arithmetic condition flags such as Negative, Zero, Carry, and
  Overflow. Additional flags depend on implemented instruction extensions.
- **IPSR** identifies the currently active exception; zero indicates Thread mode.
- **EPSR** contains execution-state information, including Thumb state and state used
  by certain instruction sequences.

The combined register is called **xPSR**. `MRS` and `MSR` instructions access the
architecturally available special registers, subject to privilege and register-specific
rules. The compiler's ABI also assigns roles to registers; that calling convention is
related to, but distinct from, the architecture's hardware register definitions.

## 4. Modes, privilege, and stacks

ARMv7-M has two execution modes:

- **Thread mode** runs application code after reset and after returning from an
  exception.
- **Handler mode** runs an exception or interrupt handler.

Thread mode can be privileged or unprivileged. Handler mode is privileged. Privileged
code can access system controls that unprivileged code cannot; an MPU, if implemented
and enabled, can further limit memory access.

There are two stack pointers:

- **MSP (Main Stack Pointer)** is used in Handler mode and is the default stack after
  reset.
- **PSP (Process Stack Pointer)** can be selected for Thread mode, often for
  application tasks in an RTOS.

The active Thread-mode stack is selected through the CONTROL register. Handler mode
uses MSP. This arrangement lets an RTOS keep handler/kernel execution separate from
per-task stacks, but it does not automatically isolate tasks: software must configure
the stacks and any protection features correctly.

## 5. Exceptions and interrupts

An **exception** is the architecture's general term for an event that changes the
normal instruction flow. It includes reset, processor faults, software requests, and
peripheral interrupts. An exception table, called the **vector table**, supplies the
initial stack pointer and handler addresses.

The exception model defines system exceptions such as Reset, NMI, HardFault,
MemManage, BusFault, UsageFault, SVCall, Debug Monitor, PendSV, and SysTick. Which
exceptions are available, configurable, or implemented depends on the architecture
requirements and the processor implementation. External interrupt numbers and counts
are chip/processor-specific.

The **NVIC** (Nested Vectored Interrupt Controller), where implemented, manages
interrupt enablement, pending state, active state, and priorities. Priority determines
which eligible exception can preempt another. Priority numbering is counterintuitive:
a lower numerical priority value represents higher urgency. Processors may implement
fewer priority bits than the register width suggests, so software must use the
implemented priority range.

Useful exception-controller behaviors include:

- **Nesting:** a sufficiently urgent exception can interrupt a lower-priority handler.
- **Tail-chaining:** the processor can move directly from one handler to another
  without fully restoring and saving Thread-mode state between them.
- **Late arrival:** a higher-priority exception arriving during entry can be serviced
  before the initially selected lower-priority exception.

These behaviors reduce interrupt latency and avoid unnecessary work. They do not
replace careful priority design, short handlers, or safe sharing of data between
handlers and application code.

### Exception entry and return

On normal exception entry, hardware stacks a basic frame containing R0-R3, R12, LR, PC,
and xPSR. It then enters Handler mode and begins executing the selected handler.
Floating-point-capable implementations may have additional context rules when the
floating-point extension is present and active.

The value in LR during a handler is an **EXC_RETURN** code, not an ordinary function
return address. It tells the processor how to return, including whether to resume
Thread or Handler mode and which stack frame to use. Exception return restores the
saved state. A handler must preserve the architecture's exception-return convention;
ordinary function-call return logic is not a substitute.

### Faults and diagnosis

Fault exceptions turn problems such as invalid instruction execution, access
violations, or failed bus transactions into controlled exception handling. Some faults
can be enabled and handled separately; otherwise they can escalate to HardFault.
System Control Block fault-status registers, where implemented, record information
that helps identify the cause. Address registers may identify a faulting address when
the relevant status indicates that the address is valid.

Fault handlers should inspect status bits before assuming an address or stacked frame
is meaningful. A fault can happen during exception entry or while handling another
fault, so robust diagnostics should also account for a corrupted or unavailable stack
frame.

## 6. Reset and the vector table

At reset, the processor obtains its initial stack pointer and reset-handler address
from the vector table according to the implementation's boot mapping. The reset handler
normally initializes runtime memory, configures clocks and devices as required, and
then calls the program's main function.

The table also provides entries for system exceptions and external interrupts. Some
implementations support relocating the vector table through a system-control register;
whether and how that is supported must be checked for the target processor. Bootloaders
and applications must agree on vector-table location and alignment.

The architecture defines the core behavior, but it does not specify a universal
microcontroller's Flash address, startup code, clock setup, or peripheral vector list.

## 7. Address space and memory behavior

The architecture uses a 32-bit address space, divided into broad regions with defined
roles. A typical M-profile map includes Code, SRAM, Peripheral, External RAM, External
Device, and Private Peripheral Bus (PPB) regions. These ranges describe architectural
memory attributes and expected uses; they do not promise that every chip implements
physical memory throughout each range.

The PPB contains processor system-control and debug components, such as the NVIC and
System Control Block, when present. Device-specific peripherals are usually mapped in
the Peripheral region. The vendor's datasheet and reference manual specify which
addresses actually exist on a chip.

The architecture distinguishes memory types because reads and writes can have
different ordering and side-effect rules:

- **Normal memory** is used for code and ordinary data. Implementations may optimize
  accesses according to its attributes.
- **Device memory** is intended for memory-mapped devices, where accesses can have
  side effects and must follow device ordering rules.
- **Strongly ordered memory**, where applicable, has stricter ordering behavior.

The selected memory attributes matter. A peripheral register must not be treated like
ordinary cached or reorderable data. Use the device documentation and appropriate
memory attributes and barriers when ordering is important.

Endianness is an architectural property, but most Cortex-M microcontrollers and their
toolchains are configured for little-endian operation. Verify the target configuration
when exchanging binary data or interacting with hardware-defined formats.

Some ARMv7-M implementations support unaligned accesses for certain instructions and
memory types; other instructions require alignment, and a control setting can make
some unaligned accesses fault. Do not assume all unaligned accesses are supported or
efficient. Use correctly aligned objects, especially for peripheral registers and
shared data structures.

## 8. Memory barriers and ordering

The architecture provides barrier instructions for ordering and synchronization:

- **DMB (Data Memory Barrier)** orders relevant explicit memory accesses.
- **DSB (Data Synchronization Barrier)** waits for relevant prior accesses to complete.
- **ISB (Instruction Synchronization Barrier)** flushes/refetches the instruction
  stream so subsequent instructions observe changes to execution context or control.

Barriers are not interchangeable, and they are not general-purpose locks. Use them
when the architecture, processor manual, or peripheral documentation requires ordering
around operations such as changing system control state, enabling hardware, or
communicating with another execution agent.

## 9. Memory protection

An **MPU (Memory Protection Unit)** is an optional protection mechanism on some
implementations. It divides the address space into regions and applies access
permissions and memory attributes. It can help enforce rules such as read-only code,
restricted peripheral access, or boundaries between task stacks.

An MPU is not an MMU: it does not provide general virtual-memory translation or
demand-paged memory. MPU region count, region layout rules, subregion support, and
available attributes depend on the implementation. Protection is effective only when
software configures it correctly and handles resulting faults.

## 10. Optional bit-band support

Some Cortex-M implementations provide **bit-band** aliases for selected SRAM and
peripheral address ranges. An alias address maps an access to one bit in the associated
region, which can simplify certain single-bit operations.

Bit-banding is not a universal property of every ARMv7-M processor or chip. Check the
processor and device documentation before using it. Even when present, it applies only
to specified regions and is not a general replacement for synchronization, atomic
operations, or peripheral-specific access rules.

## 11. Timing and low power

Some implementations include a **SysTick** system timer, commonly used as a periodic
tick source for an RTOS or simple time base. Its presence and configuration should be
verified for the target. SysTick is not the only timing source: microcontrollers also
provide vendor-specific timers that may be more appropriate for precise measurement,
PWM, or long intervals.

The architecture defines ways for software to request sleep or wait for an event or
interrupt, such as WFI and WFE. The actual power savings and wake behavior depend on
the processor, chip power controller, enabled clocks, and pending events. Firmware
must coordinate sleep with interrupt state and device activity.

## 12. Debugging and trace

ARM defines architectural debug behavior and interfaces that allow tools to inspect
or control a processor. A particular core may also implement hardware breakpoints,
watchpoints, or trace components. Their number, capabilities, and accessibility depend
on the core and chip.

Debug probes and software such as GDB and OpenOCD use the target's debug interface to
halt, inspect, and resume execution. Debug access is not guaranteed to be enabled in
every product: vendors can restrict it for security or production reasons. Trace
components and real-time trace output are also optional and must not be assumed.

## 13. How the pieces fit in firmware

A minimal bare-metal startup sequence often follows this pattern:

1. The processor reads the initial stack pointer and reset vector.
2. Startup code initializes memory required by the C runtime, such as `.data` and
   `.bss`.
3. The program configures chip-specific clocks and peripherals.
4. Application code runs in Thread mode, typically using the selected stack.
5. An interrupt or fault causes automatic exception entry and dispatch to a handler.
6. The handler acknowledges or records the event and returns, or requests deferred
   work.

An RTOS builds on the same mechanisms. It may run kernel operations in privileged
context, use PSP for task stacks, use SVC for controlled service requests, and defer
task switching to PendSV. Those are common design patterns, not mandatory architecture
requirements.

## 14. Common misconceptions

- **“ARMv7-M tells me my chip's memory map.”** It defines architectural regions and
  behavior, not the physical Flash, SRAM, or peripheral layout of a particular chip.
- **“Every ARMv7-M core has bit-banding, an MPU, SysTick, and trace.”** These features
  must be verified against the processor and device documentation.
- **“An interrupt is just a function call.”** Exception entry changes execution mode,
  stacks hardware-defined state, and uses exception priorities and return semantics.
- **“An MPU gives virtual memory.”** It enforces access rules; it does not provide
  general virtual-address translation.
- **“A barrier makes code atomic.”** Barriers order operations; they do not by
  themselves prevent concurrent modification.
- **“Every register described in the architecture exists on my board.”** System
  components and external interrupts may be absent, limited, or configured differently.

## 15. Key takeaways

ARMv7-M is a microcontroller architecture centered on Thumb execution, a 32-bit
programming model, a structured exception system, and explicit memory and privilege
rules. Its exception mechanism and interrupt controller make responsive firmware
possible; its memory attributes, barriers, and optional MPU support controlled access
to hardware and data.

The most important practical rule is to distinguish **architecture guarantees** from
**implementation choices**. Use the ARM architecture manual to understand core
behavior, the processor technical reference manual for core-specific capabilities,
and the chip vendor's reference manual for actual memory maps, peripherals, and startup
requirements.

## References

- [ARMv7-M Architecture Reference Manual](docs/DDI0403E_e_armv7m_arm.pdf)
- [Cortex-M3 architecture guide](cortex-m3-architecture-guide.md) for a processor-
  specific walkthrough of one ARMv7-M implementation.
