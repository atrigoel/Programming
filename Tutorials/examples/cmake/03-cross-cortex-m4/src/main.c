/* Bare-metal "blinky" skeleton. No SVD/HAL — just enough to produce a real
 * Cortex-M4 image that exercises .data and .bss so you can inspect the map file. */

#include <stdint.h>

/* Initialized global -> lives in .data (copied from FLASH to RAM at startup). */
volatile uint32_t tick_count = 0;

/* Zero-initialized global -> lives in .bss (zeroed at startup). */
volatile uint32_t last_state;

static void delay(volatile uint32_t n)
{
    while (n--)
        __asm__ volatile("nop");
}

int main(void)
{
    for (;;) {
        /* Pretend to toggle an LED; real hardware would write a GPIO register. */
        last_state ^= 1u;
        tick_count++;
        delay(100000);
    }
    return 0;
}
