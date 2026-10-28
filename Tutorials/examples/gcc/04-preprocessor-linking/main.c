#include <stdio.h>
#include "greet.h"     /* found via -Iinclude */

int main(void)
{
    greet("embedded world");

    /* Compiled in only when -DENABLE_EXTRA is passed on the command line. */
#ifdef ENABLE_EXTRA
    printf("Extra feature enabled (built with -DENABLE_EXTRA).\n");
#else
    printf("Extra feature OFF (rebuild with CPPFLAGS=-DENABLE_EXTRA).\n");
#endif

#ifdef VERSION
    printf("Version = %s\n", VERSION);
#endif
    return 0;
}
