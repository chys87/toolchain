#if !defined __x86_64__ || !defined __linux__ || defined __LP64__
# error "Only \"gcc -mx32\" is supported."
#endif

#include <asm/unistd.h>

    .section    .text.startup,"ax",@progbits
    .align    16
    .globl    _start
_start:
    mov     (%rsp), %edi # argc
    lea     4(%rsp), %esi # argv
    lea     8(%rsp, %rdi, 4), %edx # environ
#if defined TX32_PREFERRED_STACK_BOUNDARY && TX32_PREFERRED_STACK_BOUNDARY > 3
    and     $-(1 << TX32_PREFERRED_STACK_BOUNDARY), %esp
#endif
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
