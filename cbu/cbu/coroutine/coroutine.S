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

#if !defined __x86_64__ || !defined __LP64__
# error "Sorry, we only support x86-64"
#endif

    .section .text.cbu_coroutine_asm,"ax",@progbits
    .globl  cbu_coroutine_switch_context

    .align 16
cbu_coroutine_switch_context:
    # RDI = from
    # RSI = to
    mov     %rbx, 0 * 8(%rdi)
    mov     %rbp, 1 * 8(%rdi)
    mov     %r12, 2 * 8(%rdi)
    mov     %r13, 3 * 8(%rdi)
    mov     %r14, 4 * 8(%rdi)
    mov     %r15, 5 * 8(%rdi)
    popq          6 * 8(%rdi)  # return addr
    mov     %rsp, 7 * 8(%rdi)  # Preserves stack address

    mov     7 * 8(%rsi), %rsp  # Restores stack address
    pushq   6 * 8(%rsi)        # return addr
    mov     5 * 8(%rsi), %r15
    mov     4 * 8(%rsi), %r14
    mov     3 * 8(%rsi), %r13
    mov     2 * 8(%rsi), %r12
    mov     1 * 8(%rsi), %rbp
    mov     0 * 8(%rsi), %rbx
    mov     8 * 8(%rsi), %rdi   # RDI isn't callee-preserved, but we set it for
                                # convenient start up of new coroutines.
    ret

    .section    .note.GNU-stack,"",@progbits
