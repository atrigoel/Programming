# Automake — A Detailed Guide

> **Automake** generates complete, standards-compliant `Makefile.in` templates from a
> short, high-level `Makefile.am`. Combined with Autoconf, you describe *what* to build
> in a few lines and get a Makefile that supports `all`, `install`, `uninstall`,
> `clean`, `distclean`, `check`, `dist`, VPATH/out-of-tree builds, dependency tracking,
> and cross-compilation — for free. Runnable examples live in
> [`examples/automake/`](examples/automake/).

---

## Table of Contents

1. [Why Automake](#1-why-automake)
2. [Where Automake Fits](#2-where-it-fits)
3. [Primaries and Prefixes: the Naming System](#3-primaries-and-prefixes)
4. [A Minimal Project](#4-a-minimal-project)
5. [Programs, Sources, Flags](#5-programs-sources-flags)
6. [Libraries: Static and Convenience](#6-libraries)
7. [Headers, Data, Scripts, Man Pages](#7-headers-data-scripts)
8. [Multiple Directories (SUBDIRS)](#8-subdirs)
9. [Tests with `make check`](#9-tests)
10. [`make dist` and Distribution](#10-make-dist)
11. [Conditionals and Options](#11-conditionals)
12. [Silent Rules and Dependency Tracking](#12-silent-rules)
13. [What to Commit](#13-what-to-commit)
14. [Common Pitfalls](#14-common-pitfalls)
15. [The Runnable Examples](#15-the-runnable-examples)
16. [Further References](#16-further-references)

---

## 1. Why Automake

Writing a portable, feature-complete `Makefile.in` by hand (like the Autoconf-only
example) is tedious and error-prone: install/uninstall rules, staged installs
(`DESTDIR`), out-of-tree builds, `make dist` tarballs, header dependency tracking,
`make check`... Automake generates all of this from declarations such as:

```makefile
bin_PROGRAMS = hello
hello_SOURCES = main.c util.c util.h
```

That's it — Automake expands it into hundreds of lines of correct Makefile rules.

---

## 2. Where It Fits

```
  configure.ac  ─(AM_INIT_AUTOMAKE)─┐
  Makefile.am ───▶ automake ───▶ Makefile.in ───▶ ./configure ───▶ Makefile
```

Requirements in `configure.ac`:
```m4
AC_INIT([myapp], [1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])   # turn Automake on
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])     # one per directory with a Makefile.am
AC_OUTPUT
```

- **`AM_INIT_AUTOMAKE`** activates Automake. Options: `foreign` (don't require GNU
  files like `NEWS`, `COPYING`), `subdir-objects` (place objects next to sources),
  `-Wall -Werror` (warnings).
- **`autoreconf -i`** runs `aclocal`, `autoconf`, `autoheader`, `automake` in order and
  installs helper scripts.

---

## 3. Primaries and Prefixes: the Naming System

Automake's whole language is variables of the form **`prefix_PRIMARY`**.

- **PRIMARY** = *what kind of thing*:
  - `PROGRAMS` — executables
  - `LIBRARIES` — static libs (`.a`)
  - `LTLIBRARIES` — libtool libraries (shared/static)
  - `HEADERS` — installable headers
  - `SCRIPTS` — executable scripts
  - `DATA` — arbitrary data files
  - `MANS` — man pages
- **prefix** = *where it installs* (maps to a directory variable):
  - `bin_` → `$(bindir)` (`/usr/local/bin`)
  - `lib_` → `$(libdir)`
  - `include_` → `$(includedir)`
  - `sysconf_` → `$(sysconfdir)`
  - `pkgdata_` → `$(datadir)/$(PACKAGE)`
  - `noinst_` → **not installed** (build-only, e.g. convenience libs)
  - `check_` → built only for `make check`

Examples:
```makefile
bin_PROGRAMS   = hello          # install hello into bindir
lib_LIBRARIES  = libfoo.a       # install libfoo.a into libdir
include_HEADERS = foo.h         # install foo.h into includedir
noinst_LIBRARIES = libhelper.a  # internal, not installed
```

Then per-target variables describe each item:
```makefile
hello_SOURCES  = main.c util.c
hello_CFLAGS   = -Wall
hello_LDADD    = libfoo.a -lm
```

---

## 4. A Minimal Project

```
myapp/
├── configure.ac
├── Makefile.am
└── src/
    ├── Makefile.am
    └── main.c
```

Top `Makefile.am`:
```makefile
SUBDIRS = src
```

`src/Makefile.am`:
```makefile
bin_PROGRAMS = hello
hello_SOURCES = main.c
```

Build:
```bash
autoreconf -i
./configure
make
make install
```

---

## 5. Programs, Sources, Flags

For a program named `foo`, Automake reads these per-target variables (note the target
name is sanitized: `foo-bar` → `foo_bar_SOURCES`):

| Variable | Meaning |
|----------|---------|
| `foo_SOURCES` | source + header files that make up `foo` |
| `foo_CPPFLAGS` | preprocessor flags (`-I`, `-D`) for `foo` |
| `foo_CFLAGS` | C compiler flags for `foo` |
| `foo_LDFLAGS` | linker flags (`-L`, `-static`) for `foo` |
| `foo_LDADD` | libraries/objects to link into `foo` (`-lm`, `libx.a`) |
| `foo_DEPENDENCIES` | extra prerequisites (usually inferred) |

Project-wide defaults use `AM_` prefixes: `AM_CPPFLAGS`, `AM_CFLAGS`, `AM_LDFLAGS`.
Users can still override on the command line (`make CFLAGS=-g`).

```makefile
bin_PROGRAMS = hello
hello_SOURCES = main.c greet.c greet.h
AM_CPPFLAGS = -I$(top_srcdir)/include
hello_LDADD = -lm
```

---

## 6. Libraries

**Static library installed to `libdir`:**
```makefile
lib_LIBRARIES = libmath.a
libmath_a_SOURCES = add.c mul.c math.h
```
(Requires `AC_PROG_RANLIB` in `configure.ac`.) Note the name mangling:
`libmath.a` → `libmath_a_SOURCES`.

**Convenience (non-installed) library** — link internal code into several programs:
```makefile
noinst_LIBRARIES = libutil.a
libutil_a_SOURCES = util.c util.h

bin_PROGRAMS = tool
tool_SOURCES = tool.c
tool_LDADD = libutil.a
```

**Shared libraries** need **Libtool** (`LT_INIT` in `configure.ac`,
`lib_LTLIBRARIES = libfoo.la`). Libtool handles the portability of shared-object naming
and versioning across platforms — that's a whole topic of its own.

---

## 7. Headers, Data, Scripts, Man Pages

```makefile
include_HEADERS = foo.h            # -> $(includedir)/foo.h
pkgdata_DATA    = default.conf     # -> $(datadir)/$(PACKAGE)/default.conf
bin_SCRIPTS     = run-foo.sh       # -> $(bindir)/run-foo.sh (marked executable)
dist_man1_MANS  = foo.1            # -> $(mandir)/man1/foo.1, and shipped in tarball
```

The **`dist_`** prefix means "include this file in `make dist`"; **`nodist_`** means
"do not distribute" (e.g., generated files). By default Automake distributes sources
and known files, but arbitrary `DATA` may need `dist_` to be shipped.

---

## 8. Multiple Directories (SUBDIRS)

```makefile
# top-level Makefile.am
SUBDIRS = lib src tests docs
```

Automake recurses in order. Each listed directory needs its own `Makefile.am`, and each
must be registered in `configure.ac`'s `AC_CONFIG_FILES`. Use `.` in `SUBDIRS` to
control when the current directory is built relative to children.

Refer across directories with **`$(top_srcdir)`** (source root) and
**`$(top_builddir)`** (build root) so out-of-tree builds keep working:
```makefile
AM_CPPFLAGS = -I$(top_srcdir)/lib
src_tool_LDADD = $(top_builddir)/lib/libutil.a
```

---

## 9. Tests with `make check`

```makefile
check_PROGRAMS = test_math      # built only for testing, not installed
test_math_SOURCES = test_math.c
test_math_LDADD = libmath.a

TESTS = test_math               # programs/scripts to run on `make check`
```

`make check` builds `check_PROGRAMS` and runs each `TESTS` entry; a non-zero exit is a
failure. The parallel/TAP test harness (`AM_INIT_AUTOMAKE([parallel-tests])` or modern
default) reports a summary and writes `.log` files. This is a real, if minimal, test
runner for free.

---

## 10. make dist and Distribution

```bash
make dist          # creates myapp-1.0.tar.gz containing everything needed to build
make distcheck     # unpacks the tarball in a clean dir, configures, builds, tests,
                   # installs to a temp prefix, and uninstalls -- the gold standard
```

**`make distcheck` passing is the definitive proof your project is portable and
self-contained.** It catches missing files in `_SOURCES`/`EXTRA_DIST`, broken
out-of-tree builds, and incomplete `uninstall` rules. Add non-source files to the
tarball with `EXTRA_DIST`:
```makefile
EXTRA_DIST = README.md LICENSE examples/
```

---

## 11. Conditionals and Options

Tie Makefile.am logic to Autoconf checks. In `configure.ac`:
```m4
AC_ARG_ENABLE([tools],
  [AS_HELP_STRING([--disable-tools],[do not build extra tools])],
  [], [enable_tools=yes])
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" = "xyes"])
```
In `Makefile.am`:
```makefile
bin_PROGRAMS = hello
if BUILD_TOOLS
bin_PROGRAMS += extratool
extratool_SOURCES = extratool.c
endif
```

`AM_CONDITIONAL` must be evaluated unconditionally in `configure.ac` (not inside a
shell `if`), or Automake complains.

---

## 12. Silent Rules and Dependency Tracking

- **Automatic dependency tracking** is on by default — Automake wires up per-object
  `.deps/*.Po` files (same idea as `-MMD -MP`), so editing a header rebuilds the right
  objects. No work needed on your part.
- **Silent rules**: `AM_INIT_AUTOMAKE([... silent-rules])` or `./configure
  --enable-silent-rules` gives tidy `CC foo.o` output; `make V=1` restores full command
  echo. Great for spotting warnings in large builds.

---

## 13. What to Commit

- **Commit:** `configure.ac`, every `Makefile.am`, `m4/*.m4` you author, sources,
  `README`/`LICENSE`, and typically a bootstrap `autogen.sh`.
- **Do NOT commit (generated):** `Makefile.in`, `configure`, `aclocal.m4`, `config.h.in`,
  `Makefile`, `config.status`, `config.log`, `build-aux/` helpers (`install-sh`,
  `missing`, `depcomp`, `compile`), `autom4te.cache/`, `.deps/`, `*.o`, binaries,
  and `*.tar.gz`.
- Release tarballs from `make dist` **do** contain the generated `configure`/`*.in` so
  end users need only `./configure && make`.

---

## 14. Common Pitfalls

- **Target-name mangling**: `libfoo.a` uses `libfoo_a_SOURCES`; `my-prog` uses
  `my_prog_SOURCES`. Dots and dashes become underscores.
- Forgetting to list a directory's `Makefile` in `AC_CONFIG_FILES` → that subdir isn't
  configured.
- Forgetting `AC_PROG_RANLIB` (static libs) or `LT_INIT` (shared libs).
- Putting `AM_CONDITIONAL` inside a shell `if` in `configure.ac` → Automake error.
- Files used by the build but not in any `_SOURCES`/`EXTRA_DIST` → missing from
  tarball; `make distcheck` catches this.
- Hand-editing `Makefile.in`/`Makefile` — regenerated away. Edit `Makefile.am`.
- Not adding `foreign` to `AM_INIT_AUTOMAKE` when you lack GNU-standard files, causing
  "required file './NEWS' not found".

---

## 15. The Runnable Examples

Under [`examples/automake/`](examples/automake/). Need `autoconf automake` (and
`libtool` for shared libs):
```bash
sudo apt-get install autoconf automake libtool
```

| Folder | Teaches |
|--------|---------|
| `01-hello-automake` | Full project: top + `src/` + `lib/` + `tests/`, a `noinst` convenience library, `bin_PROGRAMS`, `check_PROGRAMS`/`TESTS`, install/uninstall, `make dist`/`distcheck` |

Run:
```bash
cd examples/automake/01-hello-automake
autoreconf -i
mkdir build && cd build         # out-of-tree build (VPATH)
../configure --prefix=$PWD/../stage
make
make check                      # runs the test program
./src/hello
make install                    # installs into ../stage
make dist                       # produce the release tarball
make distcheck                  # the full portability self-test
```

## 16. Further References

- **Automake Manual** (GNU): https://www.gnu.org/software/automake/manual/ — read
  "A Small Hello World", "Programs", and "Tests".
- **Autotools Mythbuster**: https://autotools.io/ — modern, pragmatic recipes.
- **"Autotools" by John Calcote** (No Starch Press, 2nd ed.) — the definitive book.
- **The Autobook (Goat book)**: https://www.sourceware.org/autobook/
- `info automake`, and study a real project's `Makefile.am` (e.g., a small GNU package).
