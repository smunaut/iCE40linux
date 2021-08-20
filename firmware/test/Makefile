BOARD ?= icebreaker
CROSS ?= riscv-none-embed-
CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
ICEPROG = iceprog

BOARD_DEFINE=BOARD_$(shell echo $(BOARD) | tr a-z\- A-Z_)
CFLAGS=-Wall -Os -march=rv32i -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -D$(BOARD_DEFINE) -I.

HEADERS = \
	config.h \
	console.h \
	led.h \
	mini-printf.h \
	$(NULL)

SOURCES = \
	start.S \
	console.c \
	led.c \
	mini-printf.c  \
	main.c \
	$(NULL)

all: test.bin

test.elf: lnk-app.lds $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,lnk-app.lds,--strip-debug -o $@ $(SOURCES)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

prog: fw_app.bin
	$(ICEPROG) -o 640k $<

clean:
	rm -f *.bin *.hex *.elf *.o

.PHONY: prog clean
