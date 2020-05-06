	.text
	.globl f
f:
#if ORIGINAL
	testq	%rdi, %rdi
	jz	.L1
	jmp	free
	.p2align 4
.L1:
	ret
#else
	jmp	free
#endif
