#include <stdio.h>
#include "util.h"
#include "config.h"

int main(void)
{
    printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    printf("sum_of_squares(3, 4) = %d\n", sum_of_squares(3, 4));
    return 0;
}
