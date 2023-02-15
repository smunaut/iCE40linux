CROSS ?= riscv-none-embed-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

CFLAGS=-Wall -Os -march=rv32i -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs

UART_DIV ?= 13

all: boot-hram.hex boot-qpi.hex boot-sim.hex

boot-hram.elf: lnk-boot.lds boot.S
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,lnk-boot.lds,--strip-debug -DMEM_HRAM -DUART_DIV=$(UART_DIV) -o $@ boot.S

boot-qpi.elf: lnk-boot.lds boot.S
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,lnk-boot.lds,--strip-debug -DMEM_QPI  -DUART_DIV=$(UART_DIV) -o $@ boot.S

boot-sim.elf: lnk-boot.lds boot.S
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,lnk-boot.lds,--strip-debug -DBOOT_SIM -DUART_DIV=$(UART_DIV) -o $@ boot.S

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.hex: %.bin
	hexdump -ve '1/4 "%08x\n"' $< > $@

clean:
	rm -f *.bin *.hex *.elf *.o

.PHONY: clean
