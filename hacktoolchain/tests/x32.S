	.text
	.globl e1, e2
e1:
	movl	(%edi), %edi
#if ORIGINAL
	movl	(%edi), %edi
#else
	movl	(%rdi), %edi
#endif

e2:
	movl	(%ebp), %ebp
	call	clobber_registers
#if ORIGINAL
	movl	(%ebp), %ebp
#else
	movl	(%rbp), %ebp
#endif

_ZN1AC1Ev: // A::A()
#if ORIGINAL
	movq	%rdi, %rax
#else
	movl	%edi, %eax
#endif
	ret

_ZN1AD1Ev: // A::~A()
#if ORIGINAL
	movq	%rdi, %rax
#else
	movl	%edi, %eax
#endif
	ret
