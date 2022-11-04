#if !defined __x86_64__ || !defined __linux__ || !defined __LP64__
# error "Only Linux x86-64 is supported."
#endif

#include <asm/unistd.h>

    .section    .text.startup,"ax",@progbits
    .align    16
    .globl    _start
_start:
    mov     (%rsp), %edi # argc
    lea     8(%rsp), %rsi # argv
    and     $-16, %rsp
    lea     8(%rsi, %rdi, 8), %rdx # environ
    call    main
    mov     %eax, %edi
    .align  16,,3
    .align  8,,2
    .align  2
    .globl  exit
exit:
    .globl  _exit
    .set    _exit, exit
    mov     $__NR_exit_group, %eax
    syscall
    ud2

    .section    .note.GNU-stack,"",@progbits
