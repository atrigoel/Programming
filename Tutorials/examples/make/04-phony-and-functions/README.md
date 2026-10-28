# Make Example 04 — Phony Targets & Functions

Out-of-tree build (`src/` → `build/`) using `wildcard`/`patsubst` and standard phony
targets.

## Run

```bash
make          # compiles everything into build/, links build/demo
make run      # builds if needed, then runs
make clean    # removes the whole build/ directory

# staged install (no root needed):
make install DESTDIR=$PWD/stage prefix=/usr
find stage    # see stage/usr/bin/demo
```

## What to notice

- `SRCS := $(wildcard src/*.c)` discovers sources; `patsubst` maps them to
  `build/*.o`. Add a new `.c` under `src/` and it is picked up automatically.
- The **order-only prerequisite** `| $(BUILDDIR)` ensures `build/` exists without
  making objects depend on its timestamp.
- `.PHONY` marks `all clean install uninstall run` as actions, not files.
- `DESTDIR` + `prefix`/`bindir` follow GNU install conventions used by Autotools.
