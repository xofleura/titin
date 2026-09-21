ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .titin_commands : ALIGN(8)
    {
        __start_titin_commands = .;
        KEEP(*(titin_commands))
        __stop_titin_commands = .;
    }

    .amino_commands : ALIGN(8)
    {
        __start_amino_commands = .;
        KEEP(*(amino_commands))
        __stop_amino_commands = .;
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
