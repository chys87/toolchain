	.text
	.globl test
test:
	andq	$-2, %rax
#if ORIGINAL
	andq	$2, %rax
#else
	andl	$2, %eax
#endif

	call clobber_registers

#if ORIGINAL
	andq	$-5,(%rax)
	orq		$4,(%rax)
#else
	andl	$-5,(%rax)
	orl		$4,(%rax)
#endif

	call clobber_registers

#if ORIGINAL
	andl	$255, %eax
	andl	$65535, %ecx
	andl	$-1, %edx
	andq	$0xffffffff, %rsi
#else
	movzx	%al, %eax
	movzx	%cx, %ecx
	mov		%edx, %edx
	mov		%esi, %esi
#endif

	call clobber_registers

	// Not equiv. to "not %ax"!! Different effect on high 32 bits of %rax
	xorl	$65535, %eax
	pmovmskb %xmm0, %ecx
#if ORIGINAL
	xorl	$65535, %ecx
#else // This one is OK because we know the high 32 bits are clear
	not %cx
#endif

	call clobber_registers

	ret
