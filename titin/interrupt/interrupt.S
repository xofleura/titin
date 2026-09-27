.section .text

.global interrupt_irq1
.global interrupt_irq12
.global interrupt_common

.extern interrupt_handler

interrupt_irq1:
    pushq $33
    jmp interrupt_common

interrupt_irq12:
    pushq $44
    jmp interrupt_common

interrupt_common:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    mov 120(%rsp), %rdi

    call interrupt_handler

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    addq $8, %rsp

    iretq
