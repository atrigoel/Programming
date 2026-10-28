# Build debuggable and run the scripting demo (user commands + Python pretty-printer).
CC = gcc
CFLAGS = -O0 -g3 -Wall -Wextra

.PHONY: all demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Scripted walkthrough: define a command, walk the list, then load a Python pretty-printer.
demo: prog
	gdb -q -batch -x commands.gdb ./prog

debug: prog
	gdb -q ./prog

clean:
	rm -f prog
