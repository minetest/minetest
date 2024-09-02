.text
.global aes128_key_expansion
aes128_key_expansion:
  movdqu 0(%rdi), %xmm1
  mov %rsi, %rdx
  movdqu %xmm1, 0(%rdx)
  aeskeygenassist $1, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 16(%rdx)
  aeskeygenassist $2, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 32(%rdx)
  aeskeygenassist $4, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 48(%rdx)
  aeskeygenassist $8, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 64(%rdx)
  aeskeygenassist $16, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 80(%rdx)
  aeskeygenassist $32, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 96(%rdx)
  aeskeygenassist $64, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 112(%rdx)
  aeskeygenassist $128, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 128(%rdx)
  aeskeygenassist $27, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 144(%rdx)
  aeskeygenassist $54, %xmm1, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  vpslldq $4, %xmm1, %xmm3
  pxor %xmm3, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 160(%rdx)
  pxor %xmm1, %xmm1
  pxor %xmm2, %xmm2
  pxor %xmm3, %xmm3
  ret

.global aes128_keyhash_init
aes128_keyhash_init:
  mov $579005069656919567, %r8
  pinsrq $0, %r8, %xmm4
  mov $283686952306183, %r8
  pinsrq $1, %r8, %xmm4
  pxor %xmm0, %xmm0
  movdqu %xmm0, 80(%rsi)
  mov %rdi, %r8
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm4, %xmm0
  mov %rsi, %rcx
  movdqu %xmm0, 32(%rcx)
  movdqu %xmm6, %xmm0
  mov %r12, %rax
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 0(%rcx)
  movdqu %xmm6, %xmm1
  movdqu %xmm6, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 16(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 48(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 64(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 96(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 112(%rcx)
  movdqu %xmm0, %xmm6
  mov %rax, %r12
  ret

.global aes256_key_expansion
aes256_key_expansion:
  movdqu 0(%rdi), %xmm1
  movdqu 16(%rdi), %xmm3
  mov %rsi, %rdx
  movdqu %xmm1, 0(%rdx)
  movdqu %xmm3, 16(%rdx)
  aeskeygenassist $1, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 32(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 48(%rdx)
  aeskeygenassist $2, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 64(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 80(%rdx)
  aeskeygenassist $4, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 96(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 112(%rdx)
  aeskeygenassist $8, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 128(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 144(%rdx)
  aeskeygenassist $16, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 160(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 176(%rdx)
  aeskeygenassist $32, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 192(%rdx)
  aeskeygenassist $0, %xmm1, %xmm2
  pshufd $170, %xmm2, %xmm2
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  vpslldq $4, %xmm3, %xmm4
  pxor %xmm4, %xmm3
  pxor %xmm2, %xmm3
  movdqu %xmm3, 208(%rdx)
  aeskeygenassist $64, %xmm3, %xmm2
  pshufd $255, %xmm2, %xmm2
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  vpslldq $4, %xmm1, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm2, %xmm1
  movdqu %xmm1, 224(%rdx)
  pxor %xmm1, %xmm1
  pxor %xmm2, %xmm2
  pxor %xmm3, %xmm3
  pxor %xmm4, %xmm4
  ret

.global aes256_keyhash_init
aes256_keyhash_init:
  mov $579005069656919567, %r8
  pinsrq $0, %r8, %xmm4
  mov $283686952306183, %r8
  pinsrq $1, %r8, %xmm4
  pxor %xmm0, %xmm0
  movdqu %xmm0, 80(%rsi)
  mov %rdi, %r8
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm4, %xmm0
  mov %rsi, %rcx
  movdqu %xmm0, 32(%rcx)
  movdqu %xmm6, %xmm0
  mov %r12, %rax
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 0(%rcx)
  movdqu %xmm6, %xmm1
  movdqu %xmm6, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 16(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 48(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 64(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 96(%rcx)
  movdqu %xmm6, %xmm2
  movdqu 32(%rcx), %xmm1
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm6
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  movdqu %xmm1, %xmm5
  pclmulqdq $16, %xmm2, %xmm1
  movdqu %xmm1, %xmm3
  movdqu %xmm5, %xmm1
  pclmulqdq $1, %xmm2, %xmm1
  movdqu %xmm1, %xmm4
  movdqu %xmm5, %xmm1
  pclmulqdq $0, %xmm2, %xmm1
  pclmulqdq $17, %xmm2, %xmm5
  movdqu %xmm5, %xmm2
  movdqu %xmm1, %xmm5
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm4, %xmm1
  mov $0, %r12
  pinsrd $0, %r12d, %xmm1
  pshufd $14, %xmm1, %xmm1
  pxor %xmm1, %xmm2
  movdqu %xmm3, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm1
  pshufd $79, %xmm1, %xmm1
  mov $0, %r12
  pinsrd $3, %r12d, %xmm4
  pshufd $79, %xmm4, %xmm4
  pxor %xmm4, %xmm1
  pxor %xmm5, %xmm1
  movdqu %xmm1, %xmm3
  psrld $31, %xmm3
  movdqu %xmm2, %xmm4
  psrld $31, %xmm4
  pslld $1, %xmm1
  pslld $1, %xmm2
  vpslldq $4, %xmm3, %xmm5
  vpslldq $4, %xmm4, %xmm4
  mov $0, %r12
  pinsrd $0, %r12d, %xmm3
  pshufd $3, %xmm3, %xmm3
  pxor %xmm4, %xmm3
  pxor %xmm5, %xmm1
  pxor %xmm3, %xmm2
  movdqu %xmm2, %xmm5
  pxor %xmm2, %xmm2
  mov $3774873600, %r12
  pinsrd $3, %r12d, %xmm2
  pclmulqdq $17, %xmm2, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pxor %xmm5, %xmm1
  pxor %xmm6, %xmm1
  movdqu %xmm1, %xmm6
  movdqu %xmm1, %xmm3
  pxor %xmm4, %xmm4
  pxor %xmm5, %xmm5
  mov $3254779904, %r12
  pinsrd $3, %r12d, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  mov $2147483648, %r12
  pinsrd $3, %r12d, %xmm5
  movdqu %xmm3, %xmm1
  movdqu %xmm1, %xmm2
  psrld $31, %xmm2
  pslld $1, %xmm1
  vpslldq $4, %xmm2, %xmm2
  pxor %xmm2, %xmm1
  pand %xmm5, %xmm3
  pcmpeqd %xmm5, %xmm3
  pshufd $255, %xmm3, %xmm3
  pand %xmm4, %xmm3
  vpxor %xmm3, %xmm1, %xmm1
  movdqu %xmm1, 112(%rcx)
  movdqu %xmm0, %xmm6
  mov %rax, %r12
  ret

.global gctr128_bytes
gctr128_bytes:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  movdqu 0(%r9), %xmm7
  mov %rdi, %rax
  mov %rdx, %rbx
  mov %rcx, %r13
  mov 72(%rsp), %rcx
  mov %rcx, %rbp
  imul $16, %rbp
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm8
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm8
  mov %rcx, %rdx
  shr $2, %rdx
  and $3, %rcx
  cmp $0, %rdx
  jbe L0
  mov %rax, %r9
  mov %rbx, %r10
  pshufb %xmm8, %xmm7
  movdqu %xmm7, %xmm9
  mov $579005069656919567, %rax
  pinsrq $0, %rax, %xmm0
  mov $579005069656919567, %rax
  pinsrq $1, %rax, %xmm0
  pshufb %xmm0, %xmm9
  movdqu %xmm9, %xmm10
  pxor %xmm3, %xmm3
  mov $1, %rax
  pinsrd $2, %eax, %xmm3
  paddd %xmm3, %xmm9
  mov $3, %rax
  pinsrd $2, %eax, %xmm3
  mov $2, %rax
  pinsrd $0, %eax, %xmm3
  paddd %xmm3, %xmm10
  pshufb %xmm8, %xmm9
  pshufb %xmm8, %xmm10
  pextrq $0, %xmm7, %rdi
  mov $283686952306183, %rax
  pinsrq $0, %rax, %xmm0
  mov $579005069656919567, %rax
  pinsrq $1, %rax, %xmm0
  pxor %xmm15, %xmm15
  mov $4, %rax
  pinsrd $0, %eax, %xmm15
  mov $4, %rax
  pinsrd $2, %eax, %xmm15
  jmp L3
.balign 16
L2:
  pinsrq $0, %rdi, %xmm2
  pinsrq $0, %rdi, %xmm12
  pinsrq $0, %rdi, %xmm13
  pinsrq $0, %rdi, %xmm14
  shufpd $2, %xmm9, %xmm2
  shufpd $0, %xmm9, %xmm12
  shufpd $2, %xmm10, %xmm13
  shufpd $0, %xmm10, %xmm14
  pshufb %xmm0, %xmm9
  pshufb %xmm0, %xmm10
  movdqu 0(%r8), %xmm3
  movdqu 16(%r8), %xmm4
  movdqu 32(%r8), %xmm5
  movdqu 48(%r8), %xmm6
  paddd %xmm15, %xmm9
  paddd %xmm15, %xmm10
  pxor %xmm3, %xmm2
  pxor %xmm3, %xmm12
  pxor %xmm3, %xmm13
  pxor %xmm3, %xmm14
  pshufb %xmm0, %xmm9
  pshufb %xmm0, %xmm10
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenc %xmm5, %xmm2
  aesenc %xmm5, %xmm12
  aesenc %xmm5, %xmm13
  aesenc %xmm5, %xmm14
  aesenc %xmm6, %xmm2
  aesenc %xmm6, %xmm12
  aesenc %xmm6, %xmm13
  aesenc %xmm6, %xmm14
  movdqu 64(%r8), %xmm3
  movdqu 80(%r8), %xmm4
  movdqu 96(%r8), %xmm5
  movdqu 112(%r8), %xmm6
  aesenc %xmm3, %xmm2
  aesenc %xmm3, %xmm12
  aesenc %xmm3, %xmm13
  aesenc %xmm3, %xmm14
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenc %xmm5, %xmm2
  aesenc %xmm5, %xmm12
  aesenc %xmm5, %xmm13
  aesenc %xmm5, %xmm14
  aesenc %xmm6, %xmm2
  aesenc %xmm6, %xmm12
  aesenc %xmm6, %xmm13
  aesenc %xmm6, %xmm14
  movdqu 128(%r8), %xmm3
  movdqu 144(%r8), %xmm4
  movdqu 160(%r8), %xmm5
  aesenc %xmm3, %xmm2
  aesenc %xmm3, %xmm12
  aesenc %xmm3, %xmm13
  aesenc %xmm3, %xmm14
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenclast %xmm5, %xmm2
  aesenclast %xmm5, %xmm12
  aesenclast %xmm5, %xmm13
  aesenclast %xmm5, %xmm14
  movdqu 0(%r9), %xmm7
  pxor %xmm7, %xmm2
  movdqu 16(%r9), %xmm7
  pxor %xmm7, %xmm12
  movdqu 32(%r9), %xmm7
  pxor %xmm7, %xmm13
  movdqu 48(%r9), %xmm7
  pxor %xmm7, %xmm14
  movdqu %xmm2, 0(%r10)
  movdqu %xmm12, 16(%r10)
  movdqu %xmm13, 32(%r10)
  movdqu %xmm14, 48(%r10)
  sub $1, %rdx
  add $64, %r9
  add $64, %r10
.balign 16
L3:
  cmp $0, %rdx
  ja L2
  movdqu %xmm9, %xmm7
  pinsrq $0, %rdi, %xmm7
  pshufb %xmm8, %xmm7
  mov %r9, %rax
  mov %r10, %rbx
  jmp L1
L0:
L1:
  mov $0, %rdx
  mov %rax, %r9
  mov %rbx, %r10
  pxor %xmm4, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  jmp L5
.balign 16
L4:
  movdqu %xmm7, %xmm0
  pshufb %xmm8, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r9), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rdx
  add $16, %r9
  add $16, %r10
  paddd %xmm4, %xmm7
.balign 16
L5:
  cmp %rcx, %rdx
  jne L4
  cmp %rbp, %rsi
  jbe L6
  movdqu 0(%r13), %xmm1
  movdqu %xmm7, %xmm0
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm2
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm2
  pshufb %xmm2, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pxor %xmm0, %xmm1
  movdqu %xmm1, 0(%r13)
  jmp L7
L6:
L7:
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  ret

.global gctr256_bytes
gctr256_bytes:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  movdqu 0(%r9), %xmm7
  mov %rdi, %rax
  mov %rdx, %rbx
  mov %rcx, %r13
  mov 72(%rsp), %rcx
  mov %rcx, %rbp
  imul $16, %rbp
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm8
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm8
  mov %rcx, %rdx
  shr $2, %rdx
  and $3, %rcx
  cmp $0, %rdx
  jbe L8
  mov %rax, %r9
  mov %rbx, %r10
  pshufb %xmm8, %xmm7
  movdqu %xmm7, %xmm9
  mov $579005069656919567, %rax
  pinsrq $0, %rax, %xmm0
  mov $579005069656919567, %rax
  pinsrq $1, %rax, %xmm0
  pshufb %xmm0, %xmm9
  movdqu %xmm9, %xmm10
  pxor %xmm3, %xmm3
  mov $1, %rax
  pinsrd $2, %eax, %xmm3
  paddd %xmm3, %xmm9
  mov $3, %rax
  pinsrd $2, %eax, %xmm3
  mov $2, %rax
  pinsrd $0, %eax, %xmm3
  paddd %xmm3, %xmm10
  pshufb %xmm8, %xmm9
  pshufb %xmm8, %xmm10
  pextrq $0, %xmm7, %rdi
  mov $283686952306183, %rax
  pinsrq $0, %rax, %xmm0
  mov $579005069656919567, %rax
  pinsrq $1, %rax, %xmm0
  pxor %xmm15, %xmm15
  mov $4, %rax
  pinsrd $0, %eax, %xmm15
  mov $4, %rax
  pinsrd $2, %eax, %xmm15
  jmp L11
.balign 16
L10:
  pinsrq $0, %rdi, %xmm2
  pinsrq $0, %rdi, %xmm12
  pinsrq $0, %rdi, %xmm13
  pinsrq $0, %rdi, %xmm14
  shufpd $2, %xmm9, %xmm2
  shufpd $0, %xmm9, %xmm12
  shufpd $2, %xmm10, %xmm13
  shufpd $0, %xmm10, %xmm14
  pshufb %xmm0, %xmm9
  pshufb %xmm0, %xmm10
  movdqu 0(%r8), %xmm3
  movdqu 16(%r8), %xmm4
  movdqu 32(%r8), %xmm5
  movdqu 48(%r8), %xmm6
  paddd %xmm15, %xmm9
  paddd %xmm15, %xmm10
  pxor %xmm3, %xmm2
  pxor %xmm3, %xmm12
  pxor %xmm3, %xmm13
  pxor %xmm3, %xmm14
  pshufb %xmm0, %xmm9
  pshufb %xmm0, %xmm10
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenc %xmm5, %xmm2
  aesenc %xmm5, %xmm12
  aesenc %xmm5, %xmm13
  aesenc %xmm5, %xmm14
  aesenc %xmm6, %xmm2
  aesenc %xmm6, %xmm12
  aesenc %xmm6, %xmm13
  aesenc %xmm6, %xmm14
  movdqu 64(%r8), %xmm3
  movdqu 80(%r8), %xmm4
  movdqu 96(%r8), %xmm5
  movdqu 112(%r8), %xmm6
  aesenc %xmm3, %xmm2
  aesenc %xmm3, %xmm12
  aesenc %xmm3, %xmm13
  aesenc %xmm3, %xmm14
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenc %xmm5, %xmm2
  aesenc %xmm5, %xmm12
  aesenc %xmm5, %xmm13
  aesenc %xmm5, %xmm14
  aesenc %xmm6, %xmm2
  aesenc %xmm6, %xmm12
  aesenc %xmm6, %xmm13
  aesenc %xmm6, %xmm14
  movdqu 128(%r8), %xmm3
  movdqu 144(%r8), %xmm4
  movdqu 160(%r8), %xmm5
  aesenc %xmm3, %xmm2
  aesenc %xmm3, %xmm12
  aesenc %xmm3, %xmm13
  aesenc %xmm3, %xmm14
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  movdqu %xmm5, %xmm3
  movdqu 176(%r8), %xmm4
  movdqu 192(%r8), %xmm5
  movdqu 208(%r8), %xmm6
  aesenc %xmm3, %xmm2
  aesenc %xmm3, %xmm12
  aesenc %xmm3, %xmm13
  aesenc %xmm3, %xmm14
  aesenc %xmm4, %xmm2
  aesenc %xmm4, %xmm12
  aesenc %xmm4, %xmm13
  aesenc %xmm4, %xmm14
  aesenc %xmm5, %xmm2
  aesenc %xmm5, %xmm12
  aesenc %xmm5, %xmm13
  aesenc %xmm5, %xmm14
  aesenc %xmm6, %xmm2
  aesenc %xmm6, %xmm12
  aesenc %xmm6, %xmm13
  aesenc %xmm6, %xmm14
  movdqu 224(%r8), %xmm5
  aesenclast %xmm5, %xmm2
  aesenclast %xmm5, %xmm12
  aesenclast %xmm5, %xmm13
  aesenclast %xmm5, %xmm14
  movdqu 0(%r9), %xmm7
  pxor %xmm7, %xmm2
  movdqu 16(%r9), %xmm7
  pxor %xmm7, %xmm12
  movdqu 32(%r9), %xmm7
  pxor %xmm7, %xmm13
  movdqu 48(%r9), %xmm7
  pxor %xmm7, %xmm14
  movdqu %xmm2, 0(%r10)
  movdqu %xmm12, 16(%r10)
  movdqu %xmm13, 32(%r10)
  movdqu %xmm14, 48(%r10)
  sub $1, %rdx
  add $64, %r9
  add $64, %r10
.balign 16
L11:
  cmp $0, %rdx
  ja L10
  movdqu %xmm9, %xmm7
  pinsrq $0, %rdi, %xmm7
  pshufb %xmm8, %xmm7
  mov %r9, %rax
  mov %r10, %rbx
  jmp L9
L8:
L9:
  mov $0, %rdx
  mov %rax, %r9
  mov %rbx, %r10
  pxor %xmm4, %xmm4
  mov $1, %r12
  pinsrd $0, %r12d, %xmm4
  jmp L13
.balign 16
L12:
  movdqu %xmm7, %xmm0
  pshufb %xmm8, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r9), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rdx
  add $16, %r9
  add $16, %r10
  paddd %xmm4, %xmm7
.balign 16
L13:
  cmp %rcx, %rdx
  jne L12
  cmp %rbp, %rsi
  jbe L14
  movdqu 0(%r13), %xmm1
  movdqu %xmm7, %xmm0
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm2
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm2
  pshufb %xmm2, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pxor %xmm0, %xmm1
  movdqu %xmm1, 0(%r13)
  jmp L15
L14:
L15:
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  ret

.global compute_iv_stdcall
compute_iv_stdcall:
  cmp $12, %rsi
  jne L16
  cmp $12, %rsi
  jne L18
  movdqu 0(%r8), %xmm0
  mov $579005069656919567, %rax
  pinsrq $0, %rax, %xmm1
  mov $283686952306183, %rax
  pinsrq $1, %rax, %xmm1
  pshufb %xmm1, %xmm0
  mov $1, %rax
  pinsrd $0, %eax, %xmm0
  movdqu %xmm0, 0(%rcx)
  jmp L19
L18:
  mov %rcx, %rax
  add $32, %r9
  mov %r8, %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L21
.balign 16
L20:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L21:
  cmp $6, %rdx
  jae L20
  cmp $0, %rdx
  jbe L22
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L24
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L25
L24:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L26
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L28
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L30
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L31
L30:
L31:
  jmp L29
L28:
L29:
  jmp L27
L26:
L27:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L25:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L23
L22:
L23:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L32
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L34
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L35
L34:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L35:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L33
L32:
L33:
  mov %rax, %rcx
  mov $0, %r11
  mov %rsi, %r13
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu %xmm8, 0(%rcx)
L19:
  jmp L17
L16:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  cmp $12, %rsi
  jne L36
  movdqu 0(%r8), %xmm0
  mov $579005069656919567, %rax
  pinsrq $0, %rax, %xmm1
  mov $283686952306183, %rax
  pinsrq $1, %rax, %xmm1
  pshufb %xmm1, %xmm0
  mov $1, %rax
  pinsrd $0, %eax, %xmm0
  movdqu %xmm0, 0(%rcx)
  jmp L37
L36:
  mov %rcx, %rax
  add $32, %r9
  mov %r8, %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L39
.balign 16
L38:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L39:
  cmp $6, %rdx
  jae L38
  cmp $0, %rdx
  jbe L40
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L42
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L43
L42:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L44
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L46
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L48
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L49
L48:
L49:
  jmp L47
L46:
L47:
  jmp L45
L44:
L45:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L43:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L41
L40:
L41:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L50
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L52
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L53
L52:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L53:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L51
L50:
L51:
  mov %rax, %rcx
  mov $0, %r11
  mov %rsi, %r13
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu %xmm8, 0(%rcx)
L37:
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
L17:
  ret

.global gcm128_encrypt_opt
gcm128_encrypt_opt:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  mov 144(%rsp), %rbp
  mov %rcx, %r13
  lea 32(%r9), %r9
  mov 72(%rsp), %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L55
.balign 16
L54:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L55:
  cmp $6, %rdx
  jae L54
  cmp $0, %rdx
  jbe L56
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L58
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L59
L58:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L60
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L62
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L64
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L65
L64:
L65:
  jmp L63
L62:
L63:
  jmp L61
L60:
L61:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L59:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L57
L56:
L57:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L66
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L68
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L69
L68:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L69:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L67
L66:
L67:
  mov 80(%rsp), %rdi
  mov 88(%rsp), %rsi
  mov 96(%rsp), %rdx
  mov %r13, %rcx
  movdqu %xmm9, %xmm0
  movdqu 0(%r8), %xmm1
  movdqu %xmm1, 0(%rbp)
  pxor %xmm10, %xmm10
  mov $1, %r11
  pinsrq $0, %r11, %xmm10
  vpaddd %xmm10, %xmm1, %xmm1
  cmp $0, %rdx
  jne L70
  vpshufb %xmm0, %xmm1, %xmm1
  movdqu %xmm1, 32(%rbp)
  jmp L71
L70:
  movdqu %xmm8, 32(%rbp)
  add $128, %rcx
  pextrq $0, %xmm1, %rbx
  and $255, %rbx
  vpshufb %xmm0, %xmm1, %xmm1
  lea 96(%rsi), %r14
  movdqu -128(%rcx), %xmm4
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  movdqu -112(%rcx), %xmm15
  mov %rcx, %r12
  sub $96, %r12
  vpxor %xmm4, %xmm1, %xmm9
  add $6, %rbx
  cmp $256, %rbx
  jae L72
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm2, %xmm11, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm2, %xmm12, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpaddd %xmm2, %xmm14, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
  jmp L73
L72:
  sub $256, %rbx
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpshufb %xmm0, %xmm14, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpshufb %xmm0, %xmm1, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
L73:
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 16(%rcx), %xmm15
  movdqu 32(%rcx), %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 0(%rdi), %xmm3, %xmm4
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor 16(%rdi), %xmm3, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 32(%rdi), %xmm3, %xmm6
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 48(%rdi), %xmm3, %xmm8
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 64(%rdi), %xmm3, %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 80(%rdi), %xmm3, %xmm3
  lea 96(%rdi), %rdi
  vaesenclast %xmm4, %xmm9, %xmm9
  vaesenclast %xmm5, %xmm10, %xmm10
  vaesenclast %xmm6, %xmm11, %xmm11
  vaesenclast %xmm8, %xmm12, %xmm12
  vaesenclast %xmm2, %xmm13, %xmm13
  vaesenclast %xmm3, %xmm14, %xmm14
  movdqu %xmm9, 0(%rsi)
  movdqu %xmm10, 16(%rsi)
  movdqu %xmm11, 32(%rsi)
  movdqu %xmm12, 48(%rsi)
  movdqu %xmm13, 64(%rsi)
  movdqu %xmm14, 80(%rsi)
  lea 96(%rsi), %rsi
  vpshufb %xmm0, %xmm9, %xmm8
  vpshufb %xmm0, %xmm10, %xmm2
  movdqu %xmm8, 112(%rbp)
  vpshufb %xmm0, %xmm11, %xmm4
  movdqu %xmm2, 96(%rbp)
  vpshufb %xmm0, %xmm12, %xmm5
  movdqu %xmm4, 80(%rbp)
  vpshufb %xmm0, %xmm13, %xmm6
  movdqu %xmm5, 64(%rbp)
  vpshufb %xmm0, %xmm14, %xmm7
  movdqu %xmm6, 48(%rbp)
  movdqu -128(%rcx), %xmm4
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  movdqu -112(%rcx), %xmm15
  mov %rcx, %r12
  sub $96, %r12
  vpxor %xmm4, %xmm1, %xmm9
  add $6, %rbx
  cmp $256, %rbx
  jae L74
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm2, %xmm11, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm2, %xmm12, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpaddd %xmm2, %xmm14, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
  jmp L75
L74:
  sub $256, %rbx
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpshufb %xmm0, %xmm14, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpshufb %xmm0, %xmm1, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
L75:
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 16(%rcx), %xmm15
  movdqu 32(%rcx), %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 0(%rdi), %xmm3, %xmm4
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor 16(%rdi), %xmm3, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 32(%rdi), %xmm3, %xmm6
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 48(%rdi), %xmm3, %xmm8
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 64(%rdi), %xmm3, %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 80(%rdi), %xmm3, %xmm3
  lea 96(%rdi), %rdi
  vaesenclast %xmm4, %xmm9, %xmm9
  vaesenclast %xmm5, %xmm10, %xmm10
  vaesenclast %xmm6, %xmm11, %xmm11
  vaesenclast %xmm8, %xmm12, %xmm12
  vaesenclast %xmm2, %xmm13, %xmm13
  vaesenclast %xmm3, %xmm14, %xmm14
  movdqu %xmm9, 0(%rsi)
  movdqu %xmm10, 16(%rsi)
  movdqu %xmm11, 32(%rsi)
  movdqu %xmm12, 48(%rsi)
  movdqu %xmm13, 64(%rsi)
  movdqu %xmm14, 80(%rsi)
  lea 96(%rsi), %rsi
  sub $12, %rdx
  movdqu 32(%rbp), %xmm8
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vpxor %xmm4, %xmm4, %xmm4
  movdqu -128(%rcx), %xmm15
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpaddd %xmm2, %xmm11, %xmm12
  vpaddd %xmm2, %xmm12, %xmm13
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm4, 16(%rbp)
  jmp L77
.balign 16
L76:
  add $6, %rbx
  cmp $256, %rbx
  jb L78
  mov $579005069656919567, %r11
  pinsrq $0, %r11, %xmm0
  mov $283686952306183, %r11
  pinsrq $1, %r11, %xmm0
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  movdqu -32(%r9), %xmm3
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm15, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm15, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpshufb %xmm0, %xmm14, %xmm14
  vpshufb %xmm0, %xmm1, %xmm1
  sub $256, %rbx
  jmp L79
L78:
  movdqu -32(%r9), %xmm3
  vpaddd %xmm14, %xmm2, %xmm1
  vpxor %xmm15, %xmm10, %xmm10
  vpxor %xmm15, %xmm11, %xmm11
L79:
  movdqu %xmm1, 128(%rbp)
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  vpxor %xmm15, %xmm12, %xmm12
  movdqu -112(%rcx), %xmm2
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vaesenc %xmm2, %xmm9, %xmm9
  movdqu 48(%rbp), %xmm0
  vpxor %xmm15, %xmm13, %xmm13
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vaesenc %xmm2, %xmm10, %xmm10
  vpxor %xmm15, %xmm14, %xmm14
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  vaesenc %xmm2, %xmm11, %xmm11
  movdqu -16(%r9), %xmm3
  vaesenc %xmm2, %xmm12, %xmm12
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vaesenc %xmm2, %xmm13, %xmm13
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vaesenc %xmm2, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 88(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 80(%r14), %r12
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 32(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 40(%rbp)
  movdqu 16(%r9), %xmm5
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vaesenc %xmm15, %xmm11, %xmm11
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 72(%r14), %r13
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 64(%r14), %r12
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 48(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 56(%rbp)
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 56(%r14), %r13
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 48(%r14), %r12
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 64(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 72(%rbp)
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 40(%r14), %r13
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 32(%r14), %r12
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 80(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 88(%rbp)
  vpxor %xmm5, %xmm6, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor %xmm1, %xmm6, %xmm6
  movdqu -16(%rcx), %xmm15
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $13979173243358019584, %r11
  pinsrq $1, %r11, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm8, %xmm7, %xmm7
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm5, %xmm4, %xmm4
  movbeq 24(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 16(%r14), %r12
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  movq %r13, 96(%rbp)
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r12, 104(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm1
  vaesenc %xmm1, %xmm9, %xmm9
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm1, %xmm10, %xmm10
  vpsrldq $8, %xmm6, %xmm6
  vaesenc %xmm1, %xmm11, %xmm11
  vpxor %xmm6, %xmm7, %xmm7
  vaesenc %xmm1, %xmm12, %xmm12
  vpxor %xmm0, %xmm4, %xmm4
  movbeq 8(%r14), %r13
  vaesenc %xmm1, %xmm13, %xmm13
  movbeq 0(%r14), %r12
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  movdqu %xmm7, 16(%rbp)
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vaesenc %xmm15, %xmm10, %xmm10
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor 0(%rdi), %xmm1, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 16(%rdi), %xmm1, %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 32(%rdi), %xmm1, %xmm5
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 48(%rdi), %xmm1, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 64(%rdi), %xmm1, %xmm7
  vpxor 80(%rdi), %xmm1, %xmm3
  movdqu 128(%rbp), %xmm1
  vaesenclast %xmm2, %xmm9, %xmm9
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vaesenclast %xmm0, %xmm10, %xmm10
  vpaddd %xmm2, %xmm1, %xmm0
  movq %r13, 112(%rbp)
  lea 96(%rdi), %rdi
  vaesenclast %xmm5, %xmm11, %xmm11
  vpaddd %xmm2, %xmm0, %xmm5
  movq %r12, 120(%rbp)
  lea 96(%rsi), %rsi
  movdqu -128(%rcx), %xmm15
  vaesenclast %xmm6, %xmm12, %xmm12
  vpaddd %xmm2, %xmm5, %xmm6
  vaesenclast %xmm7, %xmm13, %xmm13
  vpaddd %xmm2, %xmm6, %xmm7
  vaesenclast %xmm3, %xmm14, %xmm14
  vpaddd %xmm2, %xmm7, %xmm3
  sub $6, %rdx
  add $96, %r14
  cmp $0, %rdx
  jbe L80
  movdqu %xmm9, -96(%rsi)
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm0, %xmm10
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm5, %xmm11
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm6, %xmm12
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm7, %xmm13
  movdqu %xmm14, -16(%rsi)
  movdqu %xmm3, %xmm14
  movdqu 32(%rbp), %xmm7
  jmp L81
L80:
  vpxor 16(%rbp), %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
L81:
.balign 16
L77:
  cmp $0, %rdx
  ja L76
  movdqu 32(%rbp), %xmm7
  movdqu %xmm1, 32(%rbp)
  pxor %xmm4, %xmm4
  movdqu %xmm4, 16(%rbp)
  movdqu -32(%r9), %xmm3
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  movdqu 48(%rbp), %xmm0
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  movdqu -16(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  movdqu 16(%r9), %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vpxor %xmm5, %xmm6, %xmm6
  vpxor %xmm1, %xmm6, %xmm6
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $3254779904, %rax
  pinsrd $3, %eax, %xmm3
  vpxor %xmm8, %xmm7, %xmm7
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm0
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm0
  movdqu %xmm9, -96(%rsi)
  vpshufb %xmm0, %xmm9, %xmm9
  vpxor %xmm7, %xmm1, %xmm1
  movdqu %xmm10, -80(%rsi)
  vpshufb %xmm0, %xmm10, %xmm10
  movdqu %xmm11, -64(%rsi)
  vpshufb %xmm0, %xmm11, %xmm11
  movdqu %xmm12, -48(%rsi)
  vpshufb %xmm0, %xmm12, %xmm12
  movdqu %xmm13, -32(%rsi)
  vpshufb %xmm0, %xmm13, %xmm13
  movdqu %xmm14, -16(%rsi)
  vpshufb %xmm0, %xmm14, %xmm14
  pxor %xmm4, %xmm4
  movdqu %xmm14, %xmm7
  movdqu %xmm4, 16(%rbp)
  movdqu %xmm13, 48(%rbp)
  movdqu %xmm12, 64(%rbp)
  movdqu %xmm11, 80(%rbp)
  movdqu %xmm10, 96(%rbp)
  movdqu %xmm9, 112(%rbp)
  movdqu -32(%r9), %xmm3
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  movdqu 48(%rbp), %xmm0
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  movdqu -16(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  movdqu 16(%r9), %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vpxor %xmm5, %xmm6, %xmm6
  vpxor %xmm1, %xmm6, %xmm6
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $3254779904, %rax
  pinsrd $3, %eax, %xmm3
  vpxor %xmm8, %xmm7, %xmm7
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  sub $128, %rcx
L71:
  movdqu 32(%rbp), %xmm11
  mov %rcx, %r8
  mov 104(%rsp), %rax
  mov 112(%rsp), %rdi
  mov 120(%rsp), %rdx
  mov %rdx, %r14
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm9
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm9
  pshufb %xmm9, %xmm11
  pxor %xmm10, %xmm10
  mov $1, %rbx
  pinsrd $0, %ebx, %xmm10
  mov %rax, %r11
  mov %rdi, %r10
  mov $0, %rbx
  jmp L83
.balign 16
L82:
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r11), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rbx
  add $16, %r11
  add $16, %r10
  paddd %xmm10, %xmm11
.balign 16
L83:
  cmp %rdx, %rbx
  jne L82
  mov %rdi, %r11
  jmp L85
.balign 16
L84:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L85:
  cmp $6, %rdx
  jae L84
  cmp $0, %rdx
  jbe L86
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L88
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L89
L88:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L90
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L92
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L94
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L95
L94:
L95:
  jmp L93
L92:
L93:
  jmp L91
L90:
L91:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L89:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L87
L86:
L87:
  add 96(%rsp), %r14
  imul $16, %r14
  mov 136(%rsp), %r13
  cmp %r14, %r13
  jbe L96
  mov 128(%rsp), %rax
  mov %r13, %r10
  and $15, %r10
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%rax), %xmm4
  pxor %xmm4, %xmm0
  movdqu %xmm0, 0(%rax)
  cmp $8, %r10
  jae L98
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L99
L98:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L99:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L97
L96:
L97:
  mov %r15, %r11
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu 0(%rbp), %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm9, %xmm8
  pxor %xmm0, %xmm8
  mov 152(%rsp), %r15
  movdqu %xmm8, 0(%r15)
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  ret

.global gcm256_encrypt_opt
gcm256_encrypt_opt:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  mov 144(%rsp), %rbp
  mov %rcx, %r13
  lea 32(%r9), %r9
  mov 72(%rsp), %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L101
.balign 16
L100:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L101:
  cmp $6, %rdx
  jae L100
  cmp $0, %rdx
  jbe L102
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L104
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L105
L104:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L106
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L108
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L110
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L111
L110:
L111:
  jmp L109
L108:
L109:
  jmp L107
L106:
L107:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L105:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L103
L102:
L103:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L112
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L114
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L115
L114:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L115:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L113
L112:
L113:
  mov 80(%rsp), %rdi
  mov 88(%rsp), %rsi
  mov 96(%rsp), %rdx
  mov %r13, %rcx
  movdqu %xmm9, %xmm0
  movdqu 0(%r8), %xmm1
  movdqu %xmm1, 0(%rbp)
  pxor %xmm10, %xmm10
  mov $1, %r11
  pinsrq $0, %r11, %xmm10
  vpaddd %xmm10, %xmm1, %xmm1
  cmp $0, %rdx
  jne L116
  vpshufb %xmm0, %xmm1, %xmm1
  movdqu %xmm1, 32(%rbp)
  jmp L117
L116:
  movdqu %xmm8, 32(%rbp)
  add $128, %rcx
  pextrq $0, %xmm1, %rbx
  and $255, %rbx
  vpshufb %xmm0, %xmm1, %xmm1
  lea 96(%rsi), %r14
  movdqu -128(%rcx), %xmm4
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  movdqu -112(%rcx), %xmm15
  mov %rcx, %r12
  sub $96, %r12
  vpxor %xmm4, %xmm1, %xmm9
  add $6, %rbx
  cmp $256, %rbx
  jae L118
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm2, %xmm11, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm2, %xmm12, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpaddd %xmm2, %xmm14, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
  jmp L119
L118:
  sub $256, %rbx
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpshufb %xmm0, %xmm14, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpshufb %xmm0, %xmm1, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
L119:
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 80(%rcx), %xmm15
  movdqu 96(%rcx), %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 0(%rdi), %xmm3, %xmm4
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor 16(%rdi), %xmm3, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 32(%rdi), %xmm3, %xmm6
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 48(%rdi), %xmm3, %xmm8
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 64(%rdi), %xmm3, %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 80(%rdi), %xmm3, %xmm3
  lea 96(%rdi), %rdi
  vaesenclast %xmm4, %xmm9, %xmm9
  vaesenclast %xmm5, %xmm10, %xmm10
  vaesenclast %xmm6, %xmm11, %xmm11
  vaesenclast %xmm8, %xmm12, %xmm12
  vaesenclast %xmm2, %xmm13, %xmm13
  vaesenclast %xmm3, %xmm14, %xmm14
  movdqu %xmm9, 0(%rsi)
  movdqu %xmm10, 16(%rsi)
  movdqu %xmm11, 32(%rsi)
  movdqu %xmm12, 48(%rsi)
  movdqu %xmm13, 64(%rsi)
  movdqu %xmm14, 80(%rsi)
  lea 96(%rsi), %rsi
  vpshufb %xmm0, %xmm9, %xmm8
  vpshufb %xmm0, %xmm10, %xmm2
  movdqu %xmm8, 112(%rbp)
  vpshufb %xmm0, %xmm11, %xmm4
  movdqu %xmm2, 96(%rbp)
  vpshufb %xmm0, %xmm12, %xmm5
  movdqu %xmm4, 80(%rbp)
  vpshufb %xmm0, %xmm13, %xmm6
  movdqu %xmm5, 64(%rbp)
  vpshufb %xmm0, %xmm14, %xmm7
  movdqu %xmm6, 48(%rbp)
  movdqu -128(%rcx), %xmm4
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  movdqu -112(%rcx), %xmm15
  mov %rcx, %r12
  sub $96, %r12
  vpxor %xmm4, %xmm1, %xmm9
  add $6, %rbx
  cmp $256, %rbx
  jae L120
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm2, %xmm11, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm2, %xmm12, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpaddd %xmm2, %xmm14, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
  jmp L121
L120:
  sub $256, %rbx
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm4, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm4, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpxor %xmm4, %xmm12, %xmm12
  vpshufb %xmm0, %xmm14, %xmm14
  vpxor %xmm4, %xmm13, %xmm13
  vpshufb %xmm0, %xmm1, %xmm1
  vpxor %xmm4, %xmm14, %xmm14
L121:
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 48(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 64(%rcx), %xmm15
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 80(%rcx), %xmm15
  movdqu 96(%rcx), %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 0(%rdi), %xmm3, %xmm4
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor 16(%rdi), %xmm3, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 32(%rdi), %xmm3, %xmm6
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 48(%rdi), %xmm3, %xmm8
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 64(%rdi), %xmm3, %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 80(%rdi), %xmm3, %xmm3
  lea 96(%rdi), %rdi
  vaesenclast %xmm4, %xmm9, %xmm9
  vaesenclast %xmm5, %xmm10, %xmm10
  vaesenclast %xmm6, %xmm11, %xmm11
  vaesenclast %xmm8, %xmm12, %xmm12
  vaesenclast %xmm2, %xmm13, %xmm13
  vaesenclast %xmm3, %xmm14, %xmm14
  movdqu %xmm9, 0(%rsi)
  movdqu %xmm10, 16(%rsi)
  movdqu %xmm11, 32(%rsi)
  movdqu %xmm12, 48(%rsi)
  movdqu %xmm13, 64(%rsi)
  movdqu %xmm14, 80(%rsi)
  lea 96(%rsi), %rsi
  sub $12, %rdx
  movdqu 32(%rbp), %xmm8
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vpxor %xmm4, %xmm4, %xmm4
  movdqu -128(%rcx), %xmm15
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpaddd %xmm2, %xmm11, %xmm12
  vpaddd %xmm2, %xmm12, %xmm13
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm4, 16(%rbp)
  jmp L123
.balign 16
L122:
  add $6, %rbx
  cmp $256, %rbx
  jb L124
  mov $579005069656919567, %r11
  pinsrq $0, %r11, %xmm0
  mov $283686952306183, %r11
  pinsrq $1, %r11, %xmm0
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  movdqu -32(%r9), %xmm3
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm15, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm15, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpshufb %xmm0, %xmm14, %xmm14
  vpshufb %xmm0, %xmm1, %xmm1
  sub $256, %rbx
  jmp L125
L124:
  movdqu -32(%r9), %xmm3
  vpaddd %xmm14, %xmm2, %xmm1
  vpxor %xmm15, %xmm10, %xmm10
  vpxor %xmm15, %xmm11, %xmm11
L125:
  movdqu %xmm1, 128(%rbp)
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  vpxor %xmm15, %xmm12, %xmm12
  movdqu -112(%rcx), %xmm2
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vaesenc %xmm2, %xmm9, %xmm9
  movdqu 48(%rbp), %xmm0
  vpxor %xmm15, %xmm13, %xmm13
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vaesenc %xmm2, %xmm10, %xmm10
  vpxor %xmm15, %xmm14, %xmm14
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  vaesenc %xmm2, %xmm11, %xmm11
  movdqu -16(%r9), %xmm3
  vaesenc %xmm2, %xmm12, %xmm12
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vaesenc %xmm2, %xmm13, %xmm13
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vaesenc %xmm2, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 88(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 80(%r14), %r12
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 32(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 40(%rbp)
  movdqu 16(%r9), %xmm5
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vaesenc %xmm15, %xmm11, %xmm11
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 72(%r14), %r13
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 64(%r14), %r12
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 48(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 56(%rbp)
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 56(%r14), %r13
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 48(%r14), %r12
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 64(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 72(%rbp)
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 40(%r14), %r13
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 32(%r14), %r12
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 80(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 88(%rbp)
  vpxor %xmm5, %xmm6, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor %xmm1, %xmm6, %xmm6
  movdqu -16(%rcx), %xmm15
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $13979173243358019584, %r11
  pinsrq $1, %r11, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm8, %xmm7, %xmm7
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm5, %xmm4, %xmm4
  movbeq 24(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 16(%r14), %r12
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  movq %r13, 96(%rbp)
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r12, 104(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm1
  vaesenc %xmm1, %xmm9, %xmm9
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm1, %xmm10, %xmm10
  vpsrldq $8, %xmm6, %xmm6
  vaesenc %xmm1, %xmm11, %xmm11
  vpxor %xmm6, %xmm7, %xmm7
  vaesenc %xmm1, %xmm12, %xmm12
  vpxor %xmm0, %xmm4, %xmm4
  movbeq 8(%r14), %r13
  vaesenc %xmm1, %xmm13, %xmm13
  movbeq 0(%r14), %r12
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  vaesenc %xmm1, %xmm9, %xmm9
  vaesenc %xmm1, %xmm10, %xmm10
  vaesenc %xmm1, %xmm11, %xmm11
  vaesenc %xmm1, %xmm12, %xmm12
  vaesenc %xmm1, %xmm13, %xmm13
  movdqu 48(%rcx), %xmm15
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 64(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  vaesenc %xmm1, %xmm9, %xmm9
  vaesenc %xmm1, %xmm10, %xmm10
  vaesenc %xmm1, %xmm11, %xmm11
  vaesenc %xmm1, %xmm12, %xmm12
  vaesenc %xmm1, %xmm13, %xmm13
  movdqu 80(%rcx), %xmm15
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 96(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  movdqu %xmm7, 16(%rbp)
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vaesenc %xmm15, %xmm10, %xmm10
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor 0(%rdi), %xmm1, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 16(%rdi), %xmm1, %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 32(%rdi), %xmm1, %xmm5
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 48(%rdi), %xmm1, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 64(%rdi), %xmm1, %xmm7
  vpxor 80(%rdi), %xmm1, %xmm3
  movdqu 128(%rbp), %xmm1
  vaesenclast %xmm2, %xmm9, %xmm9
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vaesenclast %xmm0, %xmm10, %xmm10
  vpaddd %xmm2, %xmm1, %xmm0
  movq %r13, 112(%rbp)
  lea 96(%rdi), %rdi
  vaesenclast %xmm5, %xmm11, %xmm11
  vpaddd %xmm2, %xmm0, %xmm5
  movq %r12, 120(%rbp)
  lea 96(%rsi), %rsi
  movdqu -128(%rcx), %xmm15
  vaesenclast %xmm6, %xmm12, %xmm12
  vpaddd %xmm2, %xmm5, %xmm6
  vaesenclast %xmm7, %xmm13, %xmm13
  vpaddd %xmm2, %xmm6, %xmm7
  vaesenclast %xmm3, %xmm14, %xmm14
  vpaddd %xmm2, %xmm7, %xmm3
  sub $6, %rdx
  add $96, %r14
  cmp $0, %rdx
  jbe L126
  movdqu %xmm9, -96(%rsi)
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm0, %xmm10
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm5, %xmm11
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm6, %xmm12
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm7, %xmm13
  movdqu %xmm14, -16(%rsi)
  movdqu %xmm3, %xmm14
  movdqu 32(%rbp), %xmm7
  jmp L127
L126:
  vpxor 16(%rbp), %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
L127:
.balign 16
L123:
  cmp $0, %rdx
  ja L122
  movdqu 32(%rbp), %xmm7
  movdqu %xmm1, 32(%rbp)
  pxor %xmm4, %xmm4
  movdqu %xmm4, 16(%rbp)
  movdqu -32(%r9), %xmm3
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  movdqu 48(%rbp), %xmm0
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  movdqu -16(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  movdqu 16(%r9), %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vpxor %xmm5, %xmm6, %xmm6
  vpxor %xmm1, %xmm6, %xmm6
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $3254779904, %rax
  pinsrd $3, %eax, %xmm3
  vpxor %xmm8, %xmm7, %xmm7
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm0
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm0
  movdqu %xmm9, -96(%rsi)
  vpshufb %xmm0, %xmm9, %xmm9
  vpxor %xmm7, %xmm1, %xmm1
  movdqu %xmm10, -80(%rsi)
  vpshufb %xmm0, %xmm10, %xmm10
  movdqu %xmm11, -64(%rsi)
  vpshufb %xmm0, %xmm11, %xmm11
  movdqu %xmm12, -48(%rsi)
  vpshufb %xmm0, %xmm12, %xmm12
  movdqu %xmm13, -32(%rsi)
  vpshufb %xmm0, %xmm13, %xmm13
  movdqu %xmm14, -16(%rsi)
  vpshufb %xmm0, %xmm14, %xmm14
  pxor %xmm4, %xmm4
  movdqu %xmm14, %xmm7
  movdqu %xmm4, 16(%rbp)
  movdqu %xmm13, 48(%rbp)
  movdqu %xmm12, 64(%rbp)
  movdqu %xmm11, 80(%rbp)
  movdqu %xmm10, 96(%rbp)
  movdqu %xmm9, 112(%rbp)
  movdqu -32(%r9), %xmm3
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  movdqu 48(%rbp), %xmm0
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  movdqu -16(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  movdqu 16(%r9), %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vpxor %xmm5, %xmm6, %xmm6
  vpxor %xmm1, %xmm6, %xmm6
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $3254779904, %rax
  pinsrd $3, %eax, %xmm3
  vpxor %xmm8, %xmm7, %xmm7
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  sub $128, %rcx
L117:
  movdqu 32(%rbp), %xmm11
  mov %rcx, %r8
  mov 104(%rsp), %rax
  mov 112(%rsp), %rdi
  mov 120(%rsp), %rdx
  mov %rdx, %r14
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm9
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm9
  pshufb %xmm9, %xmm11
  pxor %xmm10, %xmm10
  mov $1, %rbx
  pinsrd $0, %ebx, %xmm10
  mov %rax, %r11
  mov %rdi, %r10
  mov $0, %rbx
  jmp L129
.balign 16
L128:
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r11), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rbx
  add $16, %r11
  add $16, %r10
  paddd %xmm10, %xmm11
.balign 16
L129:
  cmp %rdx, %rbx
  jne L128
  mov %rdi, %r11
  jmp L131
.balign 16
L130:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L131:
  cmp $6, %rdx
  jae L130
  cmp $0, %rdx
  jbe L132
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L134
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L135
L134:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L136
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L138
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L140
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L141
L140:
L141:
  jmp L139
L138:
L139:
  jmp L137
L136:
L137:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L135:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L133
L132:
L133:
  add 96(%rsp), %r14
  imul $16, %r14
  mov 136(%rsp), %r13
  cmp %r14, %r13
  jbe L142
  mov 128(%rsp), %rax
  mov %r13, %r10
  and $15, %r10
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%rax), %xmm4
  pxor %xmm4, %xmm0
  movdqu %xmm0, 0(%rax)
  cmp $8, %r10
  jae L144
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L145
L144:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L145:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L143
L142:
L143:
  mov %r15, %r11
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu 0(%rbp), %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm9, %xmm8
  pxor %xmm0, %xmm8
  mov 152(%rsp), %r15
  movdqu %xmm8, 0(%r15)
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  ret

.global gcm128_decrypt_opt
gcm128_decrypt_opt:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  mov 144(%rsp), %rbp
  mov %rcx, %r13
  lea 32(%r9), %r9
  mov 72(%rsp), %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L147
.balign 16
L146:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L147:
  cmp $6, %rdx
  jae L146
  cmp $0, %rdx
  jbe L148
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L150
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L151
L150:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L152
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L154
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L156
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L157
L156:
L157:
  jmp L155
L154:
L155:
  jmp L153
L152:
L153:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L151:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L149
L148:
L149:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L158
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L160
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L161
L160:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L161:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L159
L158:
L159:
  mov 80(%rsp), %rdi
  mov 88(%rsp), %rsi
  mov 96(%rsp), %rdx
  mov %r13, %rcx
  movdqu %xmm9, %xmm0
  movdqu 0(%r8), %xmm1
  movdqu %xmm1, 0(%rbp)
  pxor %xmm10, %xmm10
  mov $1, %r11
  pinsrq $0, %r11, %xmm10
  vpaddd %xmm10, %xmm1, %xmm1
  cmp $0, %rdx
  jne L162
  vpshufb %xmm0, %xmm1, %xmm1
  movdqu %xmm1, 32(%rbp)
  jmp L163
L162:
  movdqu %xmm8, 32(%rbp)
  add $128, %rcx
  pextrq $0, %xmm1, %rbx
  and $255, %rbx
  vpshufb %xmm0, %xmm1, %xmm1
  lea 96(%rdi), %r14
  movdqu 32(%rbp), %xmm8
  movdqu 80(%rdi), %xmm7
  movdqu 64(%rdi), %xmm4
  movdqu 48(%rdi), %xmm5
  movdqu 32(%rdi), %xmm6
  vpshufb %xmm0, %xmm7, %xmm7
  movdqu 16(%rdi), %xmm2
  vpshufb %xmm0, %xmm4, %xmm4
  movdqu 0(%rdi), %xmm3
  vpshufb %xmm0, %xmm5, %xmm5
  movdqu %xmm4, 48(%rbp)
  vpshufb %xmm0, %xmm6, %xmm6
  movdqu %xmm5, 64(%rbp)
  vpshufb %xmm0, %xmm2, %xmm2
  movdqu %xmm6, 80(%rbp)
  vpshufb %xmm0, %xmm3, %xmm3
  movdqu %xmm2, 96(%rbp)
  movdqu %xmm3, 112(%rbp)
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vpxor %xmm4, %xmm4, %xmm4
  movdqu -128(%rcx), %xmm15
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpaddd %xmm2, %xmm11, %xmm12
  vpaddd %xmm2, %xmm12, %xmm13
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm4, 16(%rbp)
  cmp $6, %rdx
  jne L164
  sub $96, %r14
  jmp L165
L164:
L165:
  jmp L167
.balign 16
L166:
  add $6, %rbx
  cmp $256, %rbx
  jb L168
  mov $579005069656919567, %r11
  pinsrq $0, %r11, %xmm0
  mov $283686952306183, %r11
  pinsrq $1, %r11, %xmm0
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  movdqu -32(%r9), %xmm3
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm15, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm15, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpshufb %xmm0, %xmm14, %xmm14
  vpshufb %xmm0, %xmm1, %xmm1
  sub $256, %rbx
  jmp L169
L168:
  movdqu -32(%r9), %xmm3
  vpaddd %xmm14, %xmm2, %xmm1
  vpxor %xmm15, %xmm10, %xmm10
  vpxor %xmm15, %xmm11, %xmm11
L169:
  movdqu %xmm1, 128(%rbp)
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  vpxor %xmm15, %xmm12, %xmm12
  movdqu -112(%rcx), %xmm2
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vaesenc %xmm2, %xmm9, %xmm9
  movdqu 48(%rbp), %xmm0
  vpxor %xmm15, %xmm13, %xmm13
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vaesenc %xmm2, %xmm10, %xmm10
  vpxor %xmm15, %xmm14, %xmm14
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  vaesenc %xmm2, %xmm11, %xmm11
  movdqu -16(%r9), %xmm3
  vaesenc %xmm2, %xmm12, %xmm12
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vaesenc %xmm2, %xmm13, %xmm13
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vaesenc %xmm2, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 88(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 80(%r14), %r12
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 32(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 40(%rbp)
  movdqu 16(%r9), %xmm5
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vaesenc %xmm15, %xmm11, %xmm11
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 72(%r14), %r13
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 64(%r14), %r12
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 48(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 56(%rbp)
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 56(%r14), %r13
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 48(%r14), %r12
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 64(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 72(%rbp)
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 40(%r14), %r13
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 32(%r14), %r12
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 80(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 88(%rbp)
  vpxor %xmm5, %xmm6, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor %xmm1, %xmm6, %xmm6
  movdqu -16(%rcx), %xmm15
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $13979173243358019584, %r11
  pinsrq $1, %r11, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm8, %xmm7, %xmm7
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm5, %xmm4, %xmm4
  movbeq 24(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 16(%r14), %r12
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  movq %r13, 96(%rbp)
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r12, 104(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm1
  vaesenc %xmm1, %xmm9, %xmm9
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm1, %xmm10, %xmm10
  vpsrldq $8, %xmm6, %xmm6
  vaesenc %xmm1, %xmm11, %xmm11
  vpxor %xmm6, %xmm7, %xmm7
  vaesenc %xmm1, %xmm12, %xmm12
  vpxor %xmm0, %xmm4, %xmm4
  movbeq 8(%r14), %r13
  vaesenc %xmm1, %xmm13, %xmm13
  movbeq 0(%r14), %r12
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  movdqu %xmm7, 16(%rbp)
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vaesenc %xmm15, %xmm10, %xmm10
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor 0(%rdi), %xmm1, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 16(%rdi), %xmm1, %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 32(%rdi), %xmm1, %xmm5
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 48(%rdi), %xmm1, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 64(%rdi), %xmm1, %xmm7
  vpxor 80(%rdi), %xmm1, %xmm3
  movdqu 128(%rbp), %xmm1
  vaesenclast %xmm2, %xmm9, %xmm9
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vaesenclast %xmm0, %xmm10, %xmm10
  vpaddd %xmm2, %xmm1, %xmm0
  movq %r13, 112(%rbp)
  lea 96(%rdi), %rdi
  vaesenclast %xmm5, %xmm11, %xmm11
  vpaddd %xmm2, %xmm0, %xmm5
  movq %r12, 120(%rbp)
  lea 96(%rsi), %rsi
  movdqu -128(%rcx), %xmm15
  vaesenclast %xmm6, %xmm12, %xmm12
  vpaddd %xmm2, %xmm5, %xmm6
  vaesenclast %xmm7, %xmm13, %xmm13
  vpaddd %xmm2, %xmm6, %xmm7
  vaesenclast %xmm3, %xmm14, %xmm14
  vpaddd %xmm2, %xmm7, %xmm3
  sub $6, %rdx
  cmp $6, %rdx
  jbe L170
  add $96, %r14
  jmp L171
L170:
L171:
  cmp $0, %rdx
  jbe L172
  movdqu %xmm9, -96(%rsi)
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm0, %xmm10
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm5, %xmm11
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm6, %xmm12
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm7, %xmm13
  movdqu %xmm14, -16(%rsi)
  movdqu %xmm3, %xmm14
  movdqu 32(%rbp), %xmm7
  jmp L173
L172:
  vpxor 16(%rbp), %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
L173:
.balign 16
L167:
  cmp $0, %rdx
  ja L166
  movdqu %xmm1, 32(%rbp)
  movdqu %xmm9, -96(%rsi)
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm14, -16(%rsi)
  sub $128, %rcx
L163:
  movdqu 32(%rbp), %xmm11
  mov %rcx, %r8
  mov 104(%rsp), %rax
  mov 112(%rsp), %rdi
  mov 120(%rsp), %rdx
  mov %rdx, %r14
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm9
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm9
  pshufb %xmm9, %xmm11
  mov %rdi, %rbx
  mov %rdx, %r12
  mov %rax, %rdi
  mov %rdi, %r11
  jmp L175
.balign 16
L174:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L175:
  cmp $6, %rdx
  jae L174
  cmp $0, %rdx
  jbe L176
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L178
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L179
L178:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L180
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L182
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L184
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L185
L184:
L185:
  jmp L183
L182:
L183:
  jmp L181
L180:
L181:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L179:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L177
L176:
L177:
  mov %rbx, %rdi
  mov %r12, %rdx
  pxor %xmm10, %xmm10
  mov $1, %rbx
  pinsrd $0, %ebx, %xmm10
  mov %rax, %r11
  mov %rdi, %r10
  mov $0, %rbx
  jmp L187
.balign 16
L186:
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r11), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rbx
  add $16, %r11
  add $16, %r10
  paddd %xmm10, %xmm11
.balign 16
L187:
  cmp %rdx, %rbx
  jne L186
  add 96(%rsp), %r14
  imul $16, %r14
  mov 136(%rsp), %r13
  cmp %r14, %r13
  jbe L188
  mov 128(%rsp), %rax
  mov %r13, %r10
  and $15, %r10
  movdqu 0(%rax), %xmm0
  movdqu %xmm0, %xmm10
  cmp $8, %r10
  jae L190
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L191
L190:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L191:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pxor %xmm0, %xmm10
  movdqu %xmm10, 0(%rax)
  jmp L189
L188:
L189:
  mov %r15, %r11
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu 0(%rbp), %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm9, %xmm8
  pxor %xmm0, %xmm8
  mov 152(%rsp), %r15
  movdqu 0(%r15), %xmm0
  pcmpeqd %xmm8, %xmm0
  pextrq $0, %xmm0, %rdx
  sub $18446744073709551615, %rdx
  mov $0, %rax
  adc $0, %rax
  pextrq $1, %xmm0, %rdx
  sub $18446744073709551615, %rdx
  mov $0, %rdx
  adc $0, %rdx
  add %rdx, %rax
  mov %rax, %rcx
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  mov %rcx, %rax
  ret

.global gcm256_decrypt_opt
gcm256_decrypt_opt:
  push %r15
  push %r14
  push %r13
  push %r12
  push %rsi
  push %rdi
  push %rbp
  push %rbx
  mov 144(%rsp), %rbp
  mov %rcx, %r13
  lea 32(%r9), %r9
  mov 72(%rsp), %rbx
  mov %rdx, %rcx
  imul $16, %rcx
  mov $579005069656919567, %r10
  pinsrq $0, %r10, %xmm9
  mov $283686952306183, %r10
  pinsrq $1, %r10, %xmm9
  pxor %xmm8, %xmm8
  mov %rdi, %r11
  jmp L193
.balign 16
L192:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L193:
  cmp $6, %rdx
  jae L192
  cmp $0, %rdx
  jbe L194
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L196
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L197
L196:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L198
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L200
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L202
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L203
L202:
L203:
  jmp L201
L200:
L201:
  jmp L199
L198:
L199:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L197:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L195
L194:
L195:
  mov %rsi, %r15
  cmp %rcx, %rsi
  jbe L204
  movdqu 0(%rbx), %xmm0
  mov %rsi, %r10
  and $15, %r10
  cmp $8, %r10
  jae L206
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L207
L206:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L207:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L205
L204:
L205:
  mov 80(%rsp), %rdi
  mov 88(%rsp), %rsi
  mov 96(%rsp), %rdx
  mov %r13, %rcx
  movdqu %xmm9, %xmm0
  movdqu 0(%r8), %xmm1
  movdqu %xmm1, 0(%rbp)
  pxor %xmm10, %xmm10
  mov $1, %r11
  pinsrq $0, %r11, %xmm10
  vpaddd %xmm10, %xmm1, %xmm1
  cmp $0, %rdx
  jne L208
  vpshufb %xmm0, %xmm1, %xmm1
  movdqu %xmm1, 32(%rbp)
  jmp L209
L208:
  movdqu %xmm8, 32(%rbp)
  add $128, %rcx
  pextrq $0, %xmm1, %rbx
  and $255, %rbx
  vpshufb %xmm0, %xmm1, %xmm1
  lea 96(%rdi), %r14
  movdqu 32(%rbp), %xmm8
  movdqu 80(%rdi), %xmm7
  movdqu 64(%rdi), %xmm4
  movdqu 48(%rdi), %xmm5
  movdqu 32(%rdi), %xmm6
  vpshufb %xmm0, %xmm7, %xmm7
  movdqu 16(%rdi), %xmm2
  vpshufb %xmm0, %xmm4, %xmm4
  movdqu 0(%rdi), %xmm3
  vpshufb %xmm0, %xmm5, %xmm5
  movdqu %xmm4, 48(%rbp)
  vpshufb %xmm0, %xmm6, %xmm6
  movdqu %xmm5, 64(%rbp)
  vpshufb %xmm0, %xmm2, %xmm2
  movdqu %xmm6, 80(%rbp)
  vpshufb %xmm0, %xmm3, %xmm3
  movdqu %xmm2, 96(%rbp)
  movdqu %xmm3, 112(%rbp)
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vpxor %xmm4, %xmm4, %xmm4
  movdqu -128(%rcx), %xmm15
  vpaddd %xmm2, %xmm1, %xmm10
  vpaddd %xmm2, %xmm10, %xmm11
  vpaddd %xmm2, %xmm11, %xmm12
  vpaddd %xmm2, %xmm12, %xmm13
  vpaddd %xmm2, %xmm13, %xmm14
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm4, 16(%rbp)
  cmp $6, %rdx
  jne L210
  sub $96, %r14
  jmp L211
L210:
L211:
  jmp L213
.balign 16
L212:
  add $6, %rbx
  cmp $256, %rbx
  jb L214
  mov $579005069656919567, %r11
  pinsrq $0, %r11, %xmm0
  mov $283686952306183, %r11
  pinsrq $1, %r11, %xmm0
  vpshufb %xmm0, %xmm1, %xmm6
  pxor %xmm5, %xmm5
  mov $1, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm10
  pxor %xmm5, %xmm5
  mov $2, %r11
  pinsrq $0, %r11, %xmm5
  vpaddd %xmm5, %xmm6, %xmm11
  movdqu -32(%r9), %xmm3
  vpaddd %xmm5, %xmm10, %xmm12
  vpshufb %xmm0, %xmm10, %xmm10
  vpaddd %xmm5, %xmm11, %xmm13
  vpshufb %xmm0, %xmm11, %xmm11
  vpxor %xmm15, %xmm10, %xmm10
  vpaddd %xmm5, %xmm12, %xmm14
  vpshufb %xmm0, %xmm12, %xmm12
  vpxor %xmm15, %xmm11, %xmm11
  vpaddd %xmm5, %xmm13, %xmm1
  vpshufb %xmm0, %xmm13, %xmm13
  vpshufb %xmm0, %xmm14, %xmm14
  vpshufb %xmm0, %xmm1, %xmm1
  sub $256, %rbx
  jmp L215
L214:
  movdqu -32(%r9), %xmm3
  vpaddd %xmm14, %xmm2, %xmm1
  vpxor %xmm15, %xmm10, %xmm10
  vpxor %xmm15, %xmm11, %xmm11
L215:
  movdqu %xmm1, 128(%rbp)
  vpclmulqdq $16, %xmm3, %xmm7, %xmm5
  vpxor %xmm15, %xmm12, %xmm12
  movdqu -112(%rcx), %xmm2
  vpclmulqdq $1, %xmm3, %xmm7, %xmm6
  vaesenc %xmm2, %xmm9, %xmm9
  movdqu 48(%rbp), %xmm0
  vpxor %xmm15, %xmm13, %xmm13
  vpclmulqdq $0, %xmm3, %xmm7, %xmm1
  vaesenc %xmm2, %xmm10, %xmm10
  vpxor %xmm15, %xmm14, %xmm14
  vpclmulqdq $17, %xmm3, %xmm7, %xmm7
  vaesenc %xmm2, %xmm11, %xmm11
  movdqu -16(%r9), %xmm3
  vaesenc %xmm2, %xmm12, %xmm12
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $0, %xmm3, %xmm0, %xmm5
  vpxor %xmm4, %xmm8, %xmm8
  vaesenc %xmm2, %xmm13, %xmm13
  vpxor %xmm5, %xmm1, %xmm4
  vpclmulqdq $16, %xmm3, %xmm0, %xmm1
  vaesenc %xmm2, %xmm14, %xmm14
  movdqu -96(%rcx), %xmm15
  vpclmulqdq $1, %xmm3, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor 16(%rbp), %xmm8, %xmm8
  vpclmulqdq $17, %xmm3, %xmm0, %xmm3
  movdqu 64(%rbp), %xmm0
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 88(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 80(%r14), %r12
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 32(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 40(%rbp)
  movdqu 16(%r9), %xmm5
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -80(%rcx), %xmm15
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm3, %xmm7, %xmm7
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vaesenc %xmm15, %xmm11, %xmm11
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 80(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -64(%rcx), %xmm15
  vpxor %xmm2, %xmm6, %xmm6
  vpclmulqdq $0, %xmm1, %xmm0, %xmm2
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $16, %xmm1, %xmm0, %xmm3
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 72(%r14), %r13
  vpxor %xmm5, %xmm7, %xmm7
  vpclmulqdq $1, %xmm1, %xmm0, %xmm5
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 64(%r14), %r12
  vpclmulqdq $17, %xmm1, %xmm0, %xmm1
  movdqu 96(%rbp), %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 48(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 56(%rbp)
  vpxor %xmm2, %xmm4, %xmm4
  movdqu 64(%r9), %xmm2
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -48(%rcx), %xmm15
  vpxor %xmm3, %xmm6, %xmm6
  vpclmulqdq $0, %xmm2, %xmm0, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm2, %xmm0, %xmm5
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 56(%r14), %r13
  vpxor %xmm1, %xmm7, %xmm7
  vpclmulqdq $1, %xmm2, %xmm0, %xmm1
  vpxor 112(%rbp), %xmm8, %xmm8
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 48(%r14), %r12
  vpclmulqdq $17, %xmm2, %xmm0, %xmm2
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 64(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 72(%rbp)
  vpxor %xmm3, %xmm4, %xmm4
  movdqu 80(%r9), %xmm3
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu -32(%rcx), %xmm15
  vpxor %xmm5, %xmm6, %xmm6
  vpclmulqdq $16, %xmm3, %xmm8, %xmm5
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm1, %xmm6, %xmm6
  vpclmulqdq $1, %xmm3, %xmm8, %xmm1
  vaesenc %xmm15, %xmm10, %xmm10
  movbeq 40(%r14), %r13
  vpxor %xmm2, %xmm7, %xmm7
  vpclmulqdq $0, %xmm3, %xmm8, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 32(%r14), %r12
  vpclmulqdq $17, %xmm3, %xmm8, %xmm8
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r13, 80(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  movq %r12, 88(%rbp)
  vpxor %xmm5, %xmm6, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor %xmm1, %xmm6, %xmm6
  movdqu -16(%rcx), %xmm15
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm2, %xmm4, %xmm4
  pxor %xmm3, %xmm3
  mov $13979173243358019584, %r11
  pinsrq $1, %r11, %xmm3
  vaesenc %xmm15, %xmm9, %xmm9
  vpxor %xmm8, %xmm7, %xmm7
  vaesenc %xmm15, %xmm10, %xmm10
  vpxor %xmm5, %xmm4, %xmm4
  movbeq 24(%r14), %r13
  vaesenc %xmm15, %xmm11, %xmm11
  movbeq 16(%r14), %r12
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  movq %r13, 96(%rbp)
  vaesenc %xmm15, %xmm12, %xmm12
  movq %r12, 104(%rbp)
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  movdqu 0(%rcx), %xmm1
  vaesenc %xmm1, %xmm9, %xmm9
  movdqu 16(%rcx), %xmm15
  vaesenc %xmm1, %xmm10, %xmm10
  vpsrldq $8, %xmm6, %xmm6
  vaesenc %xmm1, %xmm11, %xmm11
  vpxor %xmm6, %xmm7, %xmm7
  vaesenc %xmm1, %xmm12, %xmm12
  vpxor %xmm0, %xmm4, %xmm4
  movbeq 8(%r14), %r13
  vaesenc %xmm1, %xmm13, %xmm13
  movbeq 0(%r14), %r12
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 32(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  vaesenc %xmm1, %xmm9, %xmm9
  vaesenc %xmm1, %xmm10, %xmm10
  vaesenc %xmm1, %xmm11, %xmm11
  vaesenc %xmm1, %xmm12, %xmm12
  vaesenc %xmm1, %xmm13, %xmm13
  movdqu 48(%rcx), %xmm15
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 64(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  vaesenc %xmm15, %xmm10, %xmm10
  vaesenc %xmm15, %xmm11, %xmm11
  vaesenc %xmm15, %xmm12, %xmm12
  vaesenc %xmm15, %xmm13, %xmm13
  vaesenc %xmm15, %xmm14, %xmm14
  vaesenc %xmm1, %xmm9, %xmm9
  vaesenc %xmm1, %xmm10, %xmm10
  vaesenc %xmm1, %xmm11, %xmm11
  vaesenc %xmm1, %xmm12, %xmm12
  vaesenc %xmm1, %xmm13, %xmm13
  movdqu 80(%rcx), %xmm15
  vaesenc %xmm1, %xmm14, %xmm14
  movdqu 96(%rcx), %xmm1
  vaesenc %xmm15, %xmm9, %xmm9
  movdqu %xmm7, 16(%rbp)
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vaesenc %xmm15, %xmm10, %xmm10
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor 0(%rdi), %xmm1, %xmm2
  vaesenc %xmm15, %xmm11, %xmm11
  vpxor 16(%rdi), %xmm1, %xmm0
  vaesenc %xmm15, %xmm12, %xmm12
  vpxor 32(%rdi), %xmm1, %xmm5
  vaesenc %xmm15, %xmm13, %xmm13
  vpxor 48(%rdi), %xmm1, %xmm6
  vaesenc %xmm15, %xmm14, %xmm14
  vpxor 64(%rdi), %xmm1, %xmm7
  vpxor 80(%rdi), %xmm1, %xmm3
  movdqu 128(%rbp), %xmm1
  vaesenclast %xmm2, %xmm9, %xmm9
  pxor %xmm2, %xmm2
  mov $72057594037927936, %r11
  pinsrq $1, %r11, %xmm2
  vaesenclast %xmm0, %xmm10, %xmm10
  vpaddd %xmm2, %xmm1, %xmm0
  movq %r13, 112(%rbp)
  lea 96(%rdi), %rdi
  vaesenclast %xmm5, %xmm11, %xmm11
  vpaddd %xmm2, %xmm0, %xmm5
  movq %r12, 120(%rbp)
  lea 96(%rsi), %rsi
  movdqu -128(%rcx), %xmm15
  vaesenclast %xmm6, %xmm12, %xmm12
  vpaddd %xmm2, %xmm5, %xmm6
  vaesenclast %xmm7, %xmm13, %xmm13
  vpaddd %xmm2, %xmm6, %xmm7
  vaesenclast %xmm3, %xmm14, %xmm14
  vpaddd %xmm2, %xmm7, %xmm3
  sub $6, %rdx
  cmp $6, %rdx
  jbe L216
  add $96, %r14
  jmp L217
L216:
L217:
  cmp $0, %rdx
  jbe L218
  movdqu %xmm9, -96(%rsi)
  vpxor %xmm15, %xmm1, %xmm9
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm0, %xmm10
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm5, %xmm11
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm6, %xmm12
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm7, %xmm13
  movdqu %xmm14, -16(%rsi)
  movdqu %xmm3, %xmm14
  movdqu 32(%rbp), %xmm7
  jmp L219
L218:
  vpxor 16(%rbp), %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
L219:
.balign 16
L213:
  cmp $0, %rdx
  ja L212
  movdqu %xmm1, 32(%rbp)
  movdqu %xmm9, -96(%rsi)
  movdqu %xmm10, -80(%rsi)
  movdqu %xmm11, -64(%rsi)
  movdqu %xmm12, -48(%rsi)
  movdqu %xmm13, -32(%rsi)
  movdqu %xmm14, -16(%rsi)
  sub $128, %rcx
L209:
  movdqu 32(%rbp), %xmm11
  mov %rcx, %r8
  mov 104(%rsp), %rax
  mov 112(%rsp), %rdi
  mov 120(%rsp), %rdx
  mov %rdx, %r14
  mov $579005069656919567, %r12
  pinsrq $0, %r12, %xmm9
  mov $283686952306183, %r12
  pinsrq $1, %r12, %xmm9
  pshufb %xmm9, %xmm11
  mov %rdi, %rbx
  mov %rdx, %r12
  mov %rax, %rdi
  mov %rdi, %r11
  jmp L221
.balign 16
L220:
  add $80, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 80(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  add $96, %r11
  sub $6, %rdx
.balign 16
L221:
  cmp $6, %rdx
  jae L220
  cmp $0, %rdx
  jbe L222
  mov %rdx, %r10
  sub $1, %r10
  imul $16, %r10
  add %r10, %r11
  movdqu -32(%r9), %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  cmp $1, %rdx
  jne L224
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  jmp L225
L224:
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  movdqu %xmm1, %xmm4
  movdqu -16(%r9), %xmm1
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  movdqu %xmm1, %xmm5
  cmp $2, %rdx
  je L226
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 16(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $3, %rdx
  je L228
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 32(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  cmp $4, %rdx
  je L230
  sub $16, %r11
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu 0(%r11), %xmm0
  pshufb %xmm9, %xmm0
  vpxor %xmm1, %xmm4, %xmm4
  movdqu 64(%r9), %xmm1
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
  movdqu %xmm1, %xmm5
  jmp L231
L230:
L231:
  jmp L229
L228:
L229:
  jmp L227
L226:
L227:
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  vpxor %xmm1, %xmm4, %xmm4
  vpxor %xmm2, %xmm6, %xmm6
  vpxor %xmm3, %xmm6, %xmm6
  vpxor %xmm5, %xmm7, %xmm7
L225:
  pxor %xmm3, %xmm3
  mov $3254779904, %r10
  pinsrd $3, %r10d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  jmp L223
L222:
L223:
  mov %rbx, %rdi
  mov %r12, %rdx
  pxor %xmm10, %xmm10
  mov $1, %rbx
  pinsrd $0, %ebx, %xmm10
  mov %rax, %r11
  mov %rdi, %r10
  mov $0, %rbx
  jmp L233
.balign 16
L232:
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  movdqu 0(%r11), %xmm2
  pxor %xmm0, %xmm2
  movdqu %xmm2, 0(%r10)
  add $1, %rbx
  add $16, %r11
  add $16, %r10
  paddd %xmm10, %xmm11
.balign 16
L233:
  cmp %rdx, %rbx
  jne L232
  add 96(%rsp), %r14
  imul $16, %r14
  mov 136(%rsp), %r13
  cmp %r14, %r13
  jbe L234
  mov 128(%rsp), %rax
  mov %r13, %r10
  and $15, %r10
  movdqu 0(%rax), %xmm0
  movdqu %xmm0, %xmm10
  cmp $8, %r10
  jae L236
  mov $0, %rcx
  pinsrq $1, %rcx, %xmm0
  mov %r10, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $0, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $0, %rcx, %xmm0
  jmp L237
L236:
  mov %r10, %rcx
  sub $8, %rcx
  shl $3, %rcx
  mov $1, %r11
  shl %cl, %r11
  sub $1, %r11
  pextrq $1, %xmm0, %rcx
  and %r11, %rcx
  pinsrq $1, %rcx, %xmm0
L237:
  pshufb %xmm9, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu %xmm11, %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pxor %xmm0, %xmm10
  movdqu %xmm10, 0(%rax)
  jmp L235
L234:
L235:
  mov %r15, %r11
  pxor %xmm0, %xmm0
  mov %r11, %rax
  imul $8, %rax
  pinsrq $1, %rax, %xmm0
  mov %r13, %rax
  imul $8, %rax
  pinsrq $0, %rax, %xmm0
  movdqu -32(%r9), %xmm5
  vpxor %xmm0, %xmm8, %xmm0
  vpclmulqdq $0, %xmm5, %xmm0, %xmm1
  vpclmulqdq $16, %xmm5, %xmm0, %xmm2
  vpclmulqdq $1, %xmm5, %xmm0, %xmm3
  vpclmulqdq $17, %xmm5, %xmm0, %xmm5
  movdqu %xmm1, %xmm4
  vpxor %xmm3, %xmm2, %xmm6
  movdqu %xmm5, %xmm7
  pxor %xmm3, %xmm3
  mov $3254779904, %r11
  pinsrd $3, %r11d, %xmm3
  vpslldq $8, %xmm6, %xmm5
  vpxor %xmm5, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm0
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpsrldq $8, %xmm6, %xmm6
  vpxor %xmm6, %xmm7, %xmm7
  vpxor %xmm0, %xmm4, %xmm4
  vpalignr $8, %xmm4, %xmm4, %xmm8
  vpclmulqdq $16, %xmm3, %xmm4, %xmm4
  vpxor %xmm7, %xmm8, %xmm8
  vpxor %xmm4, %xmm8, %xmm8
  movdqu 0(%rbp), %xmm0
  pshufb %xmm9, %xmm0
  movdqu 0(%r8), %xmm2
  pxor %xmm2, %xmm0
  movdqu 16(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 32(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 48(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 64(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 80(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 96(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 112(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 128(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 144(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 160(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 176(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 192(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 208(%r8), %xmm2
  aesenc %xmm2, %xmm0
  movdqu 224(%r8), %xmm2
  aesenclast %xmm2, %xmm0
  pxor %xmm2, %xmm2
  pshufb %xmm9, %xmm8
  pxor %xmm0, %xmm8
  mov 152(%rsp), %r15
  movdqu 0(%r15), %xmm0
  pcmpeqd %xmm8, %xmm0
  pextrq $0, %xmm0, %rdx
  sub $18446744073709551615, %rdx
  mov $0, %rax
  adc $0, %rax
  pextrq $1, %xmm0, %rdx
  sub $18446744073709551615, %rdx
  mov $0, %rdx
  adc $0, %rdx
  add %rdx, %rax
  mov %rax, %rcx
  pop %rbx
  pop %rbp
  pop %rdi
  pop %rsi
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  mov %rcx, %rax
  ret

.section .note.GNU-stack,"",%progbits
