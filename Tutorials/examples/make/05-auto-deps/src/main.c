#include <stdio.h>
#include "math_utils.h"
#include "config.h"

int main(void)
{
    printf("%s v%s\n", APP_NAME, APP_VERSION);
    printf("factorial(5) = %ld\n", factorial(5));
    printf("gcd(48, 36) = %d\n", gcd(48, 36));
    return 0;
}
