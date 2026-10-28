# Example 05: automatic header-dependency tracking with -MMD -MP.

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -MMD -MP
SRCDIR   = src
BUILDDIR = build
TARGET   = $(BUILDDIR)/app

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)      # one .d file per object

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Pull in the generated dependency files. The leading '-' ignores them
# on the very first build when they do not exist yet.
-include $(DEPS)

clean:
	rm -rf $(BUILDDIR)
