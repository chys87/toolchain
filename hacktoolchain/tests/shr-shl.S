	.text
	.globl f, g
f:
#if ORIGINAL
	shr	$3, %eax
	shl	$3, %eax
#else
	andl	$-8, %eax
#endif
	ret

g:
#if ORIGINAL
	shr	%ecx
	shl	%ecx
#else
	andl	$-2, %ecx
#endif
	ret
