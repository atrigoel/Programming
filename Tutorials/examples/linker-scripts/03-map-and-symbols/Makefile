# Example 03: generate and read a linker MAP file; observe --gc-sections.

CC     = gcc
CFLAGS = -Wall -Wextra -O2 -ffunction-sections -fdata-sections
TARGET = mapdemo
MAP    = $(TARGET).map

.PHONY: all analyze clean

all: analyze

# -Wl,-Map=... asks the linker to emit a map file.
# --gc-sections drops unused sections (needs -ffunction/-fdata-sections above).
$(TARGET): main.c
	$(CC) $(CFLAGS) -Wl,-Map=$(MAP) -Wl,--gc-sections -o $@ main.c

analyze: $(TARGET)
	@echo "=== size ==="
	size $(TARGET)
	@echo ""
	@echo "=== map file: memory configuration ==="
	sed -n '/Memory Configuration/,/Linker script and memory map/p' $(MAP) | head -n 12
	@echo ""
	@echo "=== where main/table/scratch landed (from the map) ==="
	grep -E "main$$|table$$|scratch$$|helper" $(MAP) || true
	@echo ""
	@echo "=== symbol addresses (nm, sorted) ==="
	nm -n $(TARGET) | grep -E "main|table|scratch|helper"
	@echo ""
	@echo "Open $(MAP) in an editor to see every input section's placement."
	./$(TARGET)

clean:
	rm -f $(TARGET) $(MAP)
