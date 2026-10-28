# Build a crashing program, produce a core dump, and inspect it post-mortem.
CC = gcc
CFLAGS = -O0 -g3 -Wall -Wextra

.PHONY: all crash demo debug clean

all: prog

prog: main.c
	$(CC) $(CFLAGS) main.c -o prog

# Run it so it crashes and drops a core file in this directory.
# (ulimit only affects this shell; core_pattern may send it to systemd instead — see README.)
crash: prog
	-ulimit -c unlimited; ./prog

# Crash, then open the core dump in batch mode and print the stack.
demo: prog
	-ulimit -c unlimited; ./prog; true
	@echo "== opening core dump =="
	@corefile=$$(ls -1 core* 2>/dev/null | head -n1); \
	if [ -z "$$corefile" ]; then \
	  echo "No core file here. Your system likely routes cores to systemd-coredump."; \
	  echo "Try:  coredumpctl gdb prog   (see README)"; \
	else \
	  gdb -q -batch -x demo.gdb ./prog "$$corefile"; \
	fi

debug: prog
	gdb -q ./prog

clean:
	rm -f prog core core.*
