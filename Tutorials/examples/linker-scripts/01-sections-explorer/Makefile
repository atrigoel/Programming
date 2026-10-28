# Example 01: explore how the linker groups code/data into sections.
# Uses the *default* linker script (hosted Linux) and inspects the result.

CC     = gcc
CFLAGS = -Wall -Wextra -O0 -g
TARGET = explorer

.PHONY: all analyze clean

all: analyze

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ main.c

analyze: $(TARGET)
	@echo "=== size: text/data/bss summary ==="
	size $(TARGET)
	@echo ""
	@echo "=== readelf: section headers (addr + size) ==="
	readelf -S $(TARGET) | grep -E "\.text|\.rodata|\.data|\.bss|Name" || true
	@echo ""
	@echo "=== nm: where our symbols landed (T=text r/R=rodata D=data B=bss) ==="
	nm $(TARGET) | grep -E "message|read_only_answer|initialized_global|zeroed_global|big_buffer|main"
	@echo ""
	@echo "Tip: big_buffer[4096] lives in .bss, so it barely grows the file."
	@echo "     Compare 'size' bss column vs the on-disk file size."

clean:
	rm -f $(TARGET)
