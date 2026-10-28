# udev Example Rules

Ready-to-install rule files referenced by [../../udev-guide.md](../../udev-guide.md).
These grant non-root access to common debug probes/serial adapters and give them stable
names. **Edit the vendor/product/serial values to match your hardware** (`lsusb`,
`udevadm info -a -n /dev/...`).

| File | Device |
|------|--------|
| [`99-stlink.rules`](99-stlink.rules) | ST-Link/V2 and V2.1 probes + their virtual COM port |
| [`60-ftdi-serial.rules`](60-ftdi-serial.rules) | FTDI USB-serial adapters, pinned by serial number |

## Install

```bash
sudo cp 99-stlink.rules 60-ftdi-serial.rules /etc/udev/rules.d/
sudo udevadm control --reload
sudo udevadm trigger
```

Add yourself to the groups the rules use, then re-login:

```bash
sudo usermod -aG plugdev,dialout $USER
```

## Verify

```bash
# Replug the device, then:
ls -l /dev/stlink /dev/stlink-serial          # symlinks created?
udevadm test $(udevadm info -q path -n /dev/ttyACM0)   # which rules matched?
```

If a rule doesn't fire, the `udevadm test` output shows exactly which match clause failed —
usually a wrong `idVendor`/`idProduct` (lowercase hex, no `0x`).
