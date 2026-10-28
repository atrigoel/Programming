# Make Example 01 — Hello

The smallest useful Makefile: one source file, one target.

## Run

```bash
make        # compiles hello.c into ./hello
./hello     # prints the greeting
make clean  # deletes the ./hello binary
```

## What to notice

- The recipe lines are indented with a **TAB**, not spaces.
- The first target (`all`) is the default goal.
- `make` a second time prints "up to date" — nothing recompiles because `hello` is
  newer than `hello.c`. Edit `hello.c`, run `make` again, and it rebuilds.
- Try `make -n` to see the commands without running them.
