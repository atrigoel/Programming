/* Bare-metal Cortex-M4 demo built with arm-none-eabi-gcc.
 * Exercises .data (initialized) and .bss (zeroed) so the startup code and
 * linker script have real work to do. No HAL, no OS. */

#include <stdint.h>

volatile uint32_t counter = 0xABCD;   /* .data: copied FLASH->RAM at reset */
volatile uint32_t scratch;            /* .bss:  zeroed at reset */

static void spin(volatile uint32_t n)
{
    while (n--)
        __asm__ volatile("nop");
}

int main(void)
{
    for (;;) {
        scratch = counter++;
        spin(100000);
    }
    return 0;
}
