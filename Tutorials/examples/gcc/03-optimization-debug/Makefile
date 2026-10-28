# Build the same source at several optimization levels and compare size + assembly.
CC = gcc

.PHONY: all compare sizes asm clean

all: compare

main_O0: main.c
	$(CC) -O0 -g3 main.c -o main_O0
main_O2: main.c
	$(CC) -O2 main.c -o main_O2
main_Os: main.c
	$(CC) -Os main.c -o main_Os
main_Og: main.c
	$(CC) -Og -g3 main.c -o main_Og

# Emit assembly for -O0 vs -O2 so you can diff the optimizer's work.
main_O0.s: main.c
	$(CC) -O0 -S main.c -o main_O0.s
main_O2.s: main.c
	$(CC) -O2 -S main.c -o main_O2.s

sizes: main_O0 main_O2 main_Os main_Og
	@echo "Section sizes (text = code, smaller is tighter):"
	@size main_O0 main_O2 main_Os main_Og

asm: main_O0.s main_O2.s
	@echo "Lines of assembly for sum_of_squares:"
	@echo "  -O0: $$(grep -c . main_O0.s) total asm lines"
	@echo "  -O2: $$(grep -c . main_O2.s) total asm lines"

compare: sizes asm

clean:
	rm -f main_O0 main_O2 main_Os main_Og main_O0.s main_O2.s
