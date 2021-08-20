# Execs
ICEBRAM := icebram


# no2build Project config
# -----------------------

PROJ = riscv_linux

PROJ_DEPS := no2misc no2ice40 no2hyperbus no2memcache
PROJ_RTL_SRCS := $(addprefix rtl/, \
	platform.v \
	soc_bram.v \
	sysmgr.v \
	VexRiscv.v \
)
PROJ_SIM_SRCS := $(addprefix sim/, \
	spiflash.v \
)
PROJ_SIM_SRCS += rtl/top.v
PROJ_TESTBENCHES := \
	platform_tb \
	top_tb \
	$(NULL)
PROJ_PREREQ = \
	$(BUILD_TMP)/boot.hex \
	$(NULL)
PROJ_TOP_SRC := rtl/top.v
PROJ_TOP_MOD := top

# Target config
BOARD ?= icebreaker
DEVICE = up5k
PACKAGE = sg48

SEED ?= 0

YOSYS_SYNTH_ARGS = -dffe_min_ce_use 4 # -abc2
NEXTPNR_ARGS  = --pre-pack data/clocks.py --pre-place $(CORE_no2ice40_DIR)/sw/serdes-nextpnr-place.py --seed $(SEED)
NEXTPNR_ARGS += --timing-allow-fail

# Include default rules
include ../build/project-rules.mk


# Firmware
# --------

FW_BOOT_HW  := ../../firmware/boot/boot-hw.hex
FW_BOOT_SIM := ../../firmware/boot/boot-sim.hex
FW_BIOS     := ../../firmware/bios/bios.hex
FW_DT       := ../../firmware/dt/ice40linux.hex

# Random boot.hex for synthesis
$(BUILD_TMP)/boot.hex:
	$(ICEBRAM) -g -s 0 32 1024 > $@

# Build of BootLoader, BIOS code and DTB
../../firmware/boot/%.hex: _force_run_
	make -C ../../firmware/boot/ $*.hex

../../firmware/bios/%.hex: _force_run_
	make -C ../../firmware/bios/ $*.hex

../../firmware/dt/ice40linux.dtb: _force_run_
	make -C ../../firmware/dt/ ice40linux.dtb

../../firmware/dt/ice40linux.hex: _force_run_
	make -C ../../firmware/dt/ ice40linux.hex

# Combined BootLoader & BIOS for BRAM init
$(BUILD_TMP)/boot-hw.hex: $(FW_BOOT_HW) $(FW_BIOS)
	./sw/hexmerge.py 0:$(FW_BOOT_HW) 256:$(FW_BIOS) > $@

$(BUILD_TMP)/boot-sim.hex: $(FW_BOOT_SIM) $(FW_BIOS)
	./sw/hexmerge.py 0:$(FW_BOOT_SIM) 256:$(FW_BIOS) > $@

# Force call to update .hex from other Makefiles
_force_run_:
.PHONY: _force_run_


# Final HW with BRAM init
# -----------------------

$(BUILD_TMP)/$(PROJ)_init.asc: $(BUILD_TMP)/$(PROJ).asc $(BUILD_TMP)/boot.hex $(BUILD_TMP)/boot-hw.hex
	cat $< | icebram $(BUILD_TMP)/boot.hex $(BUILD_TMP)/boot-hw.hex > $@

bin-init: $(BUILD_TMP)/$(PROJ)_init.bin

prog-init: $(BUILD_TMP)/$(PROJ)_init.bin
	$(ICEPROG) $<
