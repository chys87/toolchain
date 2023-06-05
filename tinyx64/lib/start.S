#if defined __x86_64__ && defined __linux__ && defined __LP64__

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
    hlt

#elif defined __aarch64__ && defined __linux__ && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#include <asm/unistd.h>

    .section    .text.startup,"ax",@progbits
    .align    4
    .globl    _start
_start:
    ldr     w0, [sp]        // argc
    add     x2, sp, #16
    add     x1, sp, #8      // argv

    # initial frame
    mov     fp, xzr     // fp is x29
    # no need to clear lr (x30)

    # environ = sp + argc + 2
    add     x2, x2, x0, lsl #3

    bl      main

    .globl  exit
exit:
    .globl  _exit
    .set    _exit, exit
    mov     x8, #__NR_exit_group
    svc     0
    brk     256

#else

# error "Unsupported architecture"

#endif

    .section    .note.GNU-stack,"",@progbits
