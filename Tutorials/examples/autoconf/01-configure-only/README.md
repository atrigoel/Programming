# Autoconf Example 01 — Configure Only (no Automake)

Pure Autoconf: you write `configure.ac` and a hand-written `Makefile.in` template.
`configure` probes the system and fills in `Makefile` + `config.h`.

## Prerequisites

```bash
sudo apt-get install autoconf     # provides autoconf + autoheader + autoreconf
```

## Run

```bash
./autogen.sh            # or: autoreconf -i   (generates ./configure, config.h.in)
./configure             # detects CC, headers, libm; writes Makefile + config.h
make
./hello

# Try the custom feature switch:
./configure --enable-debug && make && ./hello    # note the [debug build] line

# Standard options work out of the box:
./configure --help
./configure --prefix=$PWD/stage && make && make install
find stage

make clean
```

## What to notice

- **`AC_SEARCH_LIBS([sqrt],[m])`** figures out whether `-lm` is needed and puts it in
  `@LIBS@` → `Makefile`'s `LIBS`. You never hard-code `-lm`.
- **`AC_CHECK_HEADERS([unistd.h ...])`** defines `HAVE_UNISTD_H` in `config.h`, which
  `main.c` tests with `#ifdef`.
- **`@CC@ @CFLAGS@ @LIBS@ @prefix@`** in `Makefile.in` are replaced by `configure`.
- Open **`config.h`** to see the generated `#define`s, and **`config.log`** to see the
  exact test programs configure compiled.
- Edit `configure.ac`, then re-run `autoreconf -i` — never edit `configure` or the
  generated `Makefile` directly.
