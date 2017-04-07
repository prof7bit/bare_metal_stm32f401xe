# Minimal gcc makefile project for STM32F401xE

This contains the absolute bare minimum needed
to compile a blinky example for the STM32F401xE
on the command line with gcc and make and then
flash it to the demo board with OpenOCD. It is
using the original headers provided by STM and
ARM but only those for the core and peripheral
registers, none of the HAL or any other optional
libs.

The startup code has been implemented from scratch
and the linker script has been taken from the gcc
distribution and adapted for this controller and
therefore none of these files have any Atollic
copyright anymore. Startup code and SystemInit
have been implemented in plain C and are together
in one file gcc_startup_system.c.


# Usage

## Prerequisites

* install Linux on your PC
* get a NUCLEO-F401RE demo board from STMicroelectronics
* install the official arm-none-eabi-gcc toolchain from Launchpad
* install OpenOCD
* add this line to your udev rules (permissions for the STLink on the NUCLEO board)<br/>
  `ACTION=="add|change", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="664", GROUP="plugdev"`

## Build and run

* clone this repository
* connect the NUCLEO board
* `$ make install`
* watch the green LED blink while studying the reference manual

happy hacking :-)
