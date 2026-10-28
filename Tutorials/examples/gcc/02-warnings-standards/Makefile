# Compare warning levels on deliberately sloppy code.
CC = gcc

.PHONY: all lax strict pedantic werror clean

all: strict

# Almost silent: the compiler accepts a lot by default.
lax:
	$(CC) main.c -o main_lax

# The recommended baseline. Expect several warnings here.
strict:
	$(CC) -Wall -Wextra -Wshadow -Wconversion main.c -o main_strict

# Enforce strict ISO C: flags non-standard constructs too.
pedantic:
	$(CC) -std=c11 -Wall -Wextra -Wpedantic main.c -o main_pedantic

# Make warnings fatal — this build FAILS, which is the point (CI gate).
werror:
	$(CC) -Wall -Wextra -Werror main.c -o main_werror

clean:
	rm -f main_lax main_strict main_pedantic main_werror
