# Minimal gcc makefile project for STM32F401xE

This contains the absolute bare minimum needed
to compile a blinky example for the STM32F401xE
on the command line with gcc and make and then
flash it to the demo board with OpenOCD. It is
using the original headers provided by STM and
ARM but only those for the core and peripheral
registers, none of the HAL or any other optional
libs.

The startup code and the linker script have been
implemented from scratch and without the Atollic
copyright, the startup code and the SystemInit
have been implemented in plain C and are in the
file gcc_startup_system.c.

# Usage

## Prerequisites

* install Linux on your PC
* get a NUCLEO-F401RE demo board from STMicroelectronics
* install the official arm-none-eabi-gcc toolchain from Launchpad
* install OpenOCD

## Build and run

* clone this repository
* connect the NUCLEO board
* `$ make install`
* watch the green LED blink while studying the reference manual

happy hacking :-)

