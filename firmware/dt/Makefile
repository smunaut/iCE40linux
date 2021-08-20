all: ice40linux.dtb

%.dtb: %.dts
	dtc -I dts -O dtb -o $@ $<

%.hex: %.dtb
	hexdump -ve '1/4 "%08x\n"' $< > $@

clean:
	rm -f *.dtb

.PHONY: clean
