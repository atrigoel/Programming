# GCC Example 03 — Optimization & Debug Info

Build one source at `-O0`, `-O2`, `-Os`, and `-Og`, then compare code size and assembly.

## Run

```bash
make compare    # prints section sizes and assembly-line counts per level
make clean
```

Look at the generated assembly directly:

```bash
gcc -O0 -S main.c -o -   # verbose, 1:1 with source
gcc -O2 -S main.c -o -   # inlined square(), unrolled/folded loop
```

## What to notice

- **`size` output**: the `text` column (code) usually shrinks from `-O0` to `-O2`/`-Os`.
  `-Os` targets the smallest code — the level you want on a flash-constrained MCU.
- **`square()` disappears at `-O2`**: the optimizer inlines it, so there's no `call` in
  the assembly. At `-O0` you'll see an actual function call.
- **`-Og` for debugging**: unlike `-O2`, it keeps variables live and line info stable so
  GDB single-stepping makes sense, while still optimizing lightly.
- **`volatile n`**: without it the compiler could compute the whole result at compile
  time and the loop would vanish. `volatile` forces real runtime work to measure.
- The behavior of undefined-behavior bugs can *change* between `-O0` and `-O2`. If code
  "only works at `-O0`," suspect UB — build with `-fsanitize=undefined` to find it.
