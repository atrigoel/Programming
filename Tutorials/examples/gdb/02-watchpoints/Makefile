# Build debuggable and run the watchpoint demo.
CC = gcc
CFLAGS = -Og -g3 -Wall -Wextra

.PHONY: all demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Scripted walkthrough: a watchpoint stops on every write to a.balance.
demo: prog
	gdb -q -batch -x demo.gdb ./prog

debug: prog
	gdb -q ./prog

clean:
	rm -f prog
