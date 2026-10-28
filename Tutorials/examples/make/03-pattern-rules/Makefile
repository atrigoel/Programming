# Example 03: one pattern rule builds every object.

CC     = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = calc
OBJS   = main.o ops.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# Pattern rule: any %.o is built from the matching %.c.
# $* is the stem (e.g. "main" or "ops").
%.o: %.c ops.h
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  (built $@ from stem '$*')"

clean:
	rm -f $(TARGET) $(OBJS)
