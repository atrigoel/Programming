# Linker Example 02 — Custom Section + Boundary Symbols

Places two globals into a custom `.my_config` section and lets the **linker script**
define `__my_config_start` / `__my_config_end` symbols around them — exactly the
technique firmware uses for config blocks, bootloader tables, and the vector table.

Runs on hosted Linux by **splicing** the section into the default script with
`INSERT AFTER .data` (so the C runtime still starts normally).

## Run

```bash
make            # builds ./custom with the custom section, prints analysis
make clean
```

## What to notice

- `__attribute__((section(".my_config")))` puts `device_id` and `fw_version` into a
  named section instead of `.data`.
- **`extra.ld`** gives that section a location and, crucially, defines the
  `__my_config_start`/`__my_config_end` **address symbols** — C declares them `extern`
  and walks the region as an array. This is the same idea as `_sdata`/`_edata` in a
  bare-metal script.
- **`KEEP(...)`** stops `--gc-sections` from discarding the block if nothing references
  it directly (relevant for hardware tables reached only by address).
- `objdump -s -j .my_config` dumps the raw bytes — you'll see `0xCAFEBABE` and
  `0x00010203` (byte order is little-endian on x86).
