// ICF (Identical code fold)

#if 0 // ICF is disabled for now
#if ORIGINAL
	.text
	.p2align 4
	.type	f, @function
f:
	.cfi_startproc
	movq	(%rdi), %rdi
	movq	(%rdi), %rdi
	jmp	free
	.cfi_endproc
	.size	f, .-f
#else
	.text
	.p2align 4
	.type	f, @function
	.type	h, @function
f:
h:
	.cfi_startproc
	movq	(%rdi), %rdi
	movq	(%rdi), %rdi
	jmp	free
	.cfi_endproc
	.size	f, .-f
	.size	h, .-h
#endif
	.text
	.p2align 4
	.type	g, @function
g:
	.cfi_startproc
	movq	(%rdi), %rdi
	jmp	free
	.cfi_endproc
	.size	g, .-g
#if ORIGINAL
	.text
	.p2align 4
	.type	h, @function
h:
	.cfi_startproc
	movq	(%rdi), %rdi
	movq	(%rdi), %rdi
	jmp	free
	.cfi_endproc
	.size	h, .-h
#endif
#endif
