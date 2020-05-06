	.text
	xorl	%eax, %eax
	movl	%eax, (%rsp)
#if ORIGINAL
	xorl	%eax, %eax
#endif
	movl	%eax, 4(%rsp)
#if ORIGINAL
	xorl	%eax, %eax
#endif
	movl	%eax, 8(%rsp)

#if ORIGINAL
	xorl	%eax, %eax
#endif
	movl	%eax, (%rsp)
	xorl	%edx, %edx
#if ORIGINAL
	xorl	%eax, %eax
#endif
	movl	%edx, 4(%rsp)
	movl	%eax, 8(%rsp)

	call	clobber_registers

	andl	$1073741824, %edx
	shlq	$2, %rdx
	andl	$1073741823, %eax
#if ORIGINAL
	shlq	$2, %rax
#else
	shll	$2, %eax
#endif

	call	clobber_registers
