# Demonstrates -I (include path), -D (defines), building a static library,
# then linking against it with -L / -l. Also shows -MMD auto-dependencies.
CC       = gcc
CPPFLAGS ?= -DVERSION=\"1.0\"     # override on the CLI, e.g. CPPFLAGS='-DENABLE_EXTRA -DVERSION=\"2.0\"'
CFLAGS   = -Wall -Wextra -Iinclude
LDFLAGS  = -L.
LDLIBS   = -lgreet

.PHONY: all run clean

all: app

# 1) Compile the library source (object carries auto-dependency file main.d style).
greet.o: lib/greet.c
	$(CC) $(CFLAGS) -MMD -c lib/greet.c -o greet.o

# 2) Archive it into a static library libgreet.a.
libgreet.a: greet.o
	ar rcs libgreet.a greet.o

# 3) Compile the app, then link against libgreet via -L. and -lgreet.
main.o: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c main.c -o main.o

app: main.o libgreet.a
	$(CC) main.o $(LDFLAGS) $(LDLIBS) -o app

run: app
	./app

# Pull in generated header-dependency files if present.
-include main.d greet.d

clean:
	rm -f app *.o *.a *.d
