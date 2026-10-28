# GCC Example 04 — Preprocessor Defines, Include Paths & Linking

Combines the flags that wire a multi-file project together: `-I`, `-D`, `-L`, `-l`, static
library creation with `ar`, and `-MMD` auto-dependencies.

## Layout

```
04-preprocessor-linking/
├── include/greet.h     # header on the -I path
├── lib/greet.c         # becomes libgreet.a
├── main.c              # links against the library
└── Makefile
```

## Run

```bash
make run                                   # default build
make clean && make run CPPFLAGS='-DENABLE_EXTRA -DVERSION=\"2.0\"'
make clean
```

## What to notice

- **`-Iinclude`** lets `main.c` and `greet.c` find `greet.h` without a relative path.
- **`-D` at the command line** flips code on/off: the default build prints "Extra feature
  OFF"; rebuild with `-DENABLE_EXTRA` to switch the `#ifdef` block on. This is exactly how
  build systems pass `-DSTM32F407xx`, `-DUSE_HAL_DRIVER`, etc.
- **Static library flow**: `gcc -c` → `ar rcs libgreet.a greet.o` → link with
  **`-L.` `-lgreet`**. `-L` adds a search dir; `-lgreet` resolves to `libgreet.a`.
- **Link order matters**: `main.o` comes *before* `-lgreet`. Put the library first and the
  linker may report `undefined reference to 'greet'`.
- **`-MMD`** drops a `.d` file next to each object listing the headers it used; the
  `-include main.d greet.d` line makes edits to `greet.h` trigger a rebuild automatically.
