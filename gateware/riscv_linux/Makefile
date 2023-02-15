# Execs
ICEBRAM := icebram


# no2build Project config
# -----------------------

PROJ = riscv_linux

PROJ_DEPS := no2misc no2ice40 no2memcache
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

YOSYS_SYNTH_ARGS = -dffe_min_ce_use 4 -no-rw-check # -abc2
NEXTPNR_ARGS  = --pre-pack data/clocks.py --pre-place $(CORE_no2ice40_DIR)/sw/serdes-nextpnr-place.py --seed $(SEED)
NEXTPNR_ARGS += --timing-allow-fail

# Build options
MEM ?= hram

ifeq ($(MEM),hram)
	PROJ_DEPS += no2hyperbus
	YOSYS_READ_ARGS += -DMEM_HRAM=1
endif
ifeq ($(MEM),qpi)
	PROJ_DEPS += no2qpimem
	YOSYS_READ_ARGS += -DMEM_QPI=1
endif

# Include default rules
include ../build/project-rules.mk


# Firmware
# --------

FW_BOOT_HW  := ../../firmware/boot/boot-$(MEM).hex
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


# Memory image for Simulation
# ---------------------------

LINUX_IMAGE ?= $(BUILD_TMP)/linux.bin

$(BUILD_TMP)/linux.bin:
	@echo "[!] LINUX_IMAGE must point to the compiled Linux kernel image"
	@exit 1

$(BUILD_TMP)/linux.hex: $(LINUX_IMAGE)
	hexdump -ve '1/4 "%08x\n"' $< > $@

$(BUILD_TMP)/firmware.hex: $(BUILD_TMP)/linux.hex $(FW_DT)
	./sw/hexmerge.py 0:$(BUILD_TMP)/linux.hex 4194304:$(FW_DT) > $@

sim-firmware: $(BUILD_TMP)/firmware.hex


# CXX RTL
# -------

PROJ_CXXRTL_SRCS := $(PROJ_ALL_RTL_SRCS) $(PROJ_TOP_SRC)
PROJ_CXXRTL_SRCS += $(abspath sim/ice40_cells_sim_cxxrtl.v)
PROJ_CXXRTL_SRCS += $(CORE_no2memcache_DIR)/sim/mem_sim.v

$(BUILD_TMP)/$(PROJ)-cxxrtl.ys: $(PROJ_CXXRTL_SRCS)
	@echo "read_verilog -DSIM=1 -DCXXRTL=1 $(YOSYS_READ_ARGS) $(PROJ_SYNTH_INCLUDES) $(PROJ_SIM_INCLUDES) $(PROJ_CXXRTL_SRCS)" > $@
	@echo "write_cxxrtl -O4 -g4 -header $(PROJ_TOP_MOD).cpp" >> $@

$(BUILD_TMP)/$(PROJ).cxxrtl.rpt $(BUILD_TMP)/$(PROJ_TOP_MOD).cpp $(BUILD_TMP)/$(PROJ_TOP_MOD).h: $(PROJ_ALL_PREREQ) $(BUILD_TMP)/$(PROJ)-cxxrtl.ys $(BUILD_TMP)/boot-sim.hex
	cd $(BUILD_TMP) && \
		$(YOSYS) -s $(BUILD_TMP)/$(PROJ)-cxxrtl.ys \
			 -l $(BUILD_TMP)/$(PROJ).cxxrtl.rpt

$(BUILD_TMP)/%.o: $(BUILD_TMP)/%.cpp $(BUILD_TMP)/$(PROJ_TOP_MOD).h
	clang -c -O2 -g -std=c++14 -I`yosys-config --datdir/include` $< -o $@

$(BUILD_TMP)/top_tb.cpp: sim/top_tb.cpp
	cp -a $< $@

$(BUILD_TMP)/mem_sim_data.cpp: $(CORE_no2memcache_DIR)/sim/mem_sim_data.cpp
	sed -e's/sim.h/top.h/' -e's/MEM_SIM_AW/23/' $< > $@

$(BUILD_TMP)/top_tb_cxxrtl: $(BUILD_TMP)/top_tb.o $(BUILD_TMP)/top.o $(BUILD_TMP)/mem_sim_data.o
	clang -lstdc++ -o $@ $^

cxxrtl: $(BUILD_TMP)/top_tb_cxxrtl
