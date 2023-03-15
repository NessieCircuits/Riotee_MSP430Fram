# MSP430 SPI non-volatile memory firmware

The code in this repository turns an MSP430FR59XX series microcontrollers into a general purpose non-volatile memory controlled via SPI.

## Basics

The MSP430FR59XX series of microcontrollers come with on-board ferroelectric RAM (FRAM). In contrast to SRAM, this memory is non-volatile, i.e. it keeps its state without requiring periodic refreshs, i.e. without requiring a power supply. In contrast to flash memory, which must be erased before being written, FRAM can be treated just like SRAM and writing consumes very little energy.

The code in this repository exposes the internal FRAM via an SPI interface. The MSP430 remains in a low power mode until it detects a falling edge on the Chip Select line. Upon this edge, it prepares a 3 Byte SPI transfer via DMA. These first three bytes describe the type of transaction (R/W) and the memory address. The code proceeds to setup the corresponding transfer, also via DMA and enters low power mode while the SPI transaction to/from non-volatile FRAM takes place. After detecting a rising edge on the Chip Select line, the controller stops the transfer and transitions to low power mode again.

## Building

To build the code, you'll need gcc toolchain for the MSP430 microcontroller series.

You can build the project using:

```
make
```

You will find a binary `build.elf` in the `_build` directory.