#include <stdio.h>

/* A small call chain so there's a real stack to walk with `backtrace`. */

static long square(long x) {
    return x * x;                 /* `step` into here from sum_of_squares */
}

static long sum_of_squares(long n) {
    long total = 0;
    for (long i = 1; i <= n; i++) {
        total += square(i);       /* set a breakpoint on this line */
    }
    return total;
}

int main(void) {
    long n = 5;
    long result = sum_of_squares(n);
    printf("sum of squares 1..%ld = %ld\n", n, result);
    return 0;
}
