/* Simple linker script for QEMU MPS2-AN386 */

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
        *(.vectors)
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data :
    {
        *(.data*)
    } > SRAM AT > FLASH

    .bss :
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _end = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}