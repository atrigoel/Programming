#include "math_utils.h"

long factorial(int n)
{
    long r = 1;
    for (int i = 2; i <= n; i++)
        r *= i;
    return r;
}

int gcd(int a, int b)
{
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}
