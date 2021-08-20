MEMORY
{
    BRAM  (xrw) : ORIGIN = 0x00000400, LENGTH = 0x0c00		/*     3k */
    HRAM  (xrw) : ORIGIN = 0x40000000, LENGTH = 0x2000000	/* 32768k */
}
ENTRY(_start)
SECTIONS {
    __stacktop = ORIGIN(BRAM) + LENGTH(BRAM);
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
    } >BRAM
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
    } >BRAM
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
    } >BRAM
    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
    } >BRAM
}
