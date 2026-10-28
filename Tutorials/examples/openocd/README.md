# OpenOCD Example — Flash & Debug a Cortex-M4

Config and GDB scripts referenced by [../../openocd-guide.md](../../openocd-guide.md).
They flash and debug the firmware built by the cross-compilation examples
([gcc/05-cross-embedded](../gcc/05-cross-embedded/README.md) or
[cmake/03-cross-cortex-m4](../cmake/03-cross-cortex-m4/README.md)).

| File | Purpose |
|------|---------|
| [`openocd.cfg`](openocd.cfg) | ST-Link + STM32F4 over SWD, with an optional SWO/ITM block |
| [`gdbinit`](gdbinit) | GDB script: connect, reset, `load`, break at `main` |

## Prerequisites

```bash
sudo apt-get install openocd gcc-arm-none-eabi
# Probe permissions (see the udev guide):
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger
```

Edit `openocd.cfg` to match your probe (`interface/`) and chip (`target/`).

## One-shot flash (no debugger)

```bash
openocd -f openocd.cfg -c "program /path/to/fw.elf verify reset exit"
```

## Interactive debug session

Terminal 1 — start OpenOCD and leave it running:

```bash
openocd -f openocd.cfg
```

Terminal 2 — launch GDB with the init script:

```bash
arm-none-eabi-gdb -x gdbinit /path/to/fw.elf
```

GDB connects, flashes, resets, and stops at `main`. From there:

```gdb
next                # step over
step                # step into
info registers      # dump core registers
print counter       # inspect a variable
watch counter       # DWT hardware watchpoint: break on write
monitor reset halt  # re-reset via OpenOCD
```

## Streaming ITM/SWO trace

1. Uncomment the `tpiu`/`itm` block in [`openocd.cfg`](openocd.cfg) and set
   `-traceclk` to your **real** CPU/HCLK frequency in Hz.
2. In firmware, retarget `printf` (or call `ITM_SendChar`) to ITM stimulus port 0.
3. Watch the decoded bytes:

```bash
tail -f swo.log
```

If the output is garbage, the `-traceclk` value doesn't match the actual core clock — fix
that first (it's the #1 SWO mistake).
