#include <stdio.h>

/* This file intentionally contains sloppy code so the warning flags have
 * something to catch. Build with `make strict` to see them light up. */

int compute(int count)
{
    int total = 0;
    int count2 = count;          /* -Wshadow demo below reuses the name 'total' */

    for (int i = 0; i < count2; i++) {
        int total = i * 2;       /* shadows the outer 'total' -> -Wshadow */
        (void)total;
    }

    unsigned int u = -1;         /* implicit signed->unsigned -> -Wconversion/-Wsign-conversion */
    printf("u = %u\n", u);

    int unused_local = 42;       /* never used -> -Wunused-variable (in -Wall) */

    /* Missing return on this path -> -Wreturn-type; add -Werror to make it fatal. */
    if (count2 > 0)
        return total + (int)u;
}

int main(void)
{
    printf("compute(3) = %d\n", compute(3));
    return 0;
}
