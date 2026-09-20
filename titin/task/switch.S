.section .text

.global task_switch
.type task_switch, @function

task_switch:
    mov (%rsp), %rax
    mov %rsp, 0(%rdi)
    mov %rbp, 8(%rdi)
    mov %rbx, 16(%rdi)
    mov %r12, 24(%rdi)
    mov %r13, 32(%rdi)
    mov %r14, 40(%rdi)
    mov %r15, 48(%rdi)
    mov %rax, 56(%rdi)

    mov 8(%rsi), %rbp
    mov 16(%rsi), %rbx
    mov 24(%rsi), %r12
    mov 32(%rsi), %r13
    mov 40(%rsi), %r14
    mov 48(%rsi), %r15
    mov 0(%rsi), %rsp
    mov 56(%rsi), %rax

    jmp *%rax

.global task_bootstrap
.type task_bootstrap, @function

task_bootstrap:
    push %rbp
    mov %rsp, %rbp

    call task_current
    mov %rax, %rdi

    mov 24(%rdi), %rax
    call *%rax

    call task_finish

1:
    cli
    hlt
    jmp 1b
