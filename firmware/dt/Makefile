all: ice40linux.dtb

%.dtb: %.dts
	dtc -I dts -O dtb -o $@ $<

clean:
	rm -f *.dtb

.PHONY: clean
