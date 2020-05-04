/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of chys <admin@CHYS.INFO> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY chys <admin@CHYS.INFO> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined __x86_64__

#include <asm/unistd_64.h>
#define FUTEX_WAKE_PRIVATE 0x81
#define FUTEX_WAIT_PRIVATE 0x80

    .section .text.mutex_asm,"ax",@progbits
    .align 16
    .globl cbu_mutex_unlock_wake_asm

    // This "function" preserves red zone. Returns to R8
    //; Input: RDI (ptr); R8 (ret)
    //; Output: RAX/RCX/RDX/RSI/R11 = garbage. RDI/R8 preserved
cbu_mutex_unlock_wake_asm:
    xorl    %ecx, %ecx
    movl    $FUTEX_WAKE_PRIVATE, %esi
    movl    $1, %edx
    movl    %ecx, (%rdi)
    movl    $__NR_futex, %eax
    syscall                # Destroys rax, r11, rcx
    jmp     *%r8
    ud2


    .align 16,,10
    .align 8
    .globl cbu_mutex_lock_wait_asm

    // This "function" preserves red zone. Returns to R8
    //; Input: EAX (c); RDI (ptr); R8 (ret)
    //; Output: RAX/RCX/RDX/RSI/R10/R11 = garbage. RDI/R8 preserved
cbu_mutex_lock_wait_asm:
    # More than one threads are waiting. Don't bother to spin.
    cmpb    $2, %al
    jae     4f

    # Spin a number of times
    xorl    %eax, %eax
    movl    $0x100, %ecx
3:  cmpl    %eax, (%rdi)
    jz      5f
    decl    %ecx
    pause
    jnz     3b

5:  movl    $1, %edx
    lock cmpxchgl    %edx, (%rdi)
    jz      9f

4:
    movl    $0x2, %edx
    xorq    %r10, %r10
    movl    $FUTEX_WAIT_PRIVATE, %esi
    cmpl    %edx, %eax
    jne     2f
1:
    movl    $__NR_futex, %eax
    syscall
2:
    movl    %edx, %eax
    xchg    %eax, (%rdi)
    test    %eax, %eax
    jne     1b
9:
    jmp        *%r8
    ud2


#ifdef __RTM__
    .section .text.tm_mutex_asm,"ax",@progbits
    .globl cbu_tm_mutex_fallback_lock_asm
    .align 16
    // This "function" preserves red zone. Returns to R8
    //; Input: EAX (c); RDI (ptr); R8 (ret); RCX (retry address)
    //; Output: RAX/RCX/RDX/RSI/R10/R11 = garbage. RDI/R8 preserved
cbu_tm_mutex_fallback_lock_asm:
    testb $2, %al  // If the CPU says the transaction may succeed on a retry, we honor its advice
    jnz   1f
    xorl  %eax, %eax
    movl  $1, %edx
    lock cmpxchg    %edx, (%rdi)
    jne   cbu_mutex_lock_wait_asm
    jmp   *%r8
    ud2
    .align 16
1:
    jmp        *%rcx
    ud2

    .globl cbu_tm_mutex_fallback_unlock_asm
    .align 16
    // This "function" preserves red zone. Returns to R8
    //; Input: RDI (ptr); R8 (ret)
    //; Output: RAX/RCX/RDX/RSI/R11 = garbage. RDI/R8 preserved
cbu_tm_mutex_fallback_unlock_asm:
    lock decl (%rdi)
    jnz     cbu_mutex_unlock_wake_asm
    jmpq    *%r8
    ud2

#endif // __RTM__
#endif // __x86_64__

    .section    .note.GNU-stack,"",@progbits
