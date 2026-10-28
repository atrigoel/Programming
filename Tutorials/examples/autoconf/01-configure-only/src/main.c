#include <stdio.h>
#include <math.h>
#include "config.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

int main(void)
{
    printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);

    /* sqrt() lives in libm on many systems; AC_SEARCH_LIBS found the right -l flag. */
    printf("sqrt(2) = %f\n", sqrt(2.0));

#ifdef HAVE_UNISTD_H
    printf("unistd.h is available (pid = %ld)\n", (long)getpid());
#else
    printf("unistd.h not detected\n");
#endif

#ifdef ENABLE_DEBUG
    printf("[debug build]\n");
#endif
    return 0;
}
