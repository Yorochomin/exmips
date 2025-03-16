# MIPS Emulator

## Overview

This emulator emulates Atheros AR934x SoC including a processor core of MIPS32R2. The SoC has been used in home routers.
It is capable to run OpenWRT, a well-known Linux distribution for home routers.

This emulator implements runtime binary translation, i.e., translating a MIPS32R2 instruction into x64 instructions during execution, 
to increase execution speed. This functionarity works properly only in x64 processors.
The runtime binary translation is enabled by default and can be disabled by build options in src/config.h.

Most of functionalities to run Linux including an Ethernet controller is implemented in this emulators.
However, the following features of the SoC are not included.
- Wireless interface
- PCIe 
- Audio interfaces
- USB interface
- Internal SRAM
- DRAM controller
- Ethernet switch
And, some functionalities of cache instruction are not implemented because cache memory is not emulated.
Especially, it is not possible to run tricky codes such like codes using cache memory as a memory outside of the DRAM address region 
by locking cache lines with cache instruction.

## Build

### Emulator

```
$ make 
```

### ROM image (consisting of U-Boot and OpenWRT)

```
$ cd u-boot
$ bash build_uboot.sh
```
"firm_u-boot.bin" is the resultant ROM image.

## Usage

```
$ ./exmips u-boot/firm_u-boot.bin
```

