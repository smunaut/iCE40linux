CROSS ?= riscv-none-embed-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

CFLAGS=-Wall -Os -march=rv32i -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs

all: bios.bin

bios.elf: lnk-bios.lds config.h irq.h riscv.h main.c start.S
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,lnk-bios.lds,--strip-debug -o $@ main.c start.S

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.hex: %.bin
	hexdump -ve '1/4 "%08x\n"' $< > $@

clean:
	rm -f *.bin *.hex *.elf *.o

.PHONY: clean
