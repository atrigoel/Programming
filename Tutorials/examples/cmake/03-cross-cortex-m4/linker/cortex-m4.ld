/* Minimal Cortex-M4 linker script (generic STM32F407-class layout):
 *   FLASH 1 MB @ 0x08000000, SRAM 128 KB @ 0x20000000.
 * Enough to link a bare-metal image with a vector table, .data init, and .bss. */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Top of stack = end of RAM (stack grows downward). */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector table first, then code and read-only data — all in FLASH. */
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* .data lives in RAM at run time (VMA) but is stored in FLASH (LMA);
     * startup copies it from _sidata to _sdata.._edata. */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* .bss is zeroed by startup; occupies RAM only, no FLASH storage. */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
