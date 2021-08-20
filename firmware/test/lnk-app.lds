MEMORY
{
    HRAM  (xrw) : ORIGIN = 0x40000000, LENGTH = 0x2000000	/* 32768k */
}
ENTRY(_start)
SECTIONS {
    __stacktop = ORIGIN(HRAM) + LENGTH(HRAM);
    .text :
    {
        . = ALIGN(4);
        *(.text.start)
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
        . = ALIGN(4);
        _etext = .;
        _sidata = _etext;
    } >HRAM
    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;
        _ram_start = .;
        . = ALIGN(4);
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
        . = ALIGN(4);
        _edata = .;
    } >HRAM
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >HRAM
    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
    } >HRAM
}
