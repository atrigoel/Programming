# Make Example 05 — Automatic Header Dependencies

The modern idiom for correct C rebuilds: GCC's `-MMD -MP` generates `.d` files that
Make `-include`s, so editing **any** header rebuilds exactly the objects that use it.

## Run

```bash
make               # builds build/app and build/*.d dependency files
./build/app
cat build/main.d   # inspect the generated dependency: lists main.c + its headers

# Now prove it works:
touch src/config.h && make   # only main.o rebuilds (main.c includes config.h)
touch src/math_utils.h && make   # main.o AND math_utils.o rebuild

make clean
```

## What to notice

- Without `-MMD -MP`, changing a header would **not** trigger a rebuild — a classic
  source of stale-binary bugs. Here it Just Works.
- `-MMD` emits `build/x.d` containing `build/x.o: src/x.c headers...`.
- `-MP` adds phony targets for each header so deleting a header won't break the build
  with a "no rule to make target" error.
- `-include $(DEPS)` folds those rules in; the `-` prefix tolerates their absence on
  the first run.
