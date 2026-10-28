# Make Example 02 — Variables & Automatic Variables

Two source files compiled to objects and linked, driven by variables.

## Run

```bash
make          # builds ./app
./app
make clean
```

## What to notice

- `CC`, `CFLAGS`, `OBJS`, `TARGET` centralize configuration. Override from the command
  line: `make CFLAGS="-g -O0"`.
- Automatic variables keep recipes generic: `$@` (target), `$<` (first prerequisite),
  `$^` (all prerequisites).
- Each `.o` lists `greet.h` as a prerequisite, so touching the header rebuilds the
  objects: `touch greet.h && make`.
- Change only `greet.c` and note that `main.o` is *not* recompiled — only `greet.o`
  and the final link.
