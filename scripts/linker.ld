/* Linker script for QEMU MPS2-AN386 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4M
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vectors :
    {
        KEEP(*(.vectors))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);
    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /* 栈空间 */
    .stack :
    {
        . = ALIGN(8);
        _stack_start = .;
        . = . + 0x8000; /* 32KB 栈空间 */
        _stack_end = .;
    } > SRAM

    /* 堆空间 */
    .heap :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = ORIGIN(SRAM) + LENGTH(SRAM) - 0x1000; /* 保留4KB */
        _heap_end = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}