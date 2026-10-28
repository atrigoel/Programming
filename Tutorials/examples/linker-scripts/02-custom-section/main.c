#include <stdio.h>

/* Force this variable into a custom section named ".my_config".
 * The linker-script fragment (extra.ld) gives that section a home and
 * defines boundary symbols around it. */
__attribute__((section(".my_config"), used))
volatile unsigned int device_id = 0xCAFEBABE;

__attribute__((section(".my_config"), used))
volatile unsigned int fw_version = 0x00010203;

/* These symbols are DEFINED BY THE LINKER SCRIPT (extra.ld), not by C. */
extern unsigned int __my_config_start;
extern unsigned int __my_config_end;

int main(void)
{
    unsigned int *p   = &__my_config_start;
    unsigned int *end = &__my_config_end;

    printf("device_id  = 0x%08X\n", device_id);
    printf("fw_version = 0x%08X\n", fw_version);
    printf(".my_config spans %ld bytes at %p .. %p\n",
           (long)((char *)end - (char *)p), (void *)p, (void *)end);

    printf("raw words in .my_config:\n");
    for (; p < end; p++)
        printf("  0x%08X\n", *p);
    return 0;
}
