#include <stdio.h>
#include "mathx.h"   /* found via mathx's PUBLIC include directory */

int main(void)
{
    printf("triangular(10) = %d\n", mathx_triangular(10));
    printf("gcd(48, 36)    = %d\n", mathx_gcd(48, 36));
    return 0;
}
