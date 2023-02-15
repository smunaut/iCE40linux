iCE40 Linux
===========

This repository contains what's needed to reproduce the
"Linux running on RISC-V on an iCE40 UP5k" demo and can
possibly be used as a base to extend it to other platforms.


Hardware
--------

The target hardware platform is a Lattice UltraPlus 5k FPGA
with 32 Mbytes of RAM attached, either provided by 4 x 64 Mbits
HyperRAM chips or 4 x 64 Mbits SPI PSRAM chips.

* [iCEBreaker](https://1bitsquared.com/products/icebreaker)
* [Quad HyperRAM PMOD](https://1bitsquared.com/products/pmod-hyperram)
  (Make sure to select the "Quad" variant)
* [Quad SPI PSRAM PMOD](https://machdyne.com/product/qqspi-psram32/)


How it works
------------

A quick rundown of the involved parts and what they do.

### Gateware

This is the HDL logic that uses the FPGA fabric to implement a RISC-V
CPU with enough peripherals to run a RV32I Linux kernel. It's mainly
composed of a [VexRiscv](https://github.com/SpinalHDL/VexRiscv/) CPU
connected to a memory controller and associated cache.

The set of peripheral is very minimal:

 * A UART for interacting with the system
 * A basic SPI controller to read/write from flash
 * A RGB PWN LED controller for demo purposes
 * A timer to provide periodic interrupts to the kernel


### Bootloader

This is the very first thing executed by the CPU coming out of
reset. This program is hard coded directly in the FPGA bitstream.

Its role are:

 * Initialize UART for boot debug
 * Initialize the memory controller
 * Load the various pieces from flash into RAM
 * Jump to the BIOS


### Machine Mode BIOS

The machine mode BIOS helps abstract some of the hw platform details
from the kernel by providing a few standard calls to the kernel for basic
operations (see `SBI` = `Supervisor Binary Interface`).

* UART in/out : Mostly useful during boot. Using them is not the most
  performant (native driver is faster) but for early boot it's very
  convenient.

* Timer setup : The way to access the hardware timer is not standardized
  in RISC-V so querying the current time and setting up next interrupt
  can be done with some standardized SBI calls if no native driver for
  the timer is available.

It's also used to emulate some features that are expected by the kernel
but are not supported natively by our VexRISCV configuration.

* Unaligned memory accesses
* Atomic memory accesses

Those will cause a machine mode trap that will be caught by the BIOS and
it will emulate them. It's not very performant but those are very occasional
thankfully.


### Build Root : Kernel + Root filesystem

We use buildroot to cross-compile the kernel and build a root file system
image that has all the utilities needed to provide a basic linux system.

The kernel has a few patches mostly to support not having the `M` extension
built-in in the CPU and to add drivers for the UART / SPI / LED peripherals
of the SoC used here.

Source tree can be found [here](https://github.com/smunaut/ice40linux-kernel).

The root filesystem is built as a UBI image so we directly have a read write
filesystem stored in flash and it doesn't need to be fully loaded in RAM
during boot (which speeds up boot a bit).


Building
--------

### Gateware

```bash
cd iCE40linux/gateware/riscv_linux
make bin-init
```

This will build the gateware along with the bootloader (that ends
up pre-loaded in the bitstream file).
You can add `CROSS=xxx` to select another RISCV toolchain cross prefix
or `BOARD=xxx` to specify another board.

The result file is in `build-tmp/riscv_linux_init.bin`

The default is to build for the HyperRAM option, but if you are
using SPI PSRAM instead, add `MEM=qpi` on the `make` command line.


### BIOS

```bash
cd iCE40linux/firmware/bios
make
```

You can add `CROSS=xxx` to select another RISCV toolchain cross prefix.

The result file is in `bios.bin`


### Device Tree

```bash
cd iCE40linux/firmware/dt
make
```

You will need the `dtc` device tree compiler.

The result file is in `ice40linux.dtb`


### BuildRoot (rootfs + kernel)

```bash
git clone http://github.com/buildroot/buildroot
cd buildroot
make BR2_EXTERNAL=../iCE40linux/buildroot/ ice40linux_vexriscv_defconfig
make
```

This will checkout `buildroot` and trigger the build both the UBI
rootfs image and the linux kernel. Refer to the buildroot documentation
to know how you can customize this step. (kernel options / add packages / ...)

The results files are :
 * `buildroot/output/images/Image` : The kernel binary
 * `buildroot/output/images/rootfs.ubi`: The rootfs UBI image


Flashing
--------

```bash
# Bulk erase the flash
iceprog -b

# Flash the Root FS image at offset=6M
iceprog -n -X -o 6144k buildroot/output/images/rootfs.ubi

# Flash the kernel at offset=256k
iceprog -n -X -o  256k buildroot/output/images/Image

# Flash the device tree at offset=192k
iceprog -n -X -o  192k iCE40linux/firmware/dt/ice40linux.dtb

# Flash the machine mode bios at offset=128k
iceprog -n -X -o  128k iCE40linux/firmware/bios/bios.bin

# Flash the FPGA bitstream at the beginning of flash
iceprog -n -X          iCE40linux/gateware/riscv_linux/build-tmp/riscv_linux_init.bin
```


Flash & Memory map
------------------

The following table lists the various elements that are placed
in flash along with their max size and where in memory they will
be loaded to by the bootloader before the kernel is started.


| Flash address      | Memory address | Size   | Description    |
|--------------------|----------------|--------|----------------|
| `0x000000`         | n/a            |   128k | FPGA bitstream |
| `0x020000`  (128k) | `0x00000400`   |     3k | BIOS           |
| `0x030000`  (192k) | `0x41000000`   |     8k | Device Tree    |
| `0x040000`  (256k) | `0x40000000`   |  4608k | Kernel         |
| `0x600000` (6144k) | n/a            | 10240k | UBI filesystem |

Note that some locations are baked/hard-coded in several places,
so if you want to modify the layout to try on another platform
make sure to check the following places :

* The bootloader (`firmware/boot`) built-in manifest
* The BIOS code (see `LINUX_IMAGE_BASE` and `LINUX_DTB_BASE`)
* The device tree (`bootargs`, `memory` node, `reserved-memory`, flash
  partition layout)


Overclocking
------------

By default the core is run at 15 MHz which is about the fmax given
by `nextpnr`. However there is some margin and I have added an option
to instead run everything at 20 MHz.

To do so, build the gateware with:

```bash
cd iCE40linux/gateware/riscv_linux
make clean
make clean-fw
make OVERCLOCK=1 bin-init
```

This will rebuild the gateware for 20 MHz.

You will also need to update the BIOS and add `UART_DIV=18` on the
`make` command. And then edit the `ice40linux.dts` to change all
references from `15000000` to `20000000`.


License
-------

Given this repository has submodules, you need to check the licensing info
for those directly in them.

For things written specifically for this project :

- The gateware part is under "CERN Open Hardware Licence Version 2 - Permissive" license.

- The firmware/software part that are not derived from LiteX project are
  under MIT. Some libraries / drivers / ... might have their own license.
  Check the header of each file to be sure.

- The firmware part derived from Linux-on-LiteX project, for instance the
  `firmware/bios` part, follow the original license and are under the
  2-clause BSD license.

The full text of various applicable license is included in the `doc/`
subdirectory.
