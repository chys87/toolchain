.text
#if ORIGINAL
	repne
	scasb
#else
	repnz	scasb
#endif

#if ORIGINAL
	rep
	ret
#else
	rep ret
#endif
