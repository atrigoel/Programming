# CMake Example 02 — Library & Usage-Requirement Propagation

The single most important CMake skill for scaling a project: let targets carry their
own **usage requirements** so consumers inherit them automatically.

## Layout

```
02-library-propagation/
├── CMakeLists.txt          # top level: builds app, links mathx
├── src/main.c              # the application
└── mathx/
    ├── CMakeLists.txt      # builds the mathx static library
    ├── include/mathx.h     # PUBLIC header
    └── src/mathx.c
```

## Run

```bash
cmake -B build
cmake --build build
./build/app
```

## What to notice

- **`app` never names `mathx/include`.** It only does `target_link_libraries(app PRIVATE mathx)`.
  Because `mathx` marked its include directory **PUBLIC**, the path propagates to every
  consumer. This is the core idea behind modern "target-based" CMake.
- **PRIVATE vs PUBLIC vs INTERFACE:**
  - `PRIVATE` — used to build *this* target, not passed on. (mathx's warning flags.)
  - `INTERFACE` — *not* used to build this target, only passed to consumers. (Typical
    for header-only libraries.)
  - `PUBLIC` — both. (mathx's include directory.)
- Try breaking it: change `PUBLIC` to `PRIVATE` in [mathx/CMakeLists.txt](mathx/CMakeLists.txt#L9)
  and rebuild — `app` now fails to find `mathx.h`, proving the propagation was doing the work.
