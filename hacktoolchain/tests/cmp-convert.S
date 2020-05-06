	.text
f1:
#if ORIGINAL
	cmpl	$5, %eax
	jbe		.L2
#else
	cmpl	$6, %eax
	jb		.L2
#endif
	dec	%eax
	ret
.L2:
	inc	%eax
	ret

f2:
	cmpl	$10, %eax
	jbe		.L2
	adcl	%eax, %eax
	ret

f3:
	xorl	%eax, %eax
#if ORIGINAL
	cmpl	$0, %edx
#else
	testl	%edx, %edx
#endif
	setc	%al
	ret
