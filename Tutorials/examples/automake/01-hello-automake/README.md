# Automake Example 01 — Full Hello Project

A complete multi-directory Autotools project:

```
01-hello-automake/
├── configure.ac          # Autoconf + AM_INIT_AUTOMAKE
├── Makefile.am           # SUBDIRS = lib src tests
├── lib/                  # noinst convenience static library
│   ├── util.c / util.h
│   └── Makefile.am
├── src/                  # bin_PROGRAMS = hello  (links libutil.a)
│   ├── main.c
│   └── Makefile.am
└── tests/                # check_PROGRAMS + TESTS
    ├── test_util.c
    └── Makefile.am
```

## Prerequisites

```bash
sudo apt-get install autoconf automake
```

## Run (out-of-tree build recommended)

```bash
./autogen.sh                    # or: autoreconf -i
mkdir build && cd build
../configure --prefix=$PWD/../stage
make
make check                      # builds + runs tests/test_util  (should PASS)
./src/hello
make install                    # installs hello into ../stage/bin
../stage/bin/hello
make dist                       # -> hello-automake-1.0.tar.gz
make distcheck                  # unpack + configure + build + test + install + uninstall
```

## What to notice

- **Tiny `Makefile.am` files** expand into full-featured Makefiles (install, uninstall,
  clean, distclean, check, dist) — Automake writes the rules.
- **`noinst_LIBRARIES = libutil.a`** is a *convenience* library: built and linked into
  `hello` and the test, but never installed.
- **`check_PROGRAMS` + `TESTS`** give you a working `make check` test runner.
- **`$(top_srcdir)` / `$(top_builddir)`** make the out-of-tree `build/` directory work
  correctly (source vs generated locations kept separate).
- **`make distcheck`** is the real prize: if it passes, your project is provably
  self-contained and portable. Try deleting a file from a `_SOURCES` list and watch
  `distcheck` fail.
- Never edit `Makefile` or `Makefile.in`; edit `Makefile.am` and re-run `autoreconf`.
