	.text

	xor	%eax, %eax
#if ORIGINAL
	jmp	.L3
#endif
	.p2align	4
.L3:

	testl	%ecx, %ecx
#if ORIGINAL
	jz	.L4
#endif
	.p2align	4
.L4:
	rep ret
