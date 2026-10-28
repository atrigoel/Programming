/* Incremental linker script fragment.
 * `INSERT AFTER .data;` means: keep the linker's DEFAULT script intact and
 * just splice this extra output section in after .data. This is the safe way
 * to add a custom section on a hosted Linux target without replacing the
 * whole default script (which would break C runtime startup).
 *
 * Link with:  gcc -Wl,-T,extra.ld ...
 */
SECTIONS
{
    .my_config :
    {
        __my_config_start = .;    /* symbol = current address */
        KEEP(*(.my_config))       /* keep even if --gc-sections is used */
        __my_config_end = .;
    }
} INSERT AFTER .data;
