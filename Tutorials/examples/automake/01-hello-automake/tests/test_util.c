#include <stdio.h>
#include "util.h"

int main(void)
{
    int failures = 0;

    if (sum_of_squares(3, 4) != 25) {
        fprintf(stderr, "FAIL: sum_of_squares(3,4) != 25\n");
        failures++;
    }
    if (sum_of_squares(0, 0) != 0) {
        fprintf(stderr, "FAIL: sum_of_squares(0,0) != 0\n");
        failures++;
    }

    if (failures == 0)
        printf("all tests passed\n");

    /* Non-zero exit tells `make check` the test failed. */
    return failures == 0 ? 0 : 1;
}
