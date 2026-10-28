# CMake Example 01 — Hello (host)

The absolute minimum: understand the **configure → build** two-phase model on your
own PC before touching cross-compilation.

## Run

```bash
cmake -B build          # CONFIGURE: read CMakeLists.txt, generate build files in build/
cmake --build build     # BUILD:     actually compile
./build/hello           # run it
```

Clean up with `rm -rf build` (everything CMake generates lives there — the source tree
stays untouched, which is the whole point of *out-of-source* builds).

## What to notice

- **Two phases.** The first command generates a build system; the second runs it. Run
  `cmake --build build` again — nothing recompiles because nothing changed.
- **Generator choice.** Add `-G Ninja` to the configure line to emit Ninja files
  instead of Makefiles (`cmake -B build -G Ninja`). The rest is identical — that
  portability is CMake's job.
- **Scoped flags.** `-Wall -Wextra` were attached with `target_compile_options(... PRIVATE ...)`
  rather than a global variable, so they apply to this target and nothing else.
- Inspect what CMake decided: `cmake -B build -LH` lists cache variables with help text.
