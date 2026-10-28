# Autoconf — A Detailed Guide

> **Autoconf** generates a portable `configure` shell script from a file called
> `configure.ac`. The `configure` script probes the user's system (compilers,
> headers, libraries, functions, sizes) and produces output files (like `Makefile`
> and a `config.h`) tailored to that system. It is the first pillar of the **GNU
> Autotools** (Autoconf + Automake + Libtool). Runnable examples live in
> [`examples/autoconf/`](examples/autoconf/).

---

## Table of Contents

1. [The Problem Autoconf Solves](#1-the-problem)
2. [The Autotools Pipeline](#2-the-pipeline)
3. [M4 and the `configure.ac` Language](#3-m4-and-configure-ac)
4. [A Minimal `configure.ac` Explained](#4-a-minimal-configure-ac)
5. [Essential Macros](#5-essential-macros)
6. [Checking for Programs, Headers, Libraries, Functions](#6-checks)
7. [config.h and Feature Macros](#7-config-h)
8. [Substitutions and `Makefile.in`](#8-substitutions)
9. [The User's View: `./configure` Options](#9-configure-options)
10. [`--with` / `--enable` Custom Options](#10-with-and-enable)
11. [The Generated Files and What to Commit](#11-generated-files)
12. [Autoconf Without Automake](#12-without-automake)
13. [Common Pitfalls](#13-common-pitfalls)
14. [The Runnable Examples](#14-the-runnable-examples)
15. [Further References](#15-further-references)

---

## 1. The Problem

C/C++ code that must build on many Unix-like systems faces countless differences:
Is `<stdint.h>` present? Is the function `strlcpy` available? Is `long` 4 or 8 bytes?
Which library holds `sqrt` — libc or `-lm`? Hard-coding answers breaks portability.

Autoconf's philosophy: **don't test the OS name, test for the actual feature.** The
generated `configure` script *runs small compile/link tests* on the target machine
and records the results, so your code adapts to what is really there.

---

## 2. The Autotools Pipeline

```
   configure.ac  ─┐
                  ├─▶  autoconf  ─▶  configure  ─┐
   aclocal.m4  ───┘                              │
                                                 ▼
   Makefile.am ─▶ automake ─▶ Makefile.in ─▶ ./configure ─▶ Makefile + config.h
                                                 ▲
   configure.ac (AC_CONFIG_HEADERS) ─▶ autoheader ─▶ config.h.in
```

- **`autoreconf -i`** runs the whole chain in the right order (recommended).
- **You (the developer)** write `configure.ac` (+ `Makefile.am` if using Automake).
- **The user** just runs `./configure && make && make install` — no autotools needed.

---

## 3. M4 and configure.ac

`configure.ac` is written in **M4**, a macro language. Autoconf provides macros
(prefixed `AC_`, and `AM_` for Automake, `AS_` for shell helpers). Key syntax rules:

- Macro arguments are separated by commas: `AC_INIT([name], [1.0])`.
- Arguments are **quoted with square brackets** `[ ]` (M4 quote characters) to protect
  them from macro expansion. **Always quote** — this avoids subtle bugs.
- Whitespace after a comma before an argument is significant in some macros; the
  bracket-quoting convention keeps things predictable.

You rarely write raw M4 — you compose existing `AC_*` macros.

---

## 4. A Minimal configure.ac Explained

```m4
AC_INIT([hello], [1.0], [bugs@example.com])   # package name, version, contact
AC_CONFIG_SRCDIR([src/main.c])                # sanity check: a file that must exist
AC_CONFIG_HEADERS([config.h])                 # generate config.h from config.h.in
AC_PROG_CC                                     # find a C compiler -> sets $CC, $CFLAGS
AC_CONFIG_FILES([Makefile])                    # files to generate from *.in templates
AC_OUTPUT                                       # do it
```

- **`AC_INIT`** must come first; it defines `PACKAGE_NAME`, `PACKAGE_VERSION`, etc.
- **`AC_CONFIG_SRCDIR`** guards against running configure in the wrong directory.
- **`AC_CONFIG_HEADERS`** tells autoheader to make `config.h.in` and configure to fill
  `config.h`.
- **`AC_PROG_CC`** locates a working C compiler and sets output variables.
- **`AC_CONFIG_FILES`** lists template files (`Makefile.in` → `Makefile`).
- **`AC_OUTPUT`** writes everything out. Must come last.

---

## 5. Essential Macros

| Macro | Purpose |
|-------|---------|
| `AC_INIT` / `AC_OUTPUT` | bookends of every `configure.ac` |
| `AC_PREREQ([2.69])` | require a minimum autoconf version |
| `AC_CONFIG_AUX_DIR([build-aux])` | where helper scripts (`install-sh`) live |
| `AC_CONFIG_MACRO_DIRS([m4])` | where extra `.m4` macros live |
| `AC_PROG_CC` / `AC_PROG_CXX` | detect C / C++ compiler |
| `AC_PROG_INSTALL` / `AC_PROG_RANLIB` | install program, static-lib indexer |
| `AC_PROG_MKDIR_P` | portable `mkdir -p` |
| `AC_CONFIG_HEADERS([config.h])` | produce a config header |
| `AC_CONFIG_FILES([Makefile src/Makefile])` | templates to instantiate |
| `AM_INIT_AUTOMAKE` | hook Automake in (see the Automake guide) |

---

## 6. Checks

The heart of Autoconf. Each check records a result you can use in `config.h` or
`Makefile`.

**Programs:**
```m4
AC_PROG_CC
AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])
AC_PATH_PROG([PERL], [perl])       # absolute path into $PERL
```

**Header files:**
```m4
AC_CHECK_HEADERS([stdint.h unistd.h sys/time.h])
# -> defines HAVE_STDINT_H etc. in config.h if present
```

**Library + function in it:**
```m4
AC_SEARCH_LIBS([sqrt], [m])        # adds -lm to LIBS if needed for sqrt
AC_CHECK_LIB([z], [inflate])       # defines HAVE_LIBZ, adds -lz
```

**Library functions:**
```m4
AC_CHECK_FUNCS([strdup strlcpy getline])
# -> HAVE_STRDUP, HAVE_STRLCPY, HAVE_GETLINE
```

**Types and sizes:**
```m4
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_SIZEOF([long])            # -> SIZEOF_LONG
AC_C_BIGENDIAN                     # -> WORDS_BIGENDIAN
```

**Custom compile/link/run test:**
```m4
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <foo.h>]], [[foo();]])],
                  [have_foo=yes], [have_foo=no])
```

---

## 7. config.h and Feature Macros

When you use `AC_CONFIG_HEADERS`, `autoheader` scans your checks and generates
`config.h.in` with `#undef` lines. `configure` turns each into a `#define` (or leaves
it undefined). Your code then does:

```c
#include "config.h"

#ifdef HAVE_STRLCPY
    strlcpy(dst, src, n);
#else
    my_portable_strlcpy(dst, src, n);
#endif
```

This is far cleaner than passing dozens of `-D` flags on the compile line, and it puts
all detected settings in one inspectable file.

---

## 8. Substitutions and Makefile.in

Any shell variable can be exported into template files as `@VAR@` using
`AC_SUBST([VAR])`. Autoconf already substitutes common ones (`@CC@`, `@CFLAGS@`,
`@LIBS@`, `@prefix@`, `@PACKAGE_VERSION@`, ...).

`Makefile.in` (a template) contains:
```make
CC = @CC@
CFLAGS = @CFLAGS@
LIBS = @LIBS@
prefix = @prefix@
```
`configure` copies it to `Makefile`, replacing each `@...@`. When you use **Automake**,
you write `Makefile.am` instead and Automake generates `Makefile.in` for you.

Custom substitution:
```m4
AC_SUBST([MYDATADIR], ["\${prefix}/share/myapp"])
```

---

## 9. The User's View: ./configure Options

The generated `configure` gives users a standard interface:

```bash
./configure --help                 # list all options
./configure --prefix=/opt/myapp    # install root (default /usr/local)
./configure CC=clang CFLAGS="-O2"  # override detected tools/flags
./configure --host=arm-linux-gnueabihf   # cross-compile
make
make install DESTDIR=/tmp/stage    # staged install
```

Standard directory variables (`--prefix`, `--bindir`, `--libdir`, `--sysconfdir`, ...)
all come for free.

**Cross-compiling:** `--build` (where you compile), `--host` (where it runs),
`--target` (what a compiler you're building emits). Setting `--host` triggers
cross-compilation mode, where run-time tests are skipped (configure can't run target
binaries).

---

## 10. --with and --enable Custom Options

Two macros let you add project-specific switches:

```m4
# Feature toggle: --enable-debug / --disable-debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [build with debug info])],
  [debug=$enableval], [debug=no])
AS_IF([test "x$debug" = "xyes"], [CFLAGS="$CFLAGS -g -O0"])

# External package: --with-foo=PATH / --without-foo
AC_ARG_WITH([openssl],
  [AS_HELP_STRING([--with-openssl=DIR], [use OpenSSL from DIR])],
  [openssl_dir=$withval], [openssl_dir=])
```

Convention: **`--enable/--disable`** for optional *features* of your package;
**`--with/--without`** for optional *external dependencies*.

---

## 11. The Generated Files and What to Commit

Autotools produces many files. General guidance:

- **Commit to VCS:** `configure.ac`, `Makefile.am`, `m4/*.m4` you wrote, and any
  hand-written sources.
- **Do NOT commit (regenerated):** `configure`, `Makefile.in`, `config.h.in`,
  `aclocal.m4`, `autom4te.cache/`, `build-aux/` helper scripts, `config.status`,
  `config.log`, `Makefile`, `config.h`.
- **For release tarballs** (`make dist`): the generated `configure` etc. **are**
  included so end users don't need autotools installed.
- Provide an **`autogen.sh`** (or tell users to run `autoreconf -i`) to bootstrap from
  a fresh checkout.

A typical `.gitignore` excludes the generated set above.

---

## 12. Autoconf Without Automake

You can use Autoconf alone with a hand-written `Makefile.in`. This is the setup in
example `01-configure-only` — it shows the pure Autoconf mechanics (substitutions,
`config.h`, checks) without Automake's extra layer. In practice most projects add
Automake (see the Automake guide) to avoid writing `Makefile.in` by hand.

---

## 13. Common Pitfalls

- **Forgetting brackets** around macro arguments → M4 mis-expansion. Always
  `AC_INIT([pkg], [1.0])`, not `AC_INIT(pkg, 1.0)`.
- Editing generated `configure` or `Makefile.in` by hand — changes vanish on
  regeneration. Edit `configure.ac` / `Makefile.am` instead.
- Running `configure` in the source dir but expecting VPATH/out-of-tree behavior —
  create a separate `build/` dir and run `../configure` from there.
- Not re-running `autoreconf` after editing `configure.ac`.
- Testing `uname`/OS names instead of features — defeats the purpose.
- Reading `config.log` is the key to debugging *why* a check failed; it records the
  exact test program and compiler error.

---

## 14. The Runnable Examples

Under [`examples/autoconf/`](examples/autoconf/). These need `autoconf` installed
(`sudo apt-get install autoconf`; add `automake libtool` for the Automake guide).

| Folder | Teaches |
|--------|---------|
| `01-configure-only` | Autoconf alone: `configure.ac` + hand-written `Makefile.in`, `config.h`, `AC_CHECK_HEADERS`, `AC_SEARCH_LIBS`, `--enable-debug` |

Run:
```bash
cd examples/autoconf/01-configure-only
autoreconf -i        # generate configure, config.h.in
./configure          # probe system, write Makefile + config.h
make
./hello
./configure --enable-debug && make   # rebuild with -g -O0
make clean
```

Inspect `config.h` and `config.log` afterward to see what was detected.

---

## 15. Further References

- **Autoconf Manual** (GNU): https://www.gnu.org/software/autoconf/manual/ — the
  authoritative reference; skim "Writing configure.ac" and "Existing Tests".
- **"GNU Autotools" / Autobook** — *Autoconf, Automake, and Libtool* by Vaughan,
  Elliston, Tromey, Taylor (the "Goat book"): https://www.sourceware.org/autobook/
- **Autotools Mythbuster** (John Calcote / Diego Pettenò): practical, modern advice:
  https://autotools.io/
- **"Autotools" by John Calcote** (No Starch Press) — the best full-length book.
- `info autoconf`, and always read **`config.log`** when a check misbehaves.
