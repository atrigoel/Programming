#include <stdio.h>
#include <stdint.h>

/* A little arithmetic the optimizer can chew on: constant folding, loop work,
 * and a call that can be inlined. Compare the assembly across -O levels. */

static uint32_t square(uint32_t x)
{
    return x * x;
}

uint32_t sum_of_squares(uint32_t n)
{
    uint32_t acc = 0;
    for (uint32_t i = 1; i <= n; i++)
        acc += square(i);
    return acc;
}

int main(void)
{
    volatile uint32_t n = 100;   /* volatile so the optimizer can't fold it away */
    printf("sum_of_squares(%u) = %u\n", n, sum_of_squares(n));
    return 0;
}
