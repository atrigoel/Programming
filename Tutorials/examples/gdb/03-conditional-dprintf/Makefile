# Build debuggable and run the conditional-breakpoint / dprintf demo.
CC = gcc
CFLAGS = -Og -g3 -Wall -Wextra

.PHONY: all demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Scripted walkthrough: conditional break on the bad item, then dprintf tracing.
demo: prog
	gdb -q -batch -x demo.gdb ./prog

debug: prog
	gdb -q ./prog

clean:
	rm -f prog
