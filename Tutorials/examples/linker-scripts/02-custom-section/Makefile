# Example 02: place data in a custom section and bound it with linker symbols.

CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g
LDSCRIPT = extra.ld
TARGET  = custom

.PHONY: all analyze clean

all: analyze

# -Wl,-T,extra.ld splices our section into the DEFAULT script (INSERT AFTER).
$(TARGET): main.c $(LDSCRIPT)
	$(CC) $(CFLAGS) -Wl,-T,$(LDSCRIPT) -o $@ main.c

analyze: $(TARGET)
	@echo "=== the custom .my_config section (readelf) ==="
	readelf -S $(TARGET) | grep -E "my_config|Name" || true
	@echo ""
	@echo "=== boundary symbols defined by the linker script ==="
	nm $(TARGET) | grep -E "__my_config_start|__my_config_end|device_id|fw_version"
	@echo ""
	@echo "=== hexdump of the section contents (objdump) ==="
	objdump -s -j .my_config $(TARGET)
	@echo ""
	@echo "=== program output ==="
	./$(TARGET)

clean:
	rm -f $(TARGET)
