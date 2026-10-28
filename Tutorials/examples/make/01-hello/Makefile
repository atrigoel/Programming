# Example 01: the smallest useful Makefile.
# NOTE: every recipe line below is indented with a real TAB character.

CC     = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: hello

hello: hello.c
	$(CC) $(CFLAGS) -o hello hello.c

clean:
	rm -f hello
