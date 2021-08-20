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

# Random boot.hex for synthesis
$(BUILD_TMP)/boot.hex:
	$(ICEBRAM) -g -s 0 32 1024 > $@
