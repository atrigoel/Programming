# Example 04: phony targets + make functions with a src/ -> build/ layout.

CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRCDIR  = src
BUILDDIR = build
TARGET  = $(BUILDDIR)/demo

# Discover sources, then transform src/x.c -> build/x.o with functions.
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Optional install location (honors DESTDIR + prefix conventions).
prefix ?= /usr/local
bindir ?= $(prefix)/bin

.PHONY: all clean install uninstall run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# Create build/ on demand, then compile.
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(bindir)
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)/demo

uninstall:
	rm -f $(DESTDIR)$(bindir)/demo

clean:
	rm -rf $(BUILDDIR)
