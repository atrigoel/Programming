#include <stdio.h>

int table[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };   /* .data */
int scratch[256];                              /* .bss  */

static int helper(int x) { return x * x; }     /* may be gc'd if unused */

int main(void)
{
    int sum = 0;
    for (int i = 0; i < 8; i++)
        sum += helper(table[i]);
    printf("sum of squares of table = %d\n", sum);
    printf("scratch[0] = %d\n", scratch[0]);
    return 0;
}
