# udev: Managing and Adding Devices on Linux

`udev` is the **device manager** for the modern Linux kernel. Every time you plug in a
USB debug probe, serial adapter, or dev board, udev is what creates the `/dev/...` node,
sets its permissions, and (optionally) gives it a stable name so your tools can find it.
For embedded work its main jobs are: **granting your user access to a probe without sudo**
and **giving a device a predictable name** even as `/dev/ttyUSB*` numbers shuffle around.

## Contents

1. [The Linux device model](#1-the-linux-device-model)
2. [How udev works](#2-how-udev-works)
3. [Inspecting devices with udevadm](#3-inspecting-devices-with-udevadm)
4. [Rule files: location & precedence](#4-rule-files-location--precedence)
5. [Rule syntax: match vs assignment](#5-rule-syntax-match-vs-assignment)
6. [Match keys](#6-match-keys)
7. [Assignment keys](#7-assignment-keys)
8. [Reloading & testing rules](#8-reloading--testing-rules)
9. [Worked example: a debug probe](#9-worked-example-a-debug-probe)
10. [Common recipes](#10-common-recipes)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. The Linux device model

Three pieces work together:

```
 hardware event ──► kernel ──► uevent ──► udevd ──► /dev node + symlinks + permissions
                      │
                      └── exposes device state under /sys (sysfs)
```

- **`/sys` (sysfs)** — a virtual filesystem where the kernel exposes every device and its
  attributes (vendor ID, product ID, serial number…). This is the *source of truth* udev
  matches against.
- **`/dev`** — the device nodes user programs open (e.g. `/dev/ttyACM0`). udev creates and
  removes these.
- **uevents** — messages the kernel broadcasts when devices appear/disappear.

A device sits in a **tree**: a USB serial adapter is a `tty` device whose *parent* is a
USB interface, whose parent is the USB device, and so on. Rules can match attributes from
the device itself **or any ancestor** — which is how you match a probe by its USB
vendor/product ID.

---

## 2. How udev works

1. You plug in a device. The kernel creates the sysfs entry and emits a **uevent**.
2. **`udevd`** (the daemon, part of systemd: `systemd-udevd`) receives the event.
3. udevd walks its **rules** in lexical order, matching each rule's conditions against the
   device and its parents.
4. Matching rules **act**: create the `/dev` node, add `SYMLINK` aliases, set
   `OWNER`/`GROUP`/`MODE`, set `TAG`s, and optionally `RUN` a helper program.
5. Programs like `udisksd`, `ModemManager`, or your flasher then use the resulting node.

The key mental model: **udev doesn't create devices — the kernel does. udev *names* them,
*permissions* them, and *reacts* to them.**

---

## 3. Inspecting devices with udevadm

Before writing a rule, discover what to match. `udevadm` is the swiss-army tool.

**Watch events live** (plug/unplug the device to see them):
```bash
udevadm monitor --udev --property
```

**Dump the full attribute walk** for a device node — this is the command you'll use most:
```bash
udevadm info --attribute-walk --name=/dev/ttyACM0
# or by sysfs path:
udevadm info -a -p $(udevadm info -q path -n /dev/ttyACM0)
```
It prints the device and each parent, listing every `ATTRS{...}`, `KERNELS`, `SUBSYSTEMS`,
and `DRIVERS` value you can match on. You typically copy `idVendor`, `idProduct`, and maybe
`serial` from the nearest USB parent.

**Quick vendor/product lookup:**
```bash
lsusb                      # e.g. "0483:374b STMicroelectronics ST-LINK/V2.1"
udevadm info -q property -n /dev/ttyACM0   # ID_VENDOR_ID, ID_MODEL_ID, ID_SERIAL, ...
```

---

## 4. Rule files: location & precedence

Rules are `*.rules` files read in **lexical order by filename**:

| Directory | Purpose |
|-----------|---------|
| `/lib/udev/rules.d/` (or `/usr/lib/udev/rules.d/`) | Shipped by packages — **don't edit** |
| `/etc/udev/rules.d/` | **Your** local rules — put custom rules here |
| `/run/udev/rules.d/` | Runtime-generated, volatile |

- Files are processed in order of their numeric prefix: `10-...` before `99-...`.
- A file in `/etc` **overrides** a same-named file in `/lib`.
- Convention: name yours like `99-stlink.rules`, `50-myboard.rules`. Later files can
  override earlier assignments, so higher numbers "win" for permissions.

---

## 5. Rule syntax: match vs assignment

Each rule is one line of comma-separated **key**–**operator**–**value** clauses. There are
two flavours of clause:

- **Match** clauses (operators `==`, `!=`) — conditions that must hold for the rule to fire.
- **Assignment** clauses (operators `=`, `+=`, `:=`) — actions applied when it matches.

```udev
# match keys ............................  assignment keys ..................
SUBSYSTEM=="tty", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0660", GROUP="plugdev", SYMLINK+="stlink"
```

| Operator | Meaning |
|----------|---------|
| `==` | Match: equal |
| `!=` | Match: not equal |
| `=`  | Assign a value (replaces) |
| `+=` | Append to a list (e.g. `SYMLINK`) |
| `:=` | Assign and **forbid later changes** |

A rule only fires if **all** its match clauses are true.

---

## 6. Match keys

| Key | Matches | Notes |
|-----|---------|-------|
| `ACTION` | `add`, `remove`, `change` | Gate on hotplug vs unplug |
| `KERNEL` | the device's own kernel name | e.g. `ttyUSB0` |
| `KERNELS` | the name of the device **or any parent** | |
| `SUBSYSTEM` | the device's subsystem | `tty`, `usb`, `block`, `hidraw` |
| `SUBSYSTEMS` | subsystem of device **or any parent** | |
| `DRIVER` / `DRIVERS` | the bound driver (self / any parent) | |
| `ATTR{file}` | a sysfs attribute of **this** device | exact device only |
| `ATTRS{file}` | a sysfs attribute of the device **or any parent** | **use this for USB IDs** |
| `ENV{key}` | a device property/env var | e.g. `ENV{ID_FS_TYPE}` |
| `TAG` | a tag set by an earlier rule | |

**Why `ATTRS` not `ATTR` for USB IDs:** `idVendor`/`idProduct` live on the USB *device*
node, but your rule usually matches the *tty* child. `ATTRS{}` searches up the parent
chain and finds them; `ATTR{}` (single device) would not.

---

## 7. Assignment keys

| Key | Effect |
|-----|--------|
| `NAME` | Set the `/dev` node name (rarely needed; prefer `SYMLINK`) |
| `SYMLINK+=` | Add a stable alias, e.g. `/dev/stlink` → the real node |
| `OWNER` | Owning user |
| `GROUP` | Owning group (e.g. `plugdev`, `dialout`) |
| `MODE` | Permission bits, e.g. `"0660"` |
| `ENV{key}=` | Set a property other rules/tools can read |
| `TAG+=` | Tag the device (e.g. `TAG+="uaccess"` for logged-in-user access) |
| `RUN+=` | Run a short program on the event (must be quick, non-blocking) |
| `GOTO` / `LABEL` | Jump within a rules file |
| `IMPORT{...}` | Import properties from a program/builtin |

Two ways to grant your user access to a probe:
- **Group + mode:** `GROUP="plugdev", MODE="0660"` and add yourself to `plugdev`.
- **Modern uaccess:** `TAG+="uaccess"` grants the *currently logged-in* user access via
  systemd-logind (no group needed). Many probe rules use this.

> **`RUN+=` is not for daemons.** It must return quickly; long-running or blocking commands
> stall event processing. To start a service, trigger a systemd unit instead.

---

## 8. Reloading & testing rules

After editing rules in `/etc/udev/rules.d/`:

```bash
sudo udevadm control --reload           # reload rule files into the running daemon
sudo udevadm trigger                    # re-apply rules to already-present devices
# (or just replug the device)
```

**Dry-run a rule against a device** without plugging anything:
```bash
udevadm test $(udevadm info -q path -n /dev/ttyACM0) 2>&1 | less
```
This prints which rules matched and the final `SYMLINK`, `MODE`, `GROUP`, etc. — the
fastest way to debug a rule that "isn't working."

---

## 9. Worked example: a debug probe

Goal: give an ST-Link/V2 a stable name `/dev/stlink` and let non-root use it.

**Step 1 — identify it:**
```bash
lsusb
# Bus 001 Device 007: ID 0483:374b STMicroelectronics ST-LINK/V2.1
```
Vendor `0483`, product `374b`.

**Step 2 — write the rule** `/etc/udev/rules.d/99-stlink.rules`:
```udev
# ST-Link/V2.1 debug probe: stable symlink + group access
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", \
    MODE="0660", GROUP="plugdev", TAG+="uaccess", SYMLINK+="stlink"

# Its virtual COM port (ttyACMx) -> stable /dev/stlink-serial
SUBSYSTEM=="tty", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", \
    MODE="0660", GROUP="dialout", SYMLINK+="stlink-serial"
```

**Step 3 — activate & verify:**
```bash
sudo udevadm control --reload
sudo udevadm trigger
ls -l /dev/stlink /dev/stlink-serial
```

Now OpenOCD/GDB can address the probe by a fixed name, and you don't need `sudo`.

---

## 10. Common recipes

**FTDI USB-serial adapter → predictable name by serial number** (so two adapters don't
swap `/dev/ttyUSB0`/`1`):
```udev
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", \
    ATTRS{serial}=="A5069RR4", SYMLINK+="ftdi-boardA"
```

**Grant serial access via the `dialout` group** (the standard for `/dev/ttyUSB*`,
`/dev/ttyACM*`):
```bash
sudo usermod -aG dialout $USER   # then log out and back in
```

**J-Link probe:**
```udev
SUBSYSTEM=="usb", ATTRS{idVendor}=="1366", MODE="0660", GROUP="plugdev", TAG+="uaccess"
```

**Run a script when a specific board is plugged in** (keep it fast):
```udev
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", \
    RUN+="/usr/local/bin/on-probe-attach.sh"
```

---

## 11. Troubleshooting

- **Rule seems ignored** → run `udevadm test` on the device path and read which rules
  matched. Check for typos in `idVendor`/`idProduct` (they're **lowercase hex**, no `0x`).
- **`ATTRS{idVendor}` empty** → you're matching the wrong node. Use
  `udevadm info -a -n /dev/...` and copy IDs from the correct USB parent.
- **Permissions still root-only** → did you `udevadm control --reload` *and* replug (or
  `udevadm trigger`)? Existing nodes keep old permissions until re-triggered.
- **Group membership not taking effect** → group changes need a fresh login (or `newgrp`).
- **Backslash line continuations** → make sure there's no trailing space after `\`.
- **Wrong directory** → custom rules go in `/etc/udev/rules.d/`, not `/lib/udev/rules.d/`.

Example rule files you can install are in
[examples/udev/](examples/udev/README.md).

---

## Further References
- `man udev`, `man udevadm`, `man 7 udev`
- systemd udev docs: <https://www.freedesktop.org/software/systemd/man/udev.html>
- Writing udev rules (Daniel Drake, classic tutorial): <http://www.reactivated.net/writing_udev_rules.html>
- The `60-*.rules` files in `/lib/udev/rules.d/` — real-world examples to learn from.
