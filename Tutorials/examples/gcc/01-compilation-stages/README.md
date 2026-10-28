# GCC Example 01 — The Compilation Pipeline

See the four stages `gcc` normally hides: preprocess → compile → assemble → link.

## Run

```bash
make stages     # generates main.i, main.s, main.o, main and prints them
make clean
```

Or drive each stage by hand:

```bash
gcc -E main.c -o main.i    # 1. preprocess  (macros/includes expanded)
gcc -S main.c -o main.s    # 2. compile     (assembly text)
gcc -c main.c -o main.o    # 3. assemble    (object file)
gcc    main.o -o main      # 4. link        (executable)
./main
```

## What to notice

- **`main.i`** is pure C with every `#include` pasted in and `GREETING` already replaced —
  the compiler proper never sees a `#define`.
- **`main.s`** is human-readable assembly for your host CPU. Try `gcc -S -O2 main.c` and
  diff it to see the optimizer at work.
- **`main.o`** is not runnable — it still needs `printf` resolved by the linker. Prove it
  with `nm main.o` (you'll see `U printf`, an *undefined* symbol).
- **`-save-temps`** does all of this in one shot: `gcc -save-temps main.c -o main` keeps
  `main.i`, `main.s`, and `main.o` alongside the executable.
