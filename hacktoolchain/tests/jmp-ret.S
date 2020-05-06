	.text
	mov	$1, %eax
#if ORIGINAL
	jmp	.L2
#else
	ret
#endif
	xor	%eax, %eax

.L2:
	ret
