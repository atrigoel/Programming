#include <stdio.h>

/* .rodata : string literals and const data live here (read-only). */
const char *message = "linker sections explorer";
const int  read_only_answer = 42;

/* .data : initialized read/write global -> stored in image, runs in RAM. */
int initialized_global = 12345;

/* .bss : zero-initialized global -> occupies RAM only, not the file image. */
int zeroed_global;
char big_buffer[4096];      /* also .bss: notice file size barely grows */

int main(void)
{
    printf("%s\n", message);
    printf("read_only_answer   = %d\n", read_only_answer);
    printf("initialized_global = %d\n", initialized_global);
    printf("zeroed_global      = %d\n", zeroed_global);
    printf("big_buffer[0]      = %d\n", big_buffer[0]);
    return 0;
}
