# Make Example 03 — Pattern Rules

A single `%.o: %.c` pattern rule replaces one-rule-per-file boilerplate.

## Run

```bash
make          # builds ./calc using the pattern rule for each object
./calc
make clean
```

## What to notice

- The `%.o: %.c` rule matches both `main.o` and `ops.o`. The `$*` stem echoes which
  file matched.
- Adding a new `.c` to `OBJS` needs **no new rule** — the pattern covers it.
- Remove your explicit compile rule entirely and Make's **built-in** implicit rule
  still compiles `.c` → `.o`; try it to see implicit rules in action (`make -p` lists
  them).
