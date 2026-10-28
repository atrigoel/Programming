# GNU Make — A Detailed Guide

> `make` is a build automation tool that decides **what** to rebuild and **when**,
> based on file timestamps and a dependency graph you describe in a **Makefile**.
> This guide targets **GNU Make** (the `make` on Linux). Companion runnable examples
> live in [`examples/make/`](examples/make/).

---

## Table of Contents

1. [Why Make Exists](#1-why-make-exists)
2. [The Core Idea: Targets, Prerequisites, Recipes](#2-the-core-idea)
3. [How Make Decides What to Rebuild](#3-how-make-decides)
4. [Anatomy of a Rule](#4-anatomy-of-a-rule)
5. [Variables](#5-variables)
6. [Automatic Variables](#6-automatic-variables)
7. [Pattern Rules and Implicit Rules](#7-pattern-rules)
8. [Phony Targets](#8-phony-targets)
9. [Functions](#9-functions)
10. [Conditionals](#10-conditionals)
11. [Automatic Dependency Generation](#11-auto-dependencies)
12. [Recursive vs Non-Recursive Make](#12-recursive-make)
13. [Common Built-in Variables and Conventions](#13-conventions)
14. [Parallelism and Debugging](#14-parallelism-and-debugging)
15. [Common Pitfalls](#15-common-pitfalls)
16. [The Runnable Examples](#16-the-runnable-examples)
17. [Further References](#17-further-references)

---

## 1. Why Make Exists

Compiling a large program by hand is slow and error-prone. If one `.c` file changes,
you only want to recompile *that* file and relink — not rebuild everything. Make
encodes the **dependency graph** of your project and uses **file modification times**
to rebuild only what is stale. It is language-agnostic: anything expressible as
"output depends on inputs, produced by a command" fits Make.

---

## 2. The Core Idea

A Makefile is a list of **rules**. Each rule has three parts:

```make
target: prerequisites
	recipe        # <-- MUST be indented with a TAB, not spaces
```

- **target** — usually a file to create (or a phony name like `clean`).
- **prerequisites** — files the target depends on.
- **recipe** — shell commands that build the target. **Each recipe line must begin
  with a literal TAB character.** This is the #1 beginner error.

Example:

```make
hello: hello.c
	gcc -o hello hello.c
```

"To make `hello`, I need `hello.c`; if `hello` is missing or older than `hello.c`,
run `gcc`."

---

## 3. How Make Decides

Make builds a **directed acyclic graph** of targets and prerequisites, then walks it:

1. To build a target, first (recursively) ensure all prerequisites are up to date.
2. Compare timestamps: if any prerequisite is **newer** than the target (or the target
   doesn't exist), the recipe runs.
3. Otherwise the target is "up to date" and the recipe is skipped.

```
       program
      /        \
  main.o      util.o
    |             |
  main.c       util.c  util.h
```

Change `util.h` → `util.o` is stale → relink `program`. `main.o` is untouched if it
doesn't include `util.h`.

---

## 4. Anatomy of a Rule

```make
program: main.o util.o        # target : prerequisites
	gcc -o program main.o util.o   # recipe (TAB-indented)
```

- The **first target** in the file is the **default goal** (built when you run `make`
  with no arguments). Convention: name it `all`.
- A target with no prerequisites still runs its recipe if the target file is missing.
- Recipe lines each run in their **own subshell** unless joined with `\` or
  `.ONESHELL:`.
- Prefix a recipe line with:
  - `@` to **not echo** the command.
  - `-` to **ignore errors** from that line.
  - `+` to run even under `make -n` (dry run).

---

## 5. Variables

Variables reduce repetition. Two main flavors:

```make
CC      = gcc            # recursively expanded (evaluated when used)
CFLAGS := -Wall -O2      # simply expanded (evaluated once, now)
CFLAGS += -g             # append
LDFLAGS ?= -lm           # assign only if not already set
```

| Operator | Name                | Behavior                                            |
|----------|---------------------|-----------------------------------------------------|
| `=`      | recursive           | RHS re-evaluated every time the var is used         |
| `:=`     | simple              | RHS evaluated once at definition                    |
| `?=`     | conditional         | assign only if variable is currently undefined      |
| `+=`     | append              | append (keeps flavor of the original)               |

Use them: `$(CC) $(CFLAGS) -c file.c`. Single-letter vars can use `$X`, but always
prefer `$(NAME)` for clarity.

**Override precedence:** command-line (`make CC=clang`) > makefile > environment.

---

## 6. Automatic Variables

Set by Make **per rule**, these make recipes generic:

| Variable | Meaning                                                        |
|----------|----------------------------------------------------------------|
| `$@`     | the target name                                                |
| `$<`     | the **first** prerequisite                                     |
| `$^`     | **all** prerequisites (duplicates removed)                     |
| `$+`     | all prerequisites (duplicates kept, order preserved)           |
| `$?`     | prerequisites **newer** than the target                        |
| `$*`     | the **stem** matched by a pattern rule (`%`)                    |
| `$(@D)`  | directory part of `$@`; `$(@F)` the file part                  |

Example:

```make
program: main.o util.o
	$(CC) $(LDFLAGS) -o $@ $^     # $@ = program, $^ = main.o util.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@   # $< = the .c file, $@ = the .o file
```

---

## 7. Pattern Rules and Implicit Rules

A **pattern rule** uses `%` as a wildcard stem to describe a whole class of files:

```make
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

This says "any `X.o` can be built from `X.c`." Make matches the stem `X` and applies
it to both sides.

Make also ships **built-in implicit rules** (e.g., it already knows how to make `.o`
from `.c` using `$(CC)`, `$(CFLAGS)`, `$(CPPFLAGS)`). You can often omit the compile
rule entirely and just set the variables — but writing it explicitly is clearer for
learning.

**Static pattern rules** restrict a pattern to a specific list:

```make
OBJS = a.o b.o c.o
$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

---

## 8. Phony Targets

A **phony target** is a rule name that is *not* a real file — it names an action:

```make
.PHONY: all clean install

all: program

clean:
	rm -f *.o program

install: program
	install -m 755 program /usr/local/bin/
```

Declaring `.PHONY` (a) prevents confusion if a file named `clean` ever exists, and
(b) lets Make skip the timestamp check and always run the recipe.

---

## 9. Functions

Make has text-processing functions, invoked `$(function args)`:

```make
SRCS := $(wildcard src/*.c)              # find files
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))  # transform names
OBJS := $(SRCS:.c=.o)                    # shorthand substitution
DIRS := $(sort $(dir $(OBJS)))           # dedupe + directory parts
UPPER := $(shell echo hi | tr a-z A-Z)   # run a shell command
```

Handy ones: `wildcard`, `patsubst`, `subst`, `filter`, `filter-out`, `sort`, `dir`,
`notdir`, `basename`, `suffix`, `addprefix`, `addsuffix`, `foreach`, `if`, `call`,
`shell`, `abspath`, `realpath`.

`$(call name,arg1,arg2)` invokes a user-defined function:

```make
define compile
	$(CC) $(CFLAGS) -c $(1) -o $(2)
endef
# usage: $(call compile,foo.c,foo.o)
```

---

## 10. Conditionals

```make
ifeq ($(DEBUG),1)
  CFLAGS += -g -O0
else
  CFLAGS += -O2
endif

ifdef VERBOSE
  Q =
else
  Q = @
endif
```

Directives: `ifeq`, `ifneq`, `ifdef`, `ifndef`. Note these are **Make** conditionals
evaluated at parse time — different from shell `if` inside a recipe.

---

## 11. Automatic Dependency Generation

The hard part of C builds: a `.o` depends not just on its `.c` but on every header it
`#include`s. Maintaining that by hand is hopeless. GCC can emit the dependency list:

```make
CFLAGS += -MMD -MP
-include $(OBJS:.o=.d)
```

- `-MMD` writes a `foo.d` file containing `foo.o: foo.c bar.h ...` alongside each
  object.
- `-MP` adds a phony target for each header so deleting a header doesn't break the
  build.
- `-include` pulls those `.d` files in (the `-` ignores them on the first build when
  they don't exist yet).

This is the standard modern idiom and is demonstrated in example 05.

---

## 12. Recursive vs Non-Recursive Make

**Recursive** — a top Makefile invokes `make` in subdirectories:

```make
SUBDIRS = lib app
.PHONY: $(SUBDIRS)
all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
```

Simple to start, but each sub-make sees only part of the graph, which can cause
incorrect/slow rebuilds ("Recursive Make Considered Harmful", see references).

**Non-recursive** — one Make process sees the whole project by `include`-ing
fragment makefiles from each directory. Faster and more correct, but more advanced.

Use `$(MAKE)` (not literal `make`) so flags like `-j` propagate to sub-makes.

---

## 13. Conventions

GNU coding standards define expected variables and targets so users get a predictable
interface:

- Programs: `CC`, `CXX`, `AR`, `RANLIB`, `INSTALL`.
- Flags: `CFLAGS`, `CXXFLAGS`, `CPPFLAGS` (preprocessor), `LDFLAGS`, `LDLIBS`.
- Install dirs: `prefix` (default `/usr/local`), `bindir`, `libdir`, `includedir`,
  `DESTDIR` (staged installs).
- Standard phony targets: `all`, `clean`, `distclean`, `install`, `uninstall`,
  `check`, `dist`.

Following these makes your Makefile behave like the ones Autotools generates.

---

## 14. Parallelism and Debugging

- **`make -j$(nproc)`** — build in parallel using all CPU cores. Correct prerequisites
  are essential; missing dependencies cause race conditions that only show under `-j`.
- **`make -n`** (dry run) — print recipes without executing.
- **`make -p`** — dump the entire database (rules, variables, implicit rules).
- **`make -d`** — verbose debug of Make's decision process.
- **`make --trace`** — show why each target is being rebuilt.
- **`$(info ...)`, `$(warning ...)`, `$(error ...)`** — print during parsing.
- **`make -B`** — force rebuild everything (ignore timestamps).

---

## 15. Common Pitfalls

- **Spaces instead of a TAB** before a recipe → `*** missing separator` error.
- **Missing header dependencies** → stale objects; fix with `-MMD -MP`.
- **Trailing whitespace** in variable values (e.g., `DIR = build ` with a space).
- Assuming recipe lines share state — each line is a **separate shell**; use `\` or
  `.ONESHELL:` to keep `cd` effective.
- Using `=` when you meant `:=`, causing expensive re-evaluation or `$(shell ...)`
  running many times.
- Forgetting `.PHONY`, so a real file named `clean`/`test` shadows the target.
- Relying on rule order for parallel safety instead of real prerequisites.

---

## 16. The Runnable Examples

All under [`examples/make/`](examples/make/). On a Linux host with `gcc` and `make`:

| Folder | Teaches |
|--------|---------|
| `01-hello` | Minimal single-file build; `make`, `make clean` |
| `02-variables` | `CC/CFLAGS`, automatic variables `$@ $< $^` |
| `03-pattern-rules` | `%.o: %.c` pattern rule, multiple objects |
| `04-phony-and-functions` | `.PHONY`, `wildcard`, `patsubst`, install/clean |
| `05-auto-deps` | Header tracking with `-MMD -MP` and `.d` files |

Run each with:

```bash
cd examples/make/01-hello
make            # build
./hello         # run
make clean      # remove artifacts
```

Try `make -n` to preview, and edit a header in `05-auto-deps` to watch only the
dependent objects rebuild.

---

## 17. Further References

- **GNU Make Manual** — the authoritative reference: https://www.gnu.org/software/make/manual/
  (also `info make` locally). Read chapters on Rules, Variables, and Functions first.
- **Managing Projects with GNU Make**, Robert Mecklenburg (O'Reilly) — the best book.
- **"Recursive Make Considered Harmful"**, Peter Miller — classic paper on why
  non-recursive layouts scale better.
- **GNU Coding Standards → Makefile Conventions** — for standard targets/variables:
  https://www.gnu.org/prep/standards/html_node/Makefile-Conventions.html
- `man make`, and `make -p` to explore the built-in rule database.

**Practice ideas:** convert a multi-file C project to use pattern rules + auto-deps;
add a `DEBUG=1` conditional build; parallelize with `-j` and fix any missing deps that
surface.
