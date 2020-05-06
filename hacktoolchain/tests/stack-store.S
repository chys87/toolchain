	.text
	.globl f
f:
#if ORIGINAL
	movq	%rax, (%rsp)
	movq	(%rsp), %rdx
#else
	mov	%rax, (%rsp)
	mov	%rax, %rdx
#endif

#if ORIGINAL
	movq	%rax, -64(%rsp)
	movq	-64(%rsp), %rax
#else
	mov	%rax, -64(%rsp)
#endif

#if ORIGINAL
	movl	%ecx, -64(%rsp)
	movl	-64(%rsp), %ecx
#else
	mov	%ecx, -64(%rsp)
	mov	%ecx, %ecx
#endif

	movb	%cl, (%rsp)
	movl	(%rsp), %ecx

#if ORIGINAL
	vmovaps	%xmm0, (%rsp)
	movl	(%rsp),%eax
#else
	vmovaps	%xmm0, (%rsp)
	vmovd	%xmm0,%eax
#endif
