/* Minimal Cortex-M4 startup: vector table + reset handler.
 * Copies .data from FLASH to RAM, zeroes .bss, then calls main(). */

    .syntax unified
    .cpu cortex-m4
    .thumb

/* ---- Vector table -------------------------------------------------------- */
    .section .isr_vector, "a", %progbits
    .type g_vectors, %object
g_vectors:
    .word _estack           /* 0x00: initial stack pointer */
    .word Reset_Handler     /* 0x04: reset */
    .word Default_Handler   /* 0x08: NMI */
    .word Default_Handler   /* 0x0C: HardFault */
    /* (remaining fault/IRQ vectors omitted for brevity) */

/* ---- Reset handler ------------------------------------------------------- */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Copy .data initializers from FLASH (_sidata) to RAM (_sdata.._edata). */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
copy_data:
    cmp r0, r1
    bge zero_bss
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4
    adds r0, r0, #4
    b copy_data

    /* Zero the .bss section. */
zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
loop_bss:
    cmp r0, r1
    bge call_main
    str r2, [r0]
    adds r0, r0, #4
    b loop_bss

call_main:
    bl main
    /* If main returns, spin forever. */
hang:
    b hang
    .size Reset_Handler, .-Reset_Handler

/* ---- Default handler ----------------------------------------------------- */
    .section .text.Default_Handler
    .weak Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b Default_Handler
    .size Default_Handler, .-Default_Handler
