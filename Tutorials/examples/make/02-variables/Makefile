# Example 02: variables and automatic variables.
# Two source files (main.c + greet.c) linked into one program.

CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS =
TARGET  = app
OBJS    = main.o greet.o

.PHONY: all clean

all: $(TARGET)

# $@ = target (app), $^ = all prerequisites (main.o greet.o)
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Explicit compile rules. $< = first prerequisite (the .c file), $@ = the .o
main.o: main.c greet.h
	$(CC) $(CFLAGS) -c $< -o $@

greet.o: greet.c greet.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
