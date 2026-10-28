# Cross-compile a bare-metal Cortex-M4 image with the GNU Arm Embedded toolchain.
# Requires arm-none-eabi-gcc / objcopy / size on your PATH.
CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

# CPU selectors — MUST be identical for compile and link steps.
CPU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  = $(CPU) -Os -g3 -Wall -Wextra \
          -ffunction-sections -fdata-sections
LDFLAGS = $(CPU) -T linker/cortex-m4.ld -nostartfiles \
          -Wl,--gc-sections -Wl,-Map=build/fw.map \
          --specs=nano.specs --specs=nosys.specs

OBJS = build/startup.o build/main.o

.PHONY: all size clean

all: build/fw.bin build/fw.hex size

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.s | build
	$(CC) $(CFLAGS) -c $< -o $@

build/fw.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

build/fw.hex: build/fw.elf
	$(OBJCOPY) -O ihex $< $@

build/fw.bin: build/fw.elf
	$(OBJCOPY) -O binary $< $@

size: build/fw.elf
	$(SIZE) build/fw.elf

clean:
	rm -rf build
