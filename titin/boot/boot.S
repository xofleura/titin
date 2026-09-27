.set MULTIBOOT2_MAGIC, 0xE85250D6
.set ARCHITECTURE,     0

.section .multiboot
.align 8

header:
    .long MULTIBOOT2_MAGIC
    .long ARCHITECTURE
    .long header_end - header
    .long -(MULTIBOOT2_MAGIC + ARCHITECTURE + (header_end - header))

    .short 5
    .short 0
    .long 20
    .long 1280
    .long 720
    .long 32

    .align 8

    .short 0
    .short 0
    .long 8

header_end:

.section .bss
.align 16

multiboot_magic_value:
    .skip 4

multiboot_info_value:
    .skip 4

stack_bottom:
    .skip 16384

stack_top:

.align 4096

pml4:
    .skip 4096

pdpt:
    .skip 4096

pd0:
    .skip 4096

pd1:
    .skip 4096

pd2:
    .skip 4096

pd3:
    .skip 4096

.section .text
.code32

.global _start
.type _start, @function

_start:
    cli

    mov %eax, multiboot_magic_value
    mov %ebx, multiboot_info_value

    mov $0x0A, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0x20, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov $0x0F, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0xFF, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov $0x0E, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0xFF, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov $pml4, %edi
    xor %eax, %eax
    mov $6144, %ecx
    rep stosl

    mov $pdpt, %eax
    or $0x3, %eax
    mov %eax, pml4

    mov $pd0, %eax
    or $0x3, %eax
    mov %eax, pdpt

    mov $pd1, %eax
    or $0x3, %eax
    mov %eax, pdpt + 8

    mov $pd2, %eax
    or $0x3, %eax
    mov %eax, pdpt + 16

    mov $pd3, %eax
    or $0x3, %eax
    mov %eax, pdpt + 24

    mov $pd0, %edi
    xor %eax, %eax
    call build_page_table

    mov $pd1, %edi
    mov $0x40000000, %eax
    call build_page_table

    mov $pd2, %edi
    mov $0x80000000, %eax
    call build_page_table

    mov $pd3, %edi
    mov $0xC0000000, %eax
    call build_page_table

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

build_page_table:
    push %ebx
    push %ecx

    mov %eax, %ebx
    or $0x83, %ebx

    mov $512, %ecx

build_page_table_loop:
    mov %ebx, (%edi)
    add $8, %edi
    add $0x200000, %eax
    mov %eax, %ebx
    or $0x83, %ebx
    loop build_page_table_loop

    pop %ecx
    pop %ebx

    ret

.code64

long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    mov $stack_top, %rsp

    mov $0x0A, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0x20, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov $0x0F, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0xFF, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov $0x0E, %al
    mov $0x3D4, %dx
    out %al, %dx

    mov $0xFF, %al
    mov $0x3D5, %dx
    out %al, %dx

    mov multiboot_info_value, %esi

    call clear_framebuffer

    mov multiboot_magic_value, %edi
    mov multiboot_info_value, %esi

    call kernel_main

halt:
    cli
    hlt
    jmp halt

clear_framebuffer:
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsi, %rbx

    mov (%rbx), %eax
    mov %eax, %r12d

    mov $8, %r13d

find_framebuffer_tag:
    cmp %r13d, %r12d
    jae framebuffer_done

    mov (%rbx,%r13,1), %eax
    cmp $0, %eax
    je framebuffer_done

    cmp $8, %eax
    je framebuffer_tag_found

    mov 4(%rbx,%r13,1), %eax

    test %eax, %eax
    jz framebuffer_done

    add $7, %eax
    and $0xFFFFFFF8, %eax

    add %eax, %r13d
    jmp find_framebuffer_tag

framebuffer_tag_found:
    mov 28(%rbx,%r13,1), %eax
    cmp $32, %eax
    jne framebuffer_done

    mov 8(%rbx,%r13,1), %rax
    mov %rax, %r14

    mov 16(%rbx,%r13,1), %eax
    mov %eax, %r15d

    mov 20(%rbx,%r13,1), %eax
    mov %eax, %r12d

    mov 24(%rbx,%r13,1), %eax
    mov %eax, %r13d

    mov $0x20202020, %eax

    xor %ecx, %ecx

clear_framebuffer_rows:
    cmp %r13d, %ecx
    jae framebuffer_done

    mov %r14, %rdi

    mov %ecx, %edx
    imul %r15d, %edx
    add %rdx, %rdi

    mov %r12d, %edx

    push %rcx
    push %r12
    push %r13
    push %r14
    push %r15

    mov %edx, %ecx
    rep stosl

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rcx

    inc %ecx
    jmp clear_framebuffer_rows

framebuffer_done:
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx

    ret

.align 8

gdt64:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF

gdt64_descriptor:
    .word gdt64_descriptor - gdt64 - 1
    .quad gdt64
