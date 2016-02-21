/*
 * STM32F401XE ARM GCC bare metal linker script file for C
 */

MEMORY {
  FLASH             (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  RAM              (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .text : {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        
        . = ALIGN(4);
        __etext = .;
        
    } > FLASH

    .data : AT (__etext) {
        __data_start__ = .;
        
        *(.data)
        
        . = ALIGN(4);
        __data_end__ = .;

    } > RAM

    .bss : {
        __bss_start__ = .;

        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
}
