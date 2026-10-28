# GDB Example 06 — Scripting: User Commands & Python Pretty-Printers

Teach GDB new tricks so repetitive inspection becomes one word. Covers
[gdb-guide §9](../../../gdb-guide.md#9-convenience-variables--expressions) and
[§17](../../../gdb-guide.md#17-scripting-gdbinit-hooks-and-python).

`main.c` builds a linked list — the classic structure that's tedious to inspect by chasing
`->next` by hand. This example fixes that two ways.

## Files

| File | What it adds |
|------|--------------|
| [commands.gdb](commands.gdb) | A user-defined `printlist` command (a GDB `while`-loop) + the demo |
| [prettyprint.py](prettyprint.py) | A Python pretty-printer so `print *list` shows the whole chain |

## Run the scripted demo

```bash
make demo
```

You'll see three levels of ergonomics on the same list:

1. **Raw GDB** — `print list->next->value`, chasing pointers manually. Painful past 2 nodes.
2. **User-defined command** — `printlist list` walks the entire list with a `while`-loop and
   prints every value. Defined once, reused forever.
3. **Python pretty-printer** — after `source prettyprint.py`, a plain `print *list` renders
   `Node list: 50 -> 40 -> 30 -> 20 -> 10` automatically.

## What to notice

- **`define … end`** creates a real command; `$arg0` is its argument and GDB has full
  `while`/`if` control flow and `$`-variables ([gdb-guide §9](../../../gdb-guide.md#9-convenience-variables--expressions)).
- **Pretty-printers** are how `print` becomes readable for *any* type — this is exactly the
  machinery behind nice `std::vector`/`std::string` output in libstdc++.
- Put commands like `printlist` in your **`~/.gdbinit`** to have them everywhere. Auto-load a
  project's Python printers via a `.gdbinit` in the project root (subject to
  `set auto-load safe-path`).

## Make it permanent

```bash
# ~/.gdbinit
set print pretty on
define printlist
  set $n = $arg0
  while $n != 0
    printf "  value = %d\n", $n->value
    set $n = $n->next
  end
end
```

## Now you try

```bash
make debug
```
```gdb
break report
run
source prettyprint.py
print *list                 # whole chain in one line
python print(gdb.parse_and_eval("list").dereference()['value'])   # Python API access
```

```bash
make clean
```
