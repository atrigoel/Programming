# Walk each stage of the GCC pipeline as its own target so you can inspect the output.
CC = gcc

.PHONY: all stages clean

all: main

# 1) Preprocess only: expand #include/#define -> main.i
main.i: main.c
	$(CC) -E main.c -o main.i

# 2) Compile only: C -> assembly (main.s)
main.s: main.c
	$(CC) -S main.c -o main.s

# 3) Assemble only: assembly/C -> object file (main.o), no linking
main.o: main.c
	$(CC) -c main.c -o main.o

# 4) Link the object into an executable
main: main.o
	$(CC) main.o -o main

# Produce every intermediate at once, then run the program.
stages: main.i main.s main.o main
	@echo "--- main.i (first 15 lines of preprocessed source) ---"
	@head -n 15 main.i
	@echo "--- main.s (assembly) ---"
	@cat main.s
	@echo "--- running ./main ---"
	@./main

clean:
	rm -f main main.o main.i main.s
