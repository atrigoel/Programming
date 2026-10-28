#include <stdio.h>
#include <string.h>
#include "util.h"

int sum_to(int n)
{
    int s = 0;
    for (int i = 1; i <= n; i++)
        s += i;
    return s;
}

void print_banner(const char *msg)
{
    size_t len = strlen(msg);
    for (size_t i = 0; i < len + 4; i++) putchar('*');
    printf("\n* %s *\n", msg);
    for (size_t i = 0; i < len + 4; i++) putchar('*');
    putchar('\n');
}
