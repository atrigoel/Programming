# Build debuggable (-Og keeps optimization light but line info stable) and run a demo.
CC = gcc
CFLAGS = -Og -g3 -Wall -Wextra

.PHONY: all demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Scripted walkthrough: breakpoints, backtrace, frames, step/next/finish.
demo: prog
	gdb -q -batch -x demo.gdb ./prog

# Open an interactive session to try things yourself.
debug: prog
	gdb -q ./prog

clean:
	rm -f prog
