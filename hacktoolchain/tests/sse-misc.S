	.text
#if ORIGINAL
	vpsllq	$1, %ymm1, %ymm2
#else
	vpaddq	%ymm1, %ymm1, %ymm2
#endif


#if ORIGINAL
	vmovups	(%rsi), %ymm0
	add	$64, %rsi
	add	$64, %rdi
	vcmpneq_oqps	%ymm1, %ymm0, %ymm0
#else
	vcmpneq_oqps	(%rsi), %ymm1, %ymm0
	add	$64, %rsi
	add	$64, %rdi
#endif

	vmovups	(%rsi), %ymm2
	vcmpneqps	%ymm2, %ymm2, %ymm2
