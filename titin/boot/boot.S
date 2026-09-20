.set MULTIBOOT2_MAGIC, 0xE85250D6
.set ARCHITECTURE,     0

.section .multiboot
.align 8

header:
    .long MULTIBOOT2_MAGIC
    .long ARCHITECTURE
    .long header_end - header
    .long -(MULTIBOOT2_MAGIC + ARCHITECTURE + (header_end - header))

    .short 0
    .short 0
    .long 8

header_end:

.section .bss
.align 16

stack_bottom:
    .skip 16384

stack_top:

.align 4096
pml4:
    .skip 4096

pdpt:
    .skip 4096

pd:
    .skip 4096

.section .text
.code32

.global _start
.type _start, @function

_start:
    cli

    mov $pml4, %edi
    xor %eax, %eax
    mov $3072, %ecx
    rep stosl

    mov $pdpt, %eax
    or $0x3, %eax
    mov %eax, pml4

    mov $pd, %eax
    or $0x3, %eax
    mov %eax, pdpt

    mov $0x00000083, %eax
    mov %eax, pd

    mov $0x00200083, %eax
    mov %eax, pd + 8

    mov $pml4, %eax
    mov %eax, %cr3

    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    lgdt gdt64_descriptor

    ljmp $0x08, $long_mode

.code64

long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    mov $stack_top, %rsp

    call kernel_main

halt:
    cli
    hlt
    jmp halt

.align 8

gdt64:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF

gdt64_descriptor:
    .word gdt64_descriptor - gdt64 - 1
    .quad gdt64
