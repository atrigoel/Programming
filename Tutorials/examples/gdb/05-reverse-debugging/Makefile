# Build debuggable and run the reverse-execution demo.
# NOTE: `record` needs native x86-64/AArch64 Linux (works under WSL2; not all containers).
CC = gcc
CFLAGS = -O0 -g3 -Wall -Wextra

.PHONY: all demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Scripted walkthrough: record forward, then reverse-continue through the writes.
demo: prog
	gdb -q -batch -x demo.gdb ./prog

debug: prog
	gdb -q ./prog

clean:
	rm -f prog
