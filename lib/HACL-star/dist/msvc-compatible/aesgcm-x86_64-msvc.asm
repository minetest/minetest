.code
ALIGN 16
aes128_key_expansion proc
  movdqu xmm1, xmmword ptr [rcx + 0]
  movdqu xmmword ptr [rdx + 0], xmm1
  aeskeygenassist xmm2, xmm1, 1
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 16], xmm1
  aeskeygenassist xmm2, xmm1, 2
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 32], xmm1
  aeskeygenassist xmm2, xmm1, 4
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 48], xmm1
  aeskeygenassist xmm2, xmm1, 8
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 64], xmm1
  aeskeygenassist xmm2, xmm1, 16
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 80], xmm1
  aeskeygenassist xmm2, xmm1, 32
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 96], xmm1
  aeskeygenassist xmm2, xmm1, 64
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 112], xmm1
  aeskeygenassist xmm2, xmm1, 128
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 128], xmm1
  aeskeygenassist xmm2, xmm1, 27
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 144], xmm1
  aeskeygenassist xmm2, xmm1, 54
  pshufd xmm2, xmm2, 255
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  vpslldq xmm3, xmm1, 4
  pxor xmm1, xmm3
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 160], xmm1
  pxor xmm1, xmm1
  pxor xmm2, xmm2
  pxor xmm3, xmm3
  ret
aes128_key_expansion endp
ALIGN 16
aes128_keyhash_init proc
  mov r8, 579005069656919567
  pinsrq xmm4, r8, 0
  mov r8, 283686952306183
  pinsrq xmm4, r8, 1
  pxor xmm0, xmm0
  movdqu xmmword ptr [rdx + 80], xmm0
  mov r8, rcx
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm0, xmm4
  mov rcx, rdx
  movdqu xmmword ptr [rcx + 32], xmm0
  movdqu xmm0, xmm6
  mov rax, r12
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 0], xmm1
  movdqu xmm1, xmm6
  movdqu xmm2, xmm6
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 16], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 48], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 64], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 96], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 112], xmm1
  movdqu xmm6, xmm0
  mov r12, rax
  ret
aes128_keyhash_init endp
ALIGN 16
aes256_key_expansion proc
  movdqu xmm1, xmmword ptr [rcx + 0]
  movdqu xmm3, xmmword ptr [rcx + 16]
  movdqu xmmword ptr [rdx + 0], xmm1
  movdqu xmmword ptr [rdx + 16], xmm3
  aeskeygenassist xmm2, xmm3, 1
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 32], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 48], xmm3
  aeskeygenassist xmm2, xmm3, 2
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 64], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 80], xmm3
  aeskeygenassist xmm2, xmm3, 4
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 96], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 112], xmm3
  aeskeygenassist xmm2, xmm3, 8
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 128], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 144], xmm3
  aeskeygenassist xmm2, xmm3, 16
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 160], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 176], xmm3
  aeskeygenassist xmm2, xmm3, 32
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 192], xmm1
  aeskeygenassist xmm2, xmm1, 0
  pshufd xmm2, xmm2, 170
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  vpslldq xmm4, xmm3, 4
  pxor xmm3, xmm4
  pxor xmm3, xmm2
  movdqu xmmword ptr [rdx + 208], xmm3
  aeskeygenassist xmm2, xmm3, 64
  pshufd xmm2, xmm2, 255
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  vpslldq xmm4, xmm1, 4
  pxor xmm1, xmm4
  pxor xmm1, xmm2
  movdqu xmmword ptr [rdx + 224], xmm1
  pxor xmm1, xmm1
  pxor xmm2, xmm2
  pxor xmm3, xmm3
  pxor xmm4, xmm4
  ret
aes256_key_expansion endp
ALIGN 16
aes256_keyhash_init proc
  mov r8, 579005069656919567
  pinsrq xmm4, r8, 0
  mov r8, 283686952306183
  pinsrq xmm4, r8, 1
  pxor xmm0, xmm0
  movdqu xmmword ptr [rdx + 80], xmm0
  mov r8, rcx
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm0, xmm4
  mov rcx, rdx
  movdqu xmmword ptr [rcx + 32], xmm0
  movdqu xmm0, xmm6
  mov rax, r12
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 0], xmm1
  movdqu xmm1, xmm6
  movdqu xmm2, xmm6
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 16], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 48], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 64], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 96], xmm1
  movdqu xmm2, xmm6
  movdqu xmm1, xmmword ptr [rcx + 32]
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm6, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  movdqu xmm5, xmm1
  pclmulqdq xmm1, xmm2, 16
  movdqu xmm3, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 1
  movdqu xmm4, xmm1
  movdqu xmm1, xmm5
  pclmulqdq xmm1, xmm2, 0
  pclmulqdq xmm5, xmm2, 17
  movdqu xmm2, xmm5
  movdqu xmm5, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm4
  mov r12, 0
  pinsrd xmm1, r12d, 0
  pshufd xmm1, xmm1, 14
  pxor xmm2, xmm1
  movdqu xmm1, xmm3
  mov r12, 0
  pinsrd xmm1, r12d, 3
  pshufd xmm1, xmm1, 79
  mov r12, 0
  pinsrd xmm4, r12d, 3
  pshufd xmm4, xmm4, 79
  pxor xmm1, xmm4
  pxor xmm1, xmm5
  movdqu xmm3, xmm1
  psrld xmm3, 31
  movdqu xmm4, xmm2
  psrld xmm4, 31
  pslld xmm1, 1
  pslld xmm2, 1
  vpslldq xmm5, xmm3, 4
  vpslldq xmm4, xmm4, 4
  mov r12, 0
  pinsrd xmm3, r12d, 0
  pshufd xmm3, xmm3, 3
  pxor xmm3, xmm4
  pxor xmm1, xmm5
  pxor xmm2, xmm3
  movdqu xmm5, xmm2
  pxor xmm2, xmm2
  mov r12, 3774873600
  pinsrd xmm2, r12d, 3
  pclmulqdq xmm1, xmm2, 17
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pxor xmm1, xmm5
  pxor xmm1, xmm6
  movdqu xmm6, xmm1
  movdqu xmm3, xmm1
  pxor xmm4, xmm4
  pxor xmm5, xmm5
  mov r12, 3254779904
  pinsrd xmm4, r12d, 3
  mov r12, 1
  pinsrd xmm4, r12d, 0
  mov r12, 2147483648
  pinsrd xmm5, r12d, 3
  movdqu xmm1, xmm3
  movdqu xmm2, xmm1
  psrld xmm2, 31
  pslld xmm1, 1
  vpslldq xmm2, xmm2, 4
  pxor xmm1, xmm2
  pand xmm3, xmm5
  pcmpeqd xmm3, xmm5
  pshufd xmm3, xmm3, 255
  pand xmm3, xmm4
  vpxor xmm1, xmm1, xmm3
  movdqu xmmword ptr [rcx + 112], xmm1
  movdqu xmm6, xmm0
  mov r12, rax
  ret
aes256_keyhash_init endp
ALIGN 16
gctr128_bytes proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rax, qword ptr [rsp + 272]
  movdqu xmm7, xmmword ptr [rax + 0]
  mov rax, rcx
  mov rbx, r8
  mov rsi, rdx
  mov r13, r9
  mov r8, qword ptr [rsp + 264]
  mov rcx, qword ptr [rsp + 280]
  mov rbp, rcx
  imul rbp, 16
  mov r12, 579005069656919567
  pinsrq xmm8, r12, 0
  mov r12, 283686952306183
  pinsrq xmm8, r12, 1
  mov rdx, rcx
  shr rdx, 2
  and rcx, 3
  cmp rdx, 0
  jbe L0
  mov r9, rax
  mov r10, rbx
  pshufb xmm7, xmm8
  movdqu xmm9, xmm7
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 0
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 1
  pshufb xmm9, xmm0
  movdqu xmm10, xmm9
  pxor xmm3, xmm3
  mov rax, 1
  pinsrd xmm3, eax, 2
  paddd xmm9, xmm3
  mov rax, 3
  pinsrd xmm3, eax, 2
  mov rax, 2
  pinsrd xmm3, eax, 0
  paddd xmm10, xmm3
  pshufb xmm9, xmm8
  pshufb xmm10, xmm8
  pextrq rdi, xmm7, 0
  mov rax, 283686952306183
  pinsrq xmm0, rax, 0
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 1
  pxor xmm15, xmm15
  mov rax, 4
  pinsrd xmm15, eax, 0
  mov rax, 4
  pinsrd xmm15, eax, 2
  jmp L3
ALIGN 16
L2:
  pinsrq xmm2, rdi, 0
  pinsrq xmm12, rdi, 0
  pinsrq xmm13, rdi, 0
  pinsrq xmm14, rdi, 0
  shufpd xmm2, xmm9, 2
  shufpd xmm12, xmm9, 0
  shufpd xmm13, xmm10, 2
  shufpd xmm14, xmm10, 0
  pshufb xmm9, xmm0
  pshufb xmm10, xmm0
  movdqu xmm3, xmmword ptr [r8 + 0]
  movdqu xmm4, xmmword ptr [r8 + 16]
  movdqu xmm5, xmmword ptr [r8 + 32]
  movdqu xmm6, xmmword ptr [r8 + 48]
  paddd xmm9, xmm15
  paddd xmm10, xmm15
  pxor xmm2, xmm3
  pxor xmm12, xmm3
  pxor xmm13, xmm3
  pxor xmm14, xmm3
  pshufb xmm9, xmm0
  pshufb xmm10, xmm0
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenc xmm2, xmm5
  aesenc xmm12, xmm5
  aesenc xmm13, xmm5
  aesenc xmm14, xmm5
  aesenc xmm2, xmm6
  aesenc xmm12, xmm6
  aesenc xmm13, xmm6
  aesenc xmm14, xmm6
  movdqu xmm3, xmmword ptr [r8 + 64]
  movdqu xmm4, xmmword ptr [r8 + 80]
  movdqu xmm5, xmmword ptr [r8 + 96]
  movdqu xmm6, xmmword ptr [r8 + 112]
  aesenc xmm2, xmm3
  aesenc xmm12, xmm3
  aesenc xmm13, xmm3
  aesenc xmm14, xmm3
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenc xmm2, xmm5
  aesenc xmm12, xmm5
  aesenc xmm13, xmm5
  aesenc xmm14, xmm5
  aesenc xmm2, xmm6
  aesenc xmm12, xmm6
  aesenc xmm13, xmm6
  aesenc xmm14, xmm6
  movdqu xmm3, xmmword ptr [r8 + 128]
  movdqu xmm4, xmmword ptr [r8 + 144]
  movdqu xmm5, xmmword ptr [r8 + 160]
  aesenc xmm2, xmm3
  aesenc xmm12, xmm3
  aesenc xmm13, xmm3
  aesenc xmm14, xmm3
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenclast xmm2, xmm5
  aesenclast xmm12, xmm5
  aesenclast xmm13, xmm5
  aesenclast xmm14, xmm5
  movdqu xmm7, xmmword ptr [r9 + 0]
  pxor xmm2, xmm7
  movdqu xmm7, xmmword ptr [r9 + 16]
  pxor xmm12, xmm7
  movdqu xmm7, xmmword ptr [r9 + 32]
  pxor xmm13, xmm7
  movdqu xmm7, xmmword ptr [r9 + 48]
  pxor xmm14, xmm7
  movdqu xmmword ptr [r10 + 0], xmm2
  movdqu xmmword ptr [r10 + 16], xmm12
  movdqu xmmword ptr [r10 + 32], xmm13
  movdqu xmmword ptr [r10 + 48], xmm14
  sub rdx, 1
  add r9, 64
  add r10, 64
ALIGN 16
L3:
  cmp rdx, 0
  ja L2
  movdqu xmm7, xmm9
  pinsrq xmm7, rdi, 0
  pshufb xmm7, xmm8
  mov rax, r9
  mov rbx, r10
  jmp L1
L0:
L1:
  mov rdx, 0
  mov r9, rax
  mov r10, rbx
  pxor xmm4, xmm4
  mov r12, 1
  pinsrd xmm4, r12d, 0
  jmp L5
ALIGN 16
L4:
  movdqu xmm0, xmm7
  pshufb xmm0, xmm8
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r9 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rdx, 1
  add r9, 16
  add r10, 16
  paddd xmm7, xmm4
ALIGN 16
L5:
  cmp rdx, rcx
  jne L4
  cmp rsi, rbp
  jbe L6
  movdqu xmm1, xmmword ptr [r13 + 0]
  movdqu xmm0, xmm7
  mov r12, 579005069656919567
  pinsrq xmm2, r12, 0
  mov r12, 283686952306183
  pinsrq xmm2, r12, 1
  pshufb xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pxor xmm1, xmm0
  movdqu xmmword ptr [r13 + 0], xmm1
  jmp L7
L6:
L7:
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  ret
gctr128_bytes endp
ALIGN 16
gctr256_bytes proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rax, qword ptr [rsp + 272]
  movdqu xmm7, xmmword ptr [rax + 0]
  mov rax, rcx
  mov rbx, r8
  mov rsi, rdx
  mov r13, r9
  mov r8, qword ptr [rsp + 264]
  mov rcx, qword ptr [rsp + 280]
  mov rbp, rcx
  imul rbp, 16
  mov r12, 579005069656919567
  pinsrq xmm8, r12, 0
  mov r12, 283686952306183
  pinsrq xmm8, r12, 1
  mov rdx, rcx
  shr rdx, 2
  and rcx, 3
  cmp rdx, 0
  jbe L8
  mov r9, rax
  mov r10, rbx
  pshufb xmm7, xmm8
  movdqu xmm9, xmm7
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 0
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 1
  pshufb xmm9, xmm0
  movdqu xmm10, xmm9
  pxor xmm3, xmm3
  mov rax, 1
  pinsrd xmm3, eax, 2
  paddd xmm9, xmm3
  mov rax, 3
  pinsrd xmm3, eax, 2
  mov rax, 2
  pinsrd xmm3, eax, 0
  paddd xmm10, xmm3
  pshufb xmm9, xmm8
  pshufb xmm10, xmm8
  pextrq rdi, xmm7, 0
  mov rax, 283686952306183
  pinsrq xmm0, rax, 0
  mov rax, 579005069656919567
  pinsrq xmm0, rax, 1
  pxor xmm15, xmm15
  mov rax, 4
  pinsrd xmm15, eax, 0
  mov rax, 4
  pinsrd xmm15, eax, 2
  jmp L11
ALIGN 16
L10:
  pinsrq xmm2, rdi, 0
  pinsrq xmm12, rdi, 0
  pinsrq xmm13, rdi, 0
  pinsrq xmm14, rdi, 0
  shufpd xmm2, xmm9, 2
  shufpd xmm12, xmm9, 0
  shufpd xmm13, xmm10, 2
  shufpd xmm14, xmm10, 0
  pshufb xmm9, xmm0
  pshufb xmm10, xmm0
  movdqu xmm3, xmmword ptr [r8 + 0]
  movdqu xmm4, xmmword ptr [r8 + 16]
  movdqu xmm5, xmmword ptr [r8 + 32]
  movdqu xmm6, xmmword ptr [r8 + 48]
  paddd xmm9, xmm15
  paddd xmm10, xmm15
  pxor xmm2, xmm3
  pxor xmm12, xmm3
  pxor xmm13, xmm3
  pxor xmm14, xmm3
  pshufb xmm9, xmm0
  pshufb xmm10, xmm0
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenc xmm2, xmm5
  aesenc xmm12, xmm5
  aesenc xmm13, xmm5
  aesenc xmm14, xmm5
  aesenc xmm2, xmm6
  aesenc xmm12, xmm6
  aesenc xmm13, xmm6
  aesenc xmm14, xmm6
  movdqu xmm3, xmmword ptr [r8 + 64]
  movdqu xmm4, xmmword ptr [r8 + 80]
  movdqu xmm5, xmmword ptr [r8 + 96]
  movdqu xmm6, xmmword ptr [r8 + 112]
  aesenc xmm2, xmm3
  aesenc xmm12, xmm3
  aesenc xmm13, xmm3
  aesenc xmm14, xmm3
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenc xmm2, xmm5
  aesenc xmm12, xmm5
  aesenc xmm13, xmm5
  aesenc xmm14, xmm5
  aesenc xmm2, xmm6
  aesenc xmm12, xmm6
  aesenc xmm13, xmm6
  aesenc xmm14, xmm6
  movdqu xmm3, xmmword ptr [r8 + 128]
  movdqu xmm4, xmmword ptr [r8 + 144]
  movdqu xmm5, xmmword ptr [r8 + 160]
  aesenc xmm2, xmm3
  aesenc xmm12, xmm3
  aesenc xmm13, xmm3
  aesenc xmm14, xmm3
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  movdqu xmm3, xmm5
  movdqu xmm4, xmmword ptr [r8 + 176]
  movdqu xmm5, xmmword ptr [r8 + 192]
  movdqu xmm6, xmmword ptr [r8 + 208]
  aesenc xmm2, xmm3
  aesenc xmm12, xmm3
  aesenc xmm13, xmm3
  aesenc xmm14, xmm3
  aesenc xmm2, xmm4
  aesenc xmm12, xmm4
  aesenc xmm13, xmm4
  aesenc xmm14, xmm4
  aesenc xmm2, xmm5
  aesenc xmm12, xmm5
  aesenc xmm13, xmm5
  aesenc xmm14, xmm5
  aesenc xmm2, xmm6
  aesenc xmm12, xmm6
  aesenc xmm13, xmm6
  aesenc xmm14, xmm6
  movdqu xmm5, xmmword ptr [r8 + 224]
  aesenclast xmm2, xmm5
  aesenclast xmm12, xmm5
  aesenclast xmm13, xmm5
  aesenclast xmm14, xmm5
  movdqu xmm7, xmmword ptr [r9 + 0]
  pxor xmm2, xmm7
  movdqu xmm7, xmmword ptr [r9 + 16]
  pxor xmm12, xmm7
  movdqu xmm7, xmmword ptr [r9 + 32]
  pxor xmm13, xmm7
  movdqu xmm7, xmmword ptr [r9 + 48]
  pxor xmm14, xmm7
  movdqu xmmword ptr [r10 + 0], xmm2
  movdqu xmmword ptr [r10 + 16], xmm12
  movdqu xmmword ptr [r10 + 32], xmm13
  movdqu xmmword ptr [r10 + 48], xmm14
  sub rdx, 1
  add r9, 64
  add r10, 64
ALIGN 16
L11:
  cmp rdx, 0
  ja L10
  movdqu xmm7, xmm9
  pinsrq xmm7, rdi, 0
  pshufb xmm7, xmm8
  mov rax, r9
  mov rbx, r10
  jmp L9
L8:
L9:
  mov rdx, 0
  mov r9, rax
  mov r10, rbx
  pxor xmm4, xmm4
  mov r12, 1
  pinsrd xmm4, r12d, 0
  jmp L13
ALIGN 16
L12:
  movdqu xmm0, xmm7
  pshufb xmm0, xmm8
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r9 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rdx, 1
  add r9, 16
  add r10, 16
  paddd xmm7, xmm4
ALIGN 16
L13:
  cmp rdx, rcx
  jne L12
  cmp rsi, rbp
  jbe L14
  movdqu xmm1, xmmword ptr [r13 + 0]
  movdqu xmm0, xmm7
  mov r12, 579005069656919567
  pinsrq xmm2, r12, 0
  mov r12, 283686952306183
  pinsrq xmm2, r12, 1
  pshufb xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pxor xmm1, xmm0
  movdqu xmmword ptr [r13 + 0], xmm1
  jmp L15
L14:
L15:
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  ret
gctr256_bytes endp
ALIGN 16
compute_iv_stdcall proc
  cmp rdx, 12
  jne L16
  push rdi
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 56]
  mov r9, qword ptr [rsp + 64]
  cmp rsi, 12
  jne L18
  movdqu xmm0, xmmword ptr [r8 + 0]
  mov rax, 579005069656919567
  pinsrq xmm1, rax, 0
  mov rax, 283686952306183
  pinsrq xmm1, rax, 1
  pshufb xmm0, xmm1
  mov rax, 1
  pinsrd xmm0, eax, 0
  movdqu xmmword ptr [rcx + 0], xmm0
  jmp L19
L18:
  mov rax, rcx
  add r9, 32
  mov rbx, r8
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L21
ALIGN 16
L20:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L21:
  cmp rdx, 6
  jae L20
  cmp rdx, 0
  jbe L22
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L24
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L25
L24:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L26
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L28
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L30
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L31
L30:
L31:
  jmp L29
L28:
L29:
  jmp L27
L26:
L27:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L25:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L23
L22:
L23:
  mov r15, rsi
  cmp rsi, rcx
  jbe L32
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L34
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L35
L34:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L35:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L33
L32:
L33:
  mov rcx, rax
  mov r11, 0
  mov r13, rsi
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmmword ptr [rcx + 0], xmm8
L19:
  pop rsi
  pop rdi
  jmp L17
L16:
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 264]
  mov r9, qword ptr [rsp + 272]
  cmp rsi, 12
  jne L36
  movdqu xmm0, xmmword ptr [r8 + 0]
  mov rax, 579005069656919567
  pinsrq xmm1, rax, 0
  mov rax, 283686952306183
  pinsrq xmm1, rax, 1
  pshufb xmm0, xmm1
  mov rax, 1
  pinsrd xmm0, eax, 0
  movdqu xmmword ptr [rcx + 0], xmm0
  jmp L37
L36:
  mov rax, rcx
  add r9, 32
  mov rbx, r8
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L39
ALIGN 16
L38:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L39:
  cmp rdx, 6
  jae L38
  cmp rdx, 0
  jbe L40
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L42
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L43
L42:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L44
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L46
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L48
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L49
L48:
L49:
  jmp L47
L46:
L47:
  jmp L45
L44:
L45:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L43:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L41
L40:
L41:
  mov r15, rsi
  cmp rsi, rcx
  jbe L50
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L52
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L53
L52:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L53:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L51
L50:
L51:
  mov rcx, rax
  mov r11, 0
  mov r13, rsi
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmmword ptr [rcx + 0], xmm8
L37:
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
L17:
  ret
compute_iv_stdcall endp
ALIGN 16
gcm128_encrypt_opt proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 264]
  mov r9, qword ptr [rsp + 272]
  mov rbp, qword ptr [rsp + 352]
  mov r13, rcx
  lea r9, qword ptr [r9 + 32]
  mov rbx, qword ptr [rsp + 280]
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L55
ALIGN 16
L54:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L55:
  cmp rdx, 6
  jae L54
  cmp rdx, 0
  jbe L56
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L58
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L59
L58:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L60
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L62
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L64
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L65
L64:
L65:
  jmp L63
L62:
L63:
  jmp L61
L60:
L61:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L59:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L57
L56:
L57:
  mov r15, rsi
  cmp rsi, rcx
  jbe L66
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L68
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L69
L68:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L69:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L67
L66:
L67:
  mov rdi, qword ptr [rsp + 288]
  mov rsi, qword ptr [rsp + 296]
  mov rdx, qword ptr [rsp + 304]
  mov rcx, r13
  movdqu xmm0, xmm9
  movdqu xmm1, xmmword ptr [r8 + 0]
  movdqu xmmword ptr [rbp + 0], xmm1
  pxor xmm10, xmm10
  mov r11, 1
  pinsrq xmm10, r11, 0
  vpaddd xmm1, xmm1, xmm10
  cmp rdx, 0
  jne L70
  vpshufb xmm1, xmm1, xmm0
  movdqu xmmword ptr [rbp + 32], xmm1
  jmp L71
L70:
  movdqu xmmword ptr [rbp + 32], xmm8
  add rcx, 128
  pextrq rbx, xmm1, 0
  and rbx, 255
  vpshufb xmm1, xmm1, xmm0
  lea r14, qword ptr [rsi + 96]
  movdqu xmm4, xmmword ptr [rcx + -128]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  movdqu xmm15, xmmword ptr [rcx + -112]
  mov r12, rcx
  sub r12, 96
  vpxor xmm9, xmm1, xmm4
  add rbx, 6
  cmp rbx, 256
  jae L72
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm12, xmm11, xmm2
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm13, xmm12, xmm2
  vpxor xmm12, xmm12, xmm4
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm13, xmm13, xmm4
  vpaddd xmm1, xmm14, xmm2
  vpxor xmm14, xmm14, xmm4
  jmp L73
L72:
  sub rbx, 256
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpxor xmm12, xmm12, xmm4
  vpshufb xmm14, xmm14, xmm0
  vpxor xmm13, xmm13, xmm4
  vpshufb xmm1, xmm1, xmm0
  vpxor xmm14, xmm14, xmm4
L73:
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -96]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 16]
  movdqu xmm3, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm4, xmm3, xmmword ptr [rdi + 0]
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm5, xmm3, xmmword ptr [rdi + 16]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm6, xmm3, xmmword ptr [rdi + 32]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm8, xmm3, xmmword ptr [rdi + 48]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm2, xmm3, xmmword ptr [rdi + 64]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm3, xmm3, xmmword ptr [rdi + 80]
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm9, xmm9, xmm4
  vaesenclast xmm10, xmm10, xmm5
  vaesenclast xmm11, xmm11, xmm6
  vaesenclast xmm12, xmm12, xmm8
  vaesenclast xmm13, xmm13, xmm2
  vaesenclast xmm14, xmm14, xmm3
  movdqu xmmword ptr [rsi + 0], xmm9
  movdqu xmmword ptr [rsi + 16], xmm10
  movdqu xmmword ptr [rsi + 32], xmm11
  movdqu xmmword ptr [rsi + 48], xmm12
  movdqu xmmword ptr [rsi + 64], xmm13
  movdqu xmmword ptr [rsi + 80], xmm14
  lea rsi, qword ptr [rsi + 96]
  vpshufb xmm8, xmm9, xmm0
  vpshufb xmm2, xmm10, xmm0
  movdqu xmmword ptr [rbp + 112], xmm8
  vpshufb xmm4, xmm11, xmm0
  movdqu xmmword ptr [rbp + 96], xmm2
  vpshufb xmm5, xmm12, xmm0
  movdqu xmmword ptr [rbp + 80], xmm4
  vpshufb xmm6, xmm13, xmm0
  movdqu xmmword ptr [rbp + 64], xmm5
  vpshufb xmm7, xmm14, xmm0
  movdqu xmmword ptr [rbp + 48], xmm6
  movdqu xmm4, xmmword ptr [rcx + -128]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  movdqu xmm15, xmmword ptr [rcx + -112]
  mov r12, rcx
  sub r12, 96
  vpxor xmm9, xmm1, xmm4
  add rbx, 6
  cmp rbx, 256
  jae L74
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm12, xmm11, xmm2
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm13, xmm12, xmm2
  vpxor xmm12, xmm12, xmm4
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm13, xmm13, xmm4
  vpaddd xmm1, xmm14, xmm2
  vpxor xmm14, xmm14, xmm4
  jmp L75
L74:
  sub rbx, 256
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpxor xmm12, xmm12, xmm4
  vpshufb xmm14, xmm14, xmm0
  vpxor xmm13, xmm13, xmm4
  vpshufb xmm1, xmm1, xmm0
  vpxor xmm14, xmm14, xmm4
L75:
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -96]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 16]
  movdqu xmm3, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm4, xmm3, xmmword ptr [rdi + 0]
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm5, xmm3, xmmword ptr [rdi + 16]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm6, xmm3, xmmword ptr [rdi + 32]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm8, xmm3, xmmword ptr [rdi + 48]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm2, xmm3, xmmword ptr [rdi + 64]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm3, xmm3, xmmword ptr [rdi + 80]
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm9, xmm9, xmm4
  vaesenclast xmm10, xmm10, xmm5
  vaesenclast xmm11, xmm11, xmm6
  vaesenclast xmm12, xmm12, xmm8
  vaesenclast xmm13, xmm13, xmm2
  vaesenclast xmm14, xmm14, xmm3
  movdqu xmmword ptr [rsi + 0], xmm9
  movdqu xmmword ptr [rsi + 16], xmm10
  movdqu xmmword ptr [rsi + 32], xmm11
  movdqu xmmword ptr [rsi + 48], xmm12
  movdqu xmmword ptr [rsi + 64], xmm13
  movdqu xmmword ptr [rsi + 80], xmm14
  lea rsi, qword ptr [rsi + 96]
  sub rdx, 12
  movdqu xmm8, xmmword ptr [rbp + 32]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vpxor xmm4, xmm4, xmm4
  movdqu xmm15, xmmword ptr [rcx + -128]
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpaddd xmm12, xmm11, xmm2
  vpaddd xmm13, xmm12, xmm2
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rbp + 16], xmm4
  jmp L77
ALIGN 16
L76:
  add rbx, 6
  cmp rbx, 256
  jb L78
  mov r11, 579005069656919567
  pinsrq xmm0, r11, 0
  mov r11, 283686952306183
  pinsrq xmm0, r11, 1
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm15
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm15
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpshufb xmm14, xmm14, xmm0
  vpshufb xmm1, xmm1, xmm0
  sub rbx, 256
  jmp L79
L78:
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm1, xmm2, xmm14
  vpxor xmm10, xmm10, xmm15
  vpxor xmm11, xmm11, xmm15
L79:
  movdqu xmmword ptr [rbp + 128], xmm1
  vpclmulqdq xmm5, xmm7, xmm3, 16
  vpxor xmm12, xmm12, xmm15
  movdqu xmm2, xmmword ptr [rcx + -112]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vaesenc xmm9, xmm9, xmm2
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpxor xmm13, xmm13, xmm15
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vaesenc xmm10, xmm10, xmm2
  vpxor xmm14, xmm14, xmm15
  vpclmulqdq xmm7, xmm7, xmm3, 17
  vaesenc xmm11, xmm11, xmm2
  movdqu xmm3, xmmword ptr [r9 + -16]
  vaesenc xmm12, xmm12, xmm2
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vaesenc xmm13, xmm13, xmm2
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vaesenc xmm14, xmm14, xmm2
  movdqu xmm15, xmmword ptr [rcx + -96]
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 88]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 80]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 32], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 40], r12
  movdqu xmm5, xmmword ptr [r9 + 16]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vaesenc xmm11, xmm11, xmm15
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 72]
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 64]
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 48], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 56], r12
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 56]
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 48]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 64], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 72], r12
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 40]
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 32]
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 80], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 88], r12
  vpxor xmm6, xmm6, xmm5
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm6, xmm6, xmm1
  movdqu xmm15, xmmword ptr [rcx + -16]
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov r11, 13979173243358019584
  pinsrq xmm3, r11, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm7, xmm7, xmm8
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm4, xmm4, xmm5
  movbe r13, qword ptr [r14 + 24]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 16]
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  mov qword ptr [rbp + 96], r13
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 104], r12
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm1, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm1
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm10, xmm10, xmm1
  vpsrldq xmm6, xmm6, 8
  vaesenc xmm11, xmm11, xmm1
  vpxor xmm7, xmm7, xmm6
  vaesenc xmm12, xmm12, xmm1
  vpxor xmm4, xmm4, xmm0
  movbe r13, qword ptr [r14 + 8]
  vaesenc xmm13, xmm13, xmm1
  movbe r12, qword ptr [r14 + 0]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  movdqu xmmword ptr [rbp + 16], xmm7
  vpalignr xmm8, xmm4, xmm4, 8
  vaesenc xmm10, xmm10, xmm15
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm2, xmm1, xmmword ptr [rdi + 0]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm0, xmm1, xmmword ptr [rdi + 16]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm5, xmm1, xmmword ptr [rdi + 32]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm6, xmm1, xmmword ptr [rdi + 48]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm7, xmm1, xmmword ptr [rdi + 64]
  vpxor xmm3, xmm1, xmmword ptr [rdi + 80]
  movdqu xmm1, xmmword ptr [rbp + 128]
  vaesenclast xmm9, xmm9, xmm2
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vaesenclast xmm10, xmm10, xmm0
  vpaddd xmm0, xmm1, xmm2
  mov qword ptr [rbp + 112], r13
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm11, xmm11, xmm5
  vpaddd xmm5, xmm0, xmm2
  mov qword ptr [rbp + 120], r12
  lea rsi, qword ptr [rsi + 96]
  movdqu xmm15, xmmword ptr [rcx + -128]
  vaesenclast xmm12, xmm12, xmm6
  vpaddd xmm6, xmm5, xmm2
  vaesenclast xmm13, xmm13, xmm7
  vpaddd xmm7, xmm6, xmm2
  vaesenclast xmm14, xmm14, xmm3
  vpaddd xmm3, xmm7, xmm2
  sub rdx, 6
  add r14, 96
  cmp rdx, 0
  jbe L80
  movdqu xmmword ptr [rsi + -96], xmm9
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmm11, xmm5
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmm12, xmm6
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmm13, xmm7
  movdqu xmmword ptr [rsi + -16], xmm14
  movdqu xmm14, xmm3
  movdqu xmm7, xmmword ptr [rbp + 32]
  jmp L81
L80:
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpxor xmm8, xmm8, xmm4
L81:
ALIGN 16
L77:
  cmp rdx, 0
  ja L76
  movdqu xmm7, xmmword ptr [rbp + 32]
  movdqu xmmword ptr [rbp + 32], xmm1
  pxor xmm4, xmm4
  movdqu xmmword ptr [rbp + 16], xmm4
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vpclmulqdq xmm5, xmm7, xmm3, 16
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vpclmulqdq xmm7, xmm7, xmm3, 17
  movdqu xmm3, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  movdqu xmm5, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vpxor xmm6, xmm6, xmm5
  vpxor xmm6, xmm6, xmm1
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov rax, 3254779904
  pinsrd xmm3, eax, 3
  vpxor xmm7, xmm7, xmm8
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  mov r12, 579005069656919567
  pinsrq xmm0, r12, 0
  mov r12, 283686952306183
  pinsrq xmm0, r12, 1
  movdqu xmmword ptr [rsi + -96], xmm9
  vpshufb xmm9, xmm9, xmm0
  vpxor xmm1, xmm1, xmm7
  movdqu xmmword ptr [rsi + -80], xmm10
  vpshufb xmm10, xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  vpshufb xmm11, xmm11, xmm0
  movdqu xmmword ptr [rsi + -48], xmm12
  vpshufb xmm12, xmm12, xmm0
  movdqu xmmword ptr [rsi + -32], xmm13
  vpshufb xmm13, xmm13, xmm0
  movdqu xmmword ptr [rsi + -16], xmm14
  vpshufb xmm14, xmm14, xmm0
  pxor xmm4, xmm4
  movdqu xmm7, xmm14
  movdqu xmmword ptr [rbp + 16], xmm4
  movdqu xmmword ptr [rbp + 48], xmm13
  movdqu xmmword ptr [rbp + 64], xmm12
  movdqu xmmword ptr [rbp + 80], xmm11
  movdqu xmmword ptr [rbp + 96], xmm10
  movdqu xmmword ptr [rbp + 112], xmm9
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vpclmulqdq xmm5, xmm7, xmm3, 16
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vpclmulqdq xmm7, xmm7, xmm3, 17
  movdqu xmm3, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  movdqu xmm5, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vpxor xmm6, xmm6, xmm5
  vpxor xmm6, xmm6, xmm1
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov rax, 3254779904
  pinsrd xmm3, eax, 3
  vpxor xmm7, xmm7, xmm8
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  sub rcx, 128
L71:
  movdqu xmm11, xmmword ptr [rbp + 32]
  mov r8, rcx
  mov rax, qword ptr [rsp + 312]
  mov rdi, qword ptr [rsp + 320]
  mov rdx, qword ptr [rsp + 328]
  mov r14, rdx
  mov r12, 579005069656919567
  pinsrq xmm9, r12, 0
  mov r12, 283686952306183
  pinsrq xmm9, r12, 1
  pshufb xmm11, xmm9
  pxor xmm10, xmm10
  mov rbx, 1
  pinsrd xmm10, ebx, 0
  mov r11, rax
  mov r10, rdi
  mov rbx, 0
  jmp L83
ALIGN 16
L82:
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r11 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rbx, 1
  add r11, 16
  add r10, 16
  paddd xmm11, xmm10
ALIGN 16
L83:
  cmp rbx, rdx
  jne L82
  mov r11, rdi
  jmp L85
ALIGN 16
L84:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L85:
  cmp rdx, 6
  jae L84
  cmp rdx, 0
  jbe L86
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L88
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L89
L88:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L90
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L92
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L94
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L95
L94:
L95:
  jmp L93
L92:
L93:
  jmp L91
L90:
L91:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L89:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L87
L86:
L87:
  add r14, qword ptr [rsp + 304]
  imul r14, 16
  mov r13, qword ptr [rsp + 344]
  cmp r13, r14
  jbe L96
  mov rax, qword ptr [rsp + 336]
  mov r10, r13
  and r10, 15
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm4, xmmword ptr [rax + 0]
  pxor xmm0, xmm4
  movdqu xmmword ptr [rax + 0], xmm0
  cmp r10, 8
  jae L98
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L99
L98:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L99:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L97
L96:
L97:
  mov r11, r15
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmmword ptr [rbp + 0]
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm8, xmm9
  pxor xmm8, xmm0
  mov r15, qword ptr [rsp + 360]
  movdqu xmmword ptr [r15 + 0], xmm8
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  ret
gcm128_encrypt_opt endp
ALIGN 16
gcm256_encrypt_opt proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 264]
  mov r9, qword ptr [rsp + 272]
  mov rbp, qword ptr [rsp + 352]
  mov r13, rcx
  lea r9, qword ptr [r9 + 32]
  mov rbx, qword ptr [rsp + 280]
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L101
ALIGN 16
L100:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L101:
  cmp rdx, 6
  jae L100
  cmp rdx, 0
  jbe L102
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L104
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L105
L104:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L106
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L108
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L110
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L111
L110:
L111:
  jmp L109
L108:
L109:
  jmp L107
L106:
L107:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L105:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L103
L102:
L103:
  mov r15, rsi
  cmp rsi, rcx
  jbe L112
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L114
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L115
L114:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L115:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L113
L112:
L113:
  mov rdi, qword ptr [rsp + 288]
  mov rsi, qword ptr [rsp + 296]
  mov rdx, qword ptr [rsp + 304]
  mov rcx, r13
  movdqu xmm0, xmm9
  movdqu xmm1, xmmword ptr [r8 + 0]
  movdqu xmmword ptr [rbp + 0], xmm1
  pxor xmm10, xmm10
  mov r11, 1
  pinsrq xmm10, r11, 0
  vpaddd xmm1, xmm1, xmm10
  cmp rdx, 0
  jne L116
  vpshufb xmm1, xmm1, xmm0
  movdqu xmmword ptr [rbp + 32], xmm1
  jmp L117
L116:
  movdqu xmmword ptr [rbp + 32], xmm8
  add rcx, 128
  pextrq rbx, xmm1, 0
  and rbx, 255
  vpshufb xmm1, xmm1, xmm0
  lea r14, qword ptr [rsi + 96]
  movdqu xmm4, xmmword ptr [rcx + -128]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  movdqu xmm15, xmmword ptr [rcx + -112]
  mov r12, rcx
  sub r12, 96
  vpxor xmm9, xmm1, xmm4
  add rbx, 6
  cmp rbx, 256
  jae L118
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm12, xmm11, xmm2
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm13, xmm12, xmm2
  vpxor xmm12, xmm12, xmm4
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm13, xmm13, xmm4
  vpaddd xmm1, xmm14, xmm2
  vpxor xmm14, xmm14, xmm4
  jmp L119
L118:
  sub rbx, 256
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpxor xmm12, xmm12, xmm4
  vpshufb xmm14, xmm14, xmm0
  vpxor xmm13, xmm13, xmm4
  vpshufb xmm1, xmm1, xmm0
  vpxor xmm14, xmm14, xmm4
L119:
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -96]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 80]
  movdqu xmm3, xmmword ptr [rcx + 96]
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm4, xmm3, xmmword ptr [rdi + 0]
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm5, xmm3, xmmword ptr [rdi + 16]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm6, xmm3, xmmword ptr [rdi + 32]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm8, xmm3, xmmword ptr [rdi + 48]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm2, xmm3, xmmword ptr [rdi + 64]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm3, xmm3, xmmword ptr [rdi + 80]
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm9, xmm9, xmm4
  vaesenclast xmm10, xmm10, xmm5
  vaesenclast xmm11, xmm11, xmm6
  vaesenclast xmm12, xmm12, xmm8
  vaesenclast xmm13, xmm13, xmm2
  vaesenclast xmm14, xmm14, xmm3
  movdqu xmmword ptr [rsi + 0], xmm9
  movdqu xmmword ptr [rsi + 16], xmm10
  movdqu xmmword ptr [rsi + 32], xmm11
  movdqu xmmword ptr [rsi + 48], xmm12
  movdqu xmmword ptr [rsi + 64], xmm13
  movdqu xmmword ptr [rsi + 80], xmm14
  lea rsi, qword ptr [rsi + 96]
  vpshufb xmm8, xmm9, xmm0
  vpshufb xmm2, xmm10, xmm0
  movdqu xmmword ptr [rbp + 112], xmm8
  vpshufb xmm4, xmm11, xmm0
  movdqu xmmword ptr [rbp + 96], xmm2
  vpshufb xmm5, xmm12, xmm0
  movdqu xmmword ptr [rbp + 80], xmm4
  vpshufb xmm6, xmm13, xmm0
  movdqu xmmword ptr [rbp + 64], xmm5
  vpshufb xmm7, xmm14, xmm0
  movdqu xmmword ptr [rbp + 48], xmm6
  movdqu xmm4, xmmword ptr [rcx + -128]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  movdqu xmm15, xmmword ptr [rcx + -112]
  mov r12, rcx
  sub r12, 96
  vpxor xmm9, xmm1, xmm4
  add rbx, 6
  cmp rbx, 256
  jae L120
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm12, xmm11, xmm2
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm13, xmm12, xmm2
  vpxor xmm12, xmm12, xmm4
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm13, xmm13, xmm4
  vpaddd xmm1, xmm14, xmm2
  vpxor xmm14, xmm14, xmm4
  jmp L121
L120:
  sub rbx, 256
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm4
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm4
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpxor xmm12, xmm12, xmm4
  vpshufb xmm14, xmm14, xmm0
  vpxor xmm13, xmm13, xmm4
  vpshufb xmm1, xmm1, xmm0
  vpxor xmm14, xmm14, xmm4
L121:
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -96]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 48]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + 80]
  movdqu xmm3, xmmword ptr [rcx + 96]
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm4, xmm3, xmmword ptr [rdi + 0]
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm5, xmm3, xmmword ptr [rdi + 16]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm6, xmm3, xmmword ptr [rdi + 32]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm8, xmm3, xmmword ptr [rdi + 48]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm2, xmm3, xmmword ptr [rdi + 64]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm3, xmm3, xmmword ptr [rdi + 80]
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm9, xmm9, xmm4
  vaesenclast xmm10, xmm10, xmm5
  vaesenclast xmm11, xmm11, xmm6
  vaesenclast xmm12, xmm12, xmm8
  vaesenclast xmm13, xmm13, xmm2
  vaesenclast xmm14, xmm14, xmm3
  movdqu xmmword ptr [rsi + 0], xmm9
  movdqu xmmword ptr [rsi + 16], xmm10
  movdqu xmmword ptr [rsi + 32], xmm11
  movdqu xmmword ptr [rsi + 48], xmm12
  movdqu xmmword ptr [rsi + 64], xmm13
  movdqu xmmword ptr [rsi + 80], xmm14
  lea rsi, qword ptr [rsi + 96]
  sub rdx, 12
  movdqu xmm8, xmmword ptr [rbp + 32]
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vpxor xmm4, xmm4, xmm4
  movdqu xmm15, xmmword ptr [rcx + -128]
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpaddd xmm12, xmm11, xmm2
  vpaddd xmm13, xmm12, xmm2
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rbp + 16], xmm4
  jmp L123
ALIGN 16
L122:
  add rbx, 6
  cmp rbx, 256
  jb L124
  mov r11, 579005069656919567
  pinsrq xmm0, r11, 0
  mov r11, 283686952306183
  pinsrq xmm0, r11, 1
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm15
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm15
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpshufb xmm14, xmm14, xmm0
  vpshufb xmm1, xmm1, xmm0
  sub rbx, 256
  jmp L125
L124:
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm1, xmm2, xmm14
  vpxor xmm10, xmm10, xmm15
  vpxor xmm11, xmm11, xmm15
L125:
  movdqu xmmword ptr [rbp + 128], xmm1
  vpclmulqdq xmm5, xmm7, xmm3, 16
  vpxor xmm12, xmm12, xmm15
  movdqu xmm2, xmmword ptr [rcx + -112]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vaesenc xmm9, xmm9, xmm2
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpxor xmm13, xmm13, xmm15
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vaesenc xmm10, xmm10, xmm2
  vpxor xmm14, xmm14, xmm15
  vpclmulqdq xmm7, xmm7, xmm3, 17
  vaesenc xmm11, xmm11, xmm2
  movdqu xmm3, xmmword ptr [r9 + -16]
  vaesenc xmm12, xmm12, xmm2
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vaesenc xmm13, xmm13, xmm2
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vaesenc xmm14, xmm14, xmm2
  movdqu xmm15, xmmword ptr [rcx + -96]
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 88]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 80]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 32], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 40], r12
  movdqu xmm5, xmmword ptr [r9 + 16]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vaesenc xmm11, xmm11, xmm15
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 72]
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 64]
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 48], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 56], r12
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 56]
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 48]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 64], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 72], r12
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 40]
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 32]
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 80], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 88], r12
  vpxor xmm6, xmm6, xmm5
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm6, xmm6, xmm1
  movdqu xmm15, xmmword ptr [rcx + -16]
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov r11, 13979173243358019584
  pinsrq xmm3, r11, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm7, xmm7, xmm8
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm4, xmm4, xmm5
  movbe r13, qword ptr [r14 + 24]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 16]
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  mov qword ptr [rbp + 96], r13
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 104], r12
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm1, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm1
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm10, xmm10, xmm1
  vpsrldq xmm6, xmm6, 8
  vaesenc xmm11, xmm11, xmm1
  vpxor xmm7, xmm7, xmm6
  vaesenc xmm12, xmm12, xmm1
  vpxor xmm4, xmm4, xmm0
  movbe r13, qword ptr [r14 + 8]
  vaesenc xmm13, xmm13, xmm1
  movbe r12, qword ptr [r14 + 0]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  vaesenc xmm9, xmm9, xmm1
  vaesenc xmm10, xmm10, xmm1
  vaesenc xmm11, xmm11, xmm1
  vaesenc xmm12, xmm12, xmm1
  vaesenc xmm13, xmm13, xmm1
  movdqu xmm15, xmmword ptr [rcx + 48]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  vaesenc xmm9, xmm9, xmm1
  vaesenc xmm10, xmm10, xmm1
  vaesenc xmm11, xmm11, xmm1
  vaesenc xmm12, xmm12, xmm1
  vaesenc xmm13, xmm13, xmm1
  movdqu xmm15, xmmword ptr [rcx + 80]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 96]
  vaesenc xmm9, xmm9, xmm15
  movdqu xmmword ptr [rbp + 16], xmm7
  vpalignr xmm8, xmm4, xmm4, 8
  vaesenc xmm10, xmm10, xmm15
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm2, xmm1, xmmword ptr [rdi + 0]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm0, xmm1, xmmword ptr [rdi + 16]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm5, xmm1, xmmword ptr [rdi + 32]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm6, xmm1, xmmword ptr [rdi + 48]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm7, xmm1, xmmword ptr [rdi + 64]
  vpxor xmm3, xmm1, xmmword ptr [rdi + 80]
  movdqu xmm1, xmmword ptr [rbp + 128]
  vaesenclast xmm9, xmm9, xmm2
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vaesenclast xmm10, xmm10, xmm0
  vpaddd xmm0, xmm1, xmm2
  mov qword ptr [rbp + 112], r13
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm11, xmm11, xmm5
  vpaddd xmm5, xmm0, xmm2
  mov qword ptr [rbp + 120], r12
  lea rsi, qword ptr [rsi + 96]
  movdqu xmm15, xmmword ptr [rcx + -128]
  vaesenclast xmm12, xmm12, xmm6
  vpaddd xmm6, xmm5, xmm2
  vaesenclast xmm13, xmm13, xmm7
  vpaddd xmm7, xmm6, xmm2
  vaesenclast xmm14, xmm14, xmm3
  vpaddd xmm3, xmm7, xmm2
  sub rdx, 6
  add r14, 96
  cmp rdx, 0
  jbe L126
  movdqu xmmword ptr [rsi + -96], xmm9
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmm11, xmm5
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmm12, xmm6
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmm13, xmm7
  movdqu xmmword ptr [rsi + -16], xmm14
  movdqu xmm14, xmm3
  movdqu xmm7, xmmword ptr [rbp + 32]
  jmp L127
L126:
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpxor xmm8, xmm8, xmm4
L127:
ALIGN 16
L123:
  cmp rdx, 0
  ja L122
  movdqu xmm7, xmmword ptr [rbp + 32]
  movdqu xmmword ptr [rbp + 32], xmm1
  pxor xmm4, xmm4
  movdqu xmmword ptr [rbp + 16], xmm4
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vpclmulqdq xmm5, xmm7, xmm3, 16
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vpclmulqdq xmm7, xmm7, xmm3, 17
  movdqu xmm3, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  movdqu xmm5, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vpxor xmm6, xmm6, xmm5
  vpxor xmm6, xmm6, xmm1
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov rax, 3254779904
  pinsrd xmm3, eax, 3
  vpxor xmm7, xmm7, xmm8
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  mov r12, 579005069656919567
  pinsrq xmm0, r12, 0
  mov r12, 283686952306183
  pinsrq xmm0, r12, 1
  movdqu xmmword ptr [rsi + -96], xmm9
  vpshufb xmm9, xmm9, xmm0
  vpxor xmm1, xmm1, xmm7
  movdqu xmmword ptr [rsi + -80], xmm10
  vpshufb xmm10, xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  vpshufb xmm11, xmm11, xmm0
  movdqu xmmword ptr [rsi + -48], xmm12
  vpshufb xmm12, xmm12, xmm0
  movdqu xmmword ptr [rsi + -32], xmm13
  vpshufb xmm13, xmm13, xmm0
  movdqu xmmword ptr [rsi + -16], xmm14
  vpshufb xmm14, xmm14, xmm0
  pxor xmm4, xmm4
  movdqu xmm7, xmm14
  movdqu xmmword ptr [rbp + 16], xmm4
  movdqu xmmword ptr [rbp + 48], xmm13
  movdqu xmmword ptr [rbp + 64], xmm12
  movdqu xmmword ptr [rbp + 80], xmm11
  movdqu xmmword ptr [rbp + 96], xmm10
  movdqu xmmword ptr [rbp + 112], xmm9
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vpclmulqdq xmm5, xmm7, xmm3, 16
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vpclmulqdq xmm7, xmm7, xmm3, 17
  movdqu xmm3, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  movdqu xmm5, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vpxor xmm6, xmm6, xmm5
  vpxor xmm6, xmm6, xmm1
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov rax, 3254779904
  pinsrd xmm3, eax, 3
  vpxor xmm7, xmm7, xmm8
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  sub rcx, 128
L117:
  movdqu xmm11, xmmword ptr [rbp + 32]
  mov r8, rcx
  mov rax, qword ptr [rsp + 312]
  mov rdi, qword ptr [rsp + 320]
  mov rdx, qword ptr [rsp + 328]
  mov r14, rdx
  mov r12, 579005069656919567
  pinsrq xmm9, r12, 0
  mov r12, 283686952306183
  pinsrq xmm9, r12, 1
  pshufb xmm11, xmm9
  pxor xmm10, xmm10
  mov rbx, 1
  pinsrd xmm10, ebx, 0
  mov r11, rax
  mov r10, rdi
  mov rbx, 0
  jmp L129
ALIGN 16
L128:
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r11 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rbx, 1
  add r11, 16
  add r10, 16
  paddd xmm11, xmm10
ALIGN 16
L129:
  cmp rbx, rdx
  jne L128
  mov r11, rdi
  jmp L131
ALIGN 16
L130:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L131:
  cmp rdx, 6
  jae L130
  cmp rdx, 0
  jbe L132
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L134
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L135
L134:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L136
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L138
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L140
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L141
L140:
L141:
  jmp L139
L138:
L139:
  jmp L137
L136:
L137:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L135:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L133
L132:
L133:
  add r14, qword ptr [rsp + 304]
  imul r14, 16
  mov r13, qword ptr [rsp + 344]
  cmp r13, r14
  jbe L142
  mov rax, qword ptr [rsp + 336]
  mov r10, r13
  and r10, 15
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm4, xmmword ptr [rax + 0]
  pxor xmm0, xmm4
  movdqu xmmword ptr [rax + 0], xmm0
  cmp r10, 8
  jae L144
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L145
L144:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L145:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L143
L142:
L143:
  mov r11, r15
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmmword ptr [rbp + 0]
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm8, xmm9
  pxor xmm8, xmm0
  mov r15, qword ptr [rsp + 360]
  movdqu xmmword ptr [r15 + 0], xmm8
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  ret
gcm256_encrypt_opt endp
ALIGN 16
gcm128_decrypt_opt proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 264]
  mov r9, qword ptr [rsp + 272]
  mov rbp, qword ptr [rsp + 352]
  mov r13, rcx
  lea r9, qword ptr [r9 + 32]
  mov rbx, qword ptr [rsp + 280]
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L147
ALIGN 16
L146:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L147:
  cmp rdx, 6
  jae L146
  cmp rdx, 0
  jbe L148
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L150
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L151
L150:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L152
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L154
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L156
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L157
L156:
L157:
  jmp L155
L154:
L155:
  jmp L153
L152:
L153:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L151:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L149
L148:
L149:
  mov r15, rsi
  cmp rsi, rcx
  jbe L158
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L160
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L161
L160:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L161:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L159
L158:
L159:
  mov rdi, qword ptr [rsp + 288]
  mov rsi, qword ptr [rsp + 296]
  mov rdx, qword ptr [rsp + 304]
  mov rcx, r13
  movdqu xmm0, xmm9
  movdqu xmm1, xmmword ptr [r8 + 0]
  movdqu xmmword ptr [rbp + 0], xmm1
  pxor xmm10, xmm10
  mov r11, 1
  pinsrq xmm10, r11, 0
  vpaddd xmm1, xmm1, xmm10
  cmp rdx, 0
  jne L162
  vpshufb xmm1, xmm1, xmm0
  movdqu xmmword ptr [rbp + 32], xmm1
  jmp L163
L162:
  movdqu xmmword ptr [rbp + 32], xmm8
  add rcx, 128
  pextrq rbx, xmm1, 0
  and rbx, 255
  vpshufb xmm1, xmm1, xmm0
  lea r14, qword ptr [rdi + 96]
  movdqu xmm8, xmmword ptr [rbp + 32]
  movdqu xmm7, xmmword ptr [rdi + 80]
  movdqu xmm4, xmmword ptr [rdi + 64]
  movdqu xmm5, xmmword ptr [rdi + 48]
  movdqu xmm6, xmmword ptr [rdi + 32]
  vpshufb xmm7, xmm7, xmm0
  movdqu xmm2, xmmword ptr [rdi + 16]
  vpshufb xmm4, xmm4, xmm0
  movdqu xmm3, xmmword ptr [rdi + 0]
  vpshufb xmm5, xmm5, xmm0
  movdqu xmmword ptr [rbp + 48], xmm4
  vpshufb xmm6, xmm6, xmm0
  movdqu xmmword ptr [rbp + 64], xmm5
  vpshufb xmm2, xmm2, xmm0
  movdqu xmmword ptr [rbp + 80], xmm6
  vpshufb xmm3, xmm3, xmm0
  movdqu xmmword ptr [rbp + 96], xmm2
  movdqu xmmword ptr [rbp + 112], xmm3
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vpxor xmm4, xmm4, xmm4
  movdqu xmm15, xmmword ptr [rcx + -128]
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpaddd xmm12, xmm11, xmm2
  vpaddd xmm13, xmm12, xmm2
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rbp + 16], xmm4
  cmp rdx, 6
  jne L164
  sub r14, 96
  jmp L165
L164:
L165:
  jmp L167
ALIGN 16
L166:
  add rbx, 6
  cmp rbx, 256
  jb L168
  mov r11, 579005069656919567
  pinsrq xmm0, r11, 0
  mov r11, 283686952306183
  pinsrq xmm0, r11, 1
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm15
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm15
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpshufb xmm14, xmm14, xmm0
  vpshufb xmm1, xmm1, xmm0
  sub rbx, 256
  jmp L169
L168:
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm1, xmm2, xmm14
  vpxor xmm10, xmm10, xmm15
  vpxor xmm11, xmm11, xmm15
L169:
  movdqu xmmword ptr [rbp + 128], xmm1
  vpclmulqdq xmm5, xmm7, xmm3, 16
  vpxor xmm12, xmm12, xmm15
  movdqu xmm2, xmmword ptr [rcx + -112]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vaesenc xmm9, xmm9, xmm2
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpxor xmm13, xmm13, xmm15
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vaesenc xmm10, xmm10, xmm2
  vpxor xmm14, xmm14, xmm15
  vpclmulqdq xmm7, xmm7, xmm3, 17
  vaesenc xmm11, xmm11, xmm2
  movdqu xmm3, xmmword ptr [r9 + -16]
  vaesenc xmm12, xmm12, xmm2
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vaesenc xmm13, xmm13, xmm2
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vaesenc xmm14, xmm14, xmm2
  movdqu xmm15, xmmword ptr [rcx + -96]
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 88]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 80]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 32], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 40], r12
  movdqu xmm5, xmmword ptr [r9 + 16]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vaesenc xmm11, xmm11, xmm15
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 72]
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 64]
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 48], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 56], r12
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 56]
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 48]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 64], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 72], r12
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 40]
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 32]
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 80], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 88], r12
  vpxor xmm6, xmm6, xmm5
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm6, xmm6, xmm1
  movdqu xmm15, xmmword ptr [rcx + -16]
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov r11, 13979173243358019584
  pinsrq xmm3, r11, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm7, xmm7, xmm8
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm4, xmm4, xmm5
  movbe r13, qword ptr [r14 + 24]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 16]
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  mov qword ptr [rbp + 96], r13
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 104], r12
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm1, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm1
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm10, xmm10, xmm1
  vpsrldq xmm6, xmm6, 8
  vaesenc xmm11, xmm11, xmm1
  vpxor xmm7, xmm7, xmm6
  vaesenc xmm12, xmm12, xmm1
  vpxor xmm4, xmm4, xmm0
  movbe r13, qword ptr [r14 + 8]
  vaesenc xmm13, xmm13, xmm1
  movbe r12, qword ptr [r14 + 0]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  movdqu xmmword ptr [rbp + 16], xmm7
  vpalignr xmm8, xmm4, xmm4, 8
  vaesenc xmm10, xmm10, xmm15
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm2, xmm1, xmmword ptr [rdi + 0]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm0, xmm1, xmmword ptr [rdi + 16]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm5, xmm1, xmmword ptr [rdi + 32]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm6, xmm1, xmmword ptr [rdi + 48]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm7, xmm1, xmmword ptr [rdi + 64]
  vpxor xmm3, xmm1, xmmword ptr [rdi + 80]
  movdqu xmm1, xmmword ptr [rbp + 128]
  vaesenclast xmm9, xmm9, xmm2
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vaesenclast xmm10, xmm10, xmm0
  vpaddd xmm0, xmm1, xmm2
  mov qword ptr [rbp + 112], r13
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm11, xmm11, xmm5
  vpaddd xmm5, xmm0, xmm2
  mov qword ptr [rbp + 120], r12
  lea rsi, qword ptr [rsi + 96]
  movdqu xmm15, xmmword ptr [rcx + -128]
  vaesenclast xmm12, xmm12, xmm6
  vpaddd xmm6, xmm5, xmm2
  vaesenclast xmm13, xmm13, xmm7
  vpaddd xmm7, xmm6, xmm2
  vaesenclast xmm14, xmm14, xmm3
  vpaddd xmm3, xmm7, xmm2
  sub rdx, 6
  cmp rdx, 6
  jbe L170
  add r14, 96
  jmp L171
L170:
L171:
  cmp rdx, 0
  jbe L172
  movdqu xmmword ptr [rsi + -96], xmm9
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmm11, xmm5
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmm12, xmm6
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmm13, xmm7
  movdqu xmmword ptr [rsi + -16], xmm14
  movdqu xmm14, xmm3
  movdqu xmm7, xmmword ptr [rbp + 32]
  jmp L173
L172:
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpxor xmm8, xmm8, xmm4
L173:
ALIGN 16
L167:
  cmp rdx, 0
  ja L166
  movdqu xmmword ptr [rbp + 32], xmm1
  movdqu xmmword ptr [rsi + -96], xmm9
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmmword ptr [rsi + -16], xmm14
  sub rcx, 128
L163:
  movdqu xmm11, xmmword ptr [rbp + 32]
  mov r8, rcx
  mov rax, qword ptr [rsp + 312]
  mov rdi, qword ptr [rsp + 320]
  mov rdx, qword ptr [rsp + 328]
  mov r14, rdx
  mov r12, 579005069656919567
  pinsrq xmm9, r12, 0
  mov r12, 283686952306183
  pinsrq xmm9, r12, 1
  pshufb xmm11, xmm9
  mov rbx, rdi
  mov r12, rdx
  mov rdi, rax
  mov r11, rdi
  jmp L175
ALIGN 16
L174:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L175:
  cmp rdx, 6
  jae L174
  cmp rdx, 0
  jbe L176
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L178
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L179
L178:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L180
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L182
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L184
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L185
L184:
L185:
  jmp L183
L182:
L183:
  jmp L181
L180:
L181:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L179:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L177
L176:
L177:
  mov rdi, rbx
  mov rdx, r12
  pxor xmm10, xmm10
  mov rbx, 1
  pinsrd xmm10, ebx, 0
  mov r11, rax
  mov r10, rdi
  mov rbx, 0
  jmp L187
ALIGN 16
L186:
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r11 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rbx, 1
  add r11, 16
  add r10, 16
  paddd xmm11, xmm10
ALIGN 16
L187:
  cmp rbx, rdx
  jne L186
  add r14, qword ptr [rsp + 304]
  imul r14, 16
  mov r13, qword ptr [rsp + 344]
  cmp r13, r14
  jbe L188
  mov rax, qword ptr [rsp + 336]
  mov r10, r13
  and r10, 15
  movdqu xmm0, xmmword ptr [rax + 0]
  movdqu xmm10, xmm0
  cmp r10, 8
  jae L190
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L191
L190:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L191:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pxor xmm10, xmm0
  movdqu xmmword ptr [rax + 0], xmm10
  jmp L189
L188:
L189:
  mov r11, r15
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmmword ptr [rbp + 0]
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm8, xmm9
  pxor xmm8, xmm0
  mov r15, qword ptr [rsp + 360]
  movdqu xmm0, xmmword ptr [r15 + 0]
  pcmpeqd xmm0, xmm8
  pextrq rdx, xmm0, 0
  sub rdx, 18446744073709551615
  mov rax, 0
  adc rax, 0
  pextrq rdx, xmm0, 1
  sub rdx, 18446744073709551615
  mov rdx, 0
  adc rdx, 0
  add rax, rdx
  mov rcx, rax
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  mov rax, rcx
  ret
gcm128_decrypt_opt endp
ALIGN 16
gcm256_decrypt_opt proc
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  pextrq rax, xmm15, 0
  push rax
  pextrq rax, xmm15, 1
  push rax
  pextrq rax, xmm14, 0
  push rax
  pextrq rax, xmm14, 1
  push rax
  pextrq rax, xmm13, 0
  push rax
  pextrq rax, xmm13, 1
  push rax
  pextrq rax, xmm12, 0
  push rax
  pextrq rax, xmm12, 1
  push rax
  pextrq rax, xmm11, 0
  push rax
  pextrq rax, xmm11, 1
  push rax
  pextrq rax, xmm10, 0
  push rax
  pextrq rax, xmm10, 1
  push rax
  pextrq rax, xmm9, 0
  push rax
  pextrq rax, xmm9, 1
  push rax
  pextrq rax, xmm8, 0
  push rax
  pextrq rax, xmm8, 1
  push rax
  pextrq rax, xmm7, 0
  push rax
  pextrq rax, xmm7, 1
  push rax
  pextrq rax, xmm6, 0
  push rax
  pextrq rax, xmm6, 1
  push rax
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  mov r8, qword ptr [rsp + 264]
  mov r9, qword ptr [rsp + 272]
  mov rbp, qword ptr [rsp + 352]
  mov r13, rcx
  lea r9, qword ptr [r9 + 32]
  mov rbx, qword ptr [rsp + 280]
  mov rcx, rdx
  imul rcx, 16
  mov r10, 579005069656919567
  pinsrq xmm9, r10, 0
  mov r10, 283686952306183
  pinsrq xmm9, r10, 1
  pxor xmm8, xmm8
  mov r11, rdi
  jmp L193
ALIGN 16
L192:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L193:
  cmp rdx, 6
  jae L192
  cmp rdx, 0
  jbe L194
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L196
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L197
L196:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L198
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L200
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L202
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L203
L202:
L203:
  jmp L201
L200:
L201:
  jmp L199
L198:
L199:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L197:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L195
L194:
L195:
  mov r15, rsi
  cmp rsi, rcx
  jbe L204
  movdqu xmm0, xmmword ptr [rbx + 0]
  mov r10, rsi
  and r10, 15
  cmp r10, 8
  jae L206
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L207
L206:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L207:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L205
L204:
L205:
  mov rdi, qword ptr [rsp + 288]
  mov rsi, qword ptr [rsp + 296]
  mov rdx, qword ptr [rsp + 304]
  mov rcx, r13
  movdqu xmm0, xmm9
  movdqu xmm1, xmmword ptr [r8 + 0]
  movdqu xmmword ptr [rbp + 0], xmm1
  pxor xmm10, xmm10
  mov r11, 1
  pinsrq xmm10, r11, 0
  vpaddd xmm1, xmm1, xmm10
  cmp rdx, 0
  jne L208
  vpshufb xmm1, xmm1, xmm0
  movdqu xmmword ptr [rbp + 32], xmm1
  jmp L209
L208:
  movdqu xmmword ptr [rbp + 32], xmm8
  add rcx, 128
  pextrq rbx, xmm1, 0
  and rbx, 255
  vpshufb xmm1, xmm1, xmm0
  lea r14, qword ptr [rdi + 96]
  movdqu xmm8, xmmword ptr [rbp + 32]
  movdqu xmm7, xmmword ptr [rdi + 80]
  movdqu xmm4, xmmword ptr [rdi + 64]
  movdqu xmm5, xmmword ptr [rdi + 48]
  movdqu xmm6, xmmword ptr [rdi + 32]
  vpshufb xmm7, xmm7, xmm0
  movdqu xmm2, xmmword ptr [rdi + 16]
  vpshufb xmm4, xmm4, xmm0
  movdqu xmm3, xmmword ptr [rdi + 0]
  vpshufb xmm5, xmm5, xmm0
  movdqu xmmword ptr [rbp + 48], xmm4
  vpshufb xmm6, xmm6, xmm0
  movdqu xmmword ptr [rbp + 64], xmm5
  vpshufb xmm2, xmm2, xmm0
  movdqu xmmword ptr [rbp + 80], xmm6
  vpshufb xmm3, xmm3, xmm0
  movdqu xmmword ptr [rbp + 96], xmm2
  movdqu xmmword ptr [rbp + 112], xmm3
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vpxor xmm4, xmm4, xmm4
  movdqu xmm15, xmmword ptr [rcx + -128]
  vpaddd xmm10, xmm1, xmm2
  vpaddd xmm11, xmm10, xmm2
  vpaddd xmm12, xmm11, xmm2
  vpaddd xmm13, xmm12, xmm2
  vpaddd xmm14, xmm13, xmm2
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rbp + 16], xmm4
  cmp rdx, 6
  jne L210
  sub r14, 96
  jmp L211
L210:
L211:
  jmp L213
ALIGN 16
L212:
  add rbx, 6
  cmp rbx, 256
  jb L214
  mov r11, 579005069656919567
  pinsrq xmm0, r11, 0
  mov r11, 283686952306183
  pinsrq xmm0, r11, 1
  vpshufb xmm6, xmm1, xmm0
  pxor xmm5, xmm5
  mov r11, 1
  pinsrq xmm5, r11, 0
  vpaddd xmm10, xmm6, xmm5
  pxor xmm5, xmm5
  mov r11, 2
  pinsrq xmm5, r11, 0
  vpaddd xmm11, xmm6, xmm5
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm12, xmm10, xmm5
  vpshufb xmm10, xmm10, xmm0
  vpaddd xmm13, xmm11, xmm5
  vpshufb xmm11, xmm11, xmm0
  vpxor xmm10, xmm10, xmm15
  vpaddd xmm14, xmm12, xmm5
  vpshufb xmm12, xmm12, xmm0
  vpxor xmm11, xmm11, xmm15
  vpaddd xmm1, xmm13, xmm5
  vpshufb xmm13, xmm13, xmm0
  vpshufb xmm14, xmm14, xmm0
  vpshufb xmm1, xmm1, xmm0
  sub rbx, 256
  jmp L215
L214:
  movdqu xmm3, xmmword ptr [r9 + -32]
  vpaddd xmm1, xmm2, xmm14
  vpxor xmm10, xmm10, xmm15
  vpxor xmm11, xmm11, xmm15
L215:
  movdqu xmmword ptr [rbp + 128], xmm1
  vpclmulqdq xmm5, xmm7, xmm3, 16
  vpxor xmm12, xmm12, xmm15
  movdqu xmm2, xmmword ptr [rcx + -112]
  vpclmulqdq xmm6, xmm7, xmm3, 1
  vaesenc xmm9, xmm9, xmm2
  movdqu xmm0, xmmword ptr [rbp + 48]
  vpxor xmm13, xmm13, xmm15
  vpclmulqdq xmm1, xmm7, xmm3, 0
  vaesenc xmm10, xmm10, xmm2
  vpxor xmm14, xmm14, xmm15
  vpclmulqdq xmm7, xmm7, xmm3, 17
  vaesenc xmm11, xmm11, xmm2
  movdqu xmm3, xmmword ptr [r9 + -16]
  vaesenc xmm12, xmm12, xmm2
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm3, 0
  vpxor xmm8, xmm8, xmm4
  vaesenc xmm13, xmm13, xmm2
  vpxor xmm4, xmm1, xmm5
  vpclmulqdq xmm1, xmm0, xmm3, 16
  vaesenc xmm14, xmm14, xmm2
  movdqu xmm15, xmmword ptr [rcx + -96]
  vpclmulqdq xmm2, xmm0, xmm3, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpclmulqdq xmm3, xmm0, xmm3, 17
  movdqu xmm0, xmmword ptr [rbp + 64]
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 88]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 80]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 32], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 40], r12
  movdqu xmm5, xmmword ptr [r9 + 16]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -80]
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm7, xmm7, xmm3
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vaesenc xmm11, xmm11, xmm15
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [rbp + 80]
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -64]
  vpxor xmm6, xmm6, xmm2
  vpclmulqdq xmm2, xmm0, xmm1, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm1, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 72]
  vpxor xmm7, xmm7, xmm5
  vpclmulqdq xmm5, xmm0, xmm1, 1
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 64]
  vpclmulqdq xmm1, xmm0, xmm1, 17
  movdqu xmm0, xmmword ptr [rbp + 96]
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 48], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 56], r12
  vpxor xmm4, xmm4, xmm2
  movdqu xmm2, xmmword ptr [r9 + 64]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -48]
  vpxor xmm6, xmm6, xmm3
  vpclmulqdq xmm3, xmm0, xmm2, 0
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm0, xmm2, 16
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 56]
  vpxor xmm7, xmm7, xmm1
  vpclmulqdq xmm1, xmm0, xmm2, 1
  vpxor xmm8, xmm8, xmmword ptr [rbp + 112]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 48]
  vpclmulqdq xmm2, xmm0, xmm2, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 64], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 72], r12
  vpxor xmm4, xmm4, xmm3
  movdqu xmm3, xmmword ptr [r9 + 80]
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm15, xmmword ptr [rcx + -32]
  vpxor xmm6, xmm6, xmm5
  vpclmulqdq xmm5, xmm8, xmm3, 16
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm6, xmm6, xmm1
  vpclmulqdq xmm1, xmm8, xmm3, 1
  vaesenc xmm10, xmm10, xmm15
  movbe r13, qword ptr [r14 + 40]
  vpxor xmm7, xmm7, xmm2
  vpclmulqdq xmm2, xmm8, xmm3, 0
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 32]
  vpclmulqdq xmm8, xmm8, xmm3, 17
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 80], r13
  vaesenc xmm13, xmm13, xmm15
  mov qword ptr [rbp + 88], r12
  vpxor xmm6, xmm6, xmm5
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm6, xmm6, xmm1
  movdqu xmm15, xmmword ptr [rcx + -16]
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm2
  pxor xmm3, xmm3
  mov r11, 13979173243358019584
  pinsrq xmm3, r11, 1
  vaesenc xmm9, xmm9, xmm15
  vpxor xmm7, xmm7, xmm8
  vaesenc xmm10, xmm10, xmm15
  vpxor xmm4, xmm4, xmm5
  movbe r13, qword ptr [r14 + 24]
  vaesenc xmm11, xmm11, xmm15
  movbe r12, qword ptr [r14 + 16]
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  mov qword ptr [rbp + 96], r13
  vaesenc xmm12, xmm12, xmm15
  mov qword ptr [rbp + 104], r12
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  movdqu xmm1, xmmword ptr [rcx + 0]
  vaesenc xmm9, xmm9, xmm1
  movdqu xmm15, xmmword ptr [rcx + 16]
  vaesenc xmm10, xmm10, xmm1
  vpsrldq xmm6, xmm6, 8
  vaesenc xmm11, xmm11, xmm1
  vpxor xmm7, xmm7, xmm6
  vaesenc xmm12, xmm12, xmm1
  vpxor xmm4, xmm4, xmm0
  movbe r13, qword ptr [r14 + 8]
  vaesenc xmm13, xmm13, xmm1
  movbe r12, qword ptr [r14 + 0]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 32]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  vaesenc xmm9, xmm9, xmm1
  vaesenc xmm10, xmm10, xmm1
  vaesenc xmm11, xmm11, xmm1
  vaesenc xmm12, xmm12, xmm1
  vaesenc xmm13, xmm13, xmm1
  movdqu xmm15, xmmword ptr [rcx + 48]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 64]
  vaesenc xmm9, xmm9, xmm15
  vaesenc xmm10, xmm10, xmm15
  vaesenc xmm11, xmm11, xmm15
  vaesenc xmm12, xmm12, xmm15
  vaesenc xmm13, xmm13, xmm15
  vaesenc xmm14, xmm14, xmm15
  vaesenc xmm9, xmm9, xmm1
  vaesenc xmm10, xmm10, xmm1
  vaesenc xmm11, xmm11, xmm1
  vaesenc xmm12, xmm12, xmm1
  vaesenc xmm13, xmm13, xmm1
  movdqu xmm15, xmmword ptr [rcx + 80]
  vaesenc xmm14, xmm14, xmm1
  movdqu xmm1, xmmword ptr [rcx + 96]
  vaesenc xmm9, xmm9, xmm15
  movdqu xmmword ptr [rbp + 16], xmm7
  vpalignr xmm8, xmm4, xmm4, 8
  vaesenc xmm10, xmm10, xmm15
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm2, xmm1, xmmword ptr [rdi + 0]
  vaesenc xmm11, xmm11, xmm15
  vpxor xmm0, xmm1, xmmword ptr [rdi + 16]
  vaesenc xmm12, xmm12, xmm15
  vpxor xmm5, xmm1, xmmword ptr [rdi + 32]
  vaesenc xmm13, xmm13, xmm15
  vpxor xmm6, xmm1, xmmword ptr [rdi + 48]
  vaesenc xmm14, xmm14, xmm15
  vpxor xmm7, xmm1, xmmword ptr [rdi + 64]
  vpxor xmm3, xmm1, xmmword ptr [rdi + 80]
  movdqu xmm1, xmmword ptr [rbp + 128]
  vaesenclast xmm9, xmm9, xmm2
  pxor xmm2, xmm2
  mov r11, 72057594037927936
  pinsrq xmm2, r11, 1
  vaesenclast xmm10, xmm10, xmm0
  vpaddd xmm0, xmm1, xmm2
  mov qword ptr [rbp + 112], r13
  lea rdi, qword ptr [rdi + 96]
  vaesenclast xmm11, xmm11, xmm5
  vpaddd xmm5, xmm0, xmm2
  mov qword ptr [rbp + 120], r12
  lea rsi, qword ptr [rsi + 96]
  movdqu xmm15, xmmword ptr [rcx + -128]
  vaesenclast xmm12, xmm12, xmm6
  vpaddd xmm6, xmm5, xmm2
  vaesenclast xmm13, xmm13, xmm7
  vpaddd xmm7, xmm6, xmm2
  vaesenclast xmm14, xmm14, xmm3
  vpaddd xmm3, xmm7, xmm2
  sub rdx, 6
  cmp rdx, 6
  jbe L216
  add r14, 96
  jmp L217
L216:
L217:
  cmp rdx, 0
  jbe L218
  movdqu xmmword ptr [rsi + -96], xmm9
  vpxor xmm9, xmm1, xmm15
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmm10, xmm0
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmm11, xmm5
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmm12, xmm6
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmm13, xmm7
  movdqu xmmword ptr [rsi + -16], xmm14
  movdqu xmm14, xmm3
  movdqu xmm7, xmmword ptr [rbp + 32]
  jmp L219
L218:
  vpxor xmm8, xmm8, xmmword ptr [rbp + 16]
  vpxor xmm8, xmm8, xmm4
L219:
ALIGN 16
L213:
  cmp rdx, 0
  ja L212
  movdqu xmmword ptr [rbp + 32], xmm1
  movdqu xmmword ptr [rsi + -96], xmm9
  movdqu xmmword ptr [rsi + -80], xmm10
  movdqu xmmword ptr [rsi + -64], xmm11
  movdqu xmmword ptr [rsi + -48], xmm12
  movdqu xmmword ptr [rsi + -32], xmm13
  movdqu xmmword ptr [rsi + -16], xmm14
  sub rcx, 128
L209:
  movdqu xmm11, xmmword ptr [rbp + 32]
  mov r8, rcx
  mov rax, qword ptr [rsp + 312]
  mov rdi, qword ptr [rsp + 320]
  mov rdx, qword ptr [rsp + 328]
  mov r14, rdx
  mov r12, 579005069656919567
  pinsrq xmm9, r12, 0
  mov r12, 283686952306183
  pinsrq xmm9, r12, 1
  pshufb xmm11, xmm9
  mov rbx, rdi
  mov r12, rdx
  mov rdi, rax
  mov r11, rdi
  jmp L221
ALIGN 16
L220:
  add r11, 80
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 80]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  add r11, 96
  sub rdx, 6
ALIGN 16
L221:
  cmp rdx, 6
  jae L220
  cmp rdx, 0
  jbe L222
  mov r10, rdx
  sub r10, 1
  imul r10, 16
  add r11, r10
  movdqu xmm5, xmmword ptr [r9 + -32]
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  cmp rdx, 1
  jne L224
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  jmp L225
L224:
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  movdqu xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + -16]
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 2
  je L226
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 16]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 3
  je L228
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 32]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  cmp rdx, 4
  je L230
  sub r11, 16
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm0, xmmword ptr [r11 + 0]
  pshufb xmm0, xmm9
  vpxor xmm4, xmm4, xmm1
  movdqu xmm1, xmmword ptr [r9 + 64]
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
  movdqu xmm5, xmm1
  jmp L231
L230:
L231:
  jmp L229
L228:
L229:
  jmp L227
L226:
L227:
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  vpxor xmm4, xmm4, xmm1
  vpxor xmm6, xmm6, xmm2
  vpxor xmm6, xmm6, xmm3
  vpxor xmm7, xmm7, xmm5
L225:
  pxor xmm3, xmm3
  mov r10, 3254779904
  pinsrd xmm3, r10d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  jmp L223
L222:
L223:
  mov rdi, rbx
  mov rdx, r12
  pxor xmm10, xmm10
  mov rbx, 1
  pinsrd xmm10, ebx, 0
  mov r11, rax
  mov r10, rdi
  mov rbx, 0
  jmp L233
ALIGN 16
L232:
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  movdqu xmm2, xmmword ptr [r11 + 0]
  pxor xmm2, xmm0
  movdqu xmmword ptr [r10 + 0], xmm2
  add rbx, 1
  add r11, 16
  add r10, 16
  paddd xmm11, xmm10
ALIGN 16
L233:
  cmp rbx, rdx
  jne L232
  add r14, qword ptr [rsp + 304]
  imul r14, 16
  mov r13, qword ptr [rsp + 344]
  cmp r13, r14
  jbe L234
  mov rax, qword ptr [rsp + 336]
  mov r10, r13
  and r10, 15
  movdqu xmm0, xmmword ptr [rax + 0]
  movdqu xmm10, xmm0
  cmp r10, 8
  jae L236
  mov rcx, 0
  pinsrq xmm0, rcx, 1
  mov rcx, r10
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 0
  and rcx, r11
  pinsrq xmm0, rcx, 0
  jmp L237
L236:
  mov rcx, r10
  sub rcx, 8
  shl rcx, 3
  mov r11, 1
  shl r11, cl
  sub r11, 1
  pextrq rcx, xmm0, 1
  and rcx, r11
  pinsrq xmm0, rcx, 1
L237:
  pshufb xmm0, xmm9
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmm11
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pxor xmm10, xmm0
  movdqu xmmword ptr [rax + 0], xmm10
  jmp L235
L234:
L235:
  mov r11, r15
  pxor xmm0, xmm0
  mov rax, r11
  imul rax, 8
  pinsrq xmm0, rax, 1
  mov rax, r13
  imul rax, 8
  pinsrq xmm0, rax, 0
  movdqu xmm5, xmmword ptr [r9 + -32]
  vpxor xmm0, xmm8, xmm0
  vpclmulqdq xmm1, xmm0, xmm5, 0
  vpclmulqdq xmm2, xmm0, xmm5, 16
  vpclmulqdq xmm3, xmm0, xmm5, 1
  vpclmulqdq xmm5, xmm0, xmm5, 17
  movdqu xmm4, xmm1
  vpxor xmm6, xmm2, xmm3
  movdqu xmm7, xmm5
  pxor xmm3, xmm3
  mov r11, 3254779904
  pinsrd xmm3, r11d, 3
  vpslldq xmm5, xmm6, 8
  vpxor xmm4, xmm4, xmm5
  vpalignr xmm0, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpsrldq xmm6, xmm6, 8
  vpxor xmm7, xmm7, xmm6
  vpxor xmm4, xmm4, xmm0
  vpalignr xmm8, xmm4, xmm4, 8
  vpclmulqdq xmm4, xmm4, xmm3, 16
  vpxor xmm8, xmm8, xmm7
  vpxor xmm8, xmm8, xmm4
  movdqu xmm0, xmmword ptr [rbp + 0]
  pshufb xmm0, xmm9
  movdqu xmm2, xmmword ptr [r8 + 0]
  pxor xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 16]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 32]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 48]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 64]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 80]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 96]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 112]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 128]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 144]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 160]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 176]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 192]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 208]
  aesenc xmm0, xmm2
  movdqu xmm2, xmmword ptr [r8 + 224]
  aesenclast xmm0, xmm2
  pxor xmm2, xmm2
  pshufb xmm8, xmm9
  pxor xmm8, xmm0
  mov r15, qword ptr [rsp + 360]
  movdqu xmm0, xmmword ptr [r15 + 0]
  pcmpeqd xmm0, xmm8
  pextrq rdx, xmm0, 0
  sub rdx, 18446744073709551615
  mov rax, 0
  adc rax, 0
  pextrq rdx, xmm0, 1
  sub rdx, 18446744073709551615
  mov rdx, 0
  adc rdx, 0
  add rax, rdx
  mov rcx, rax
  pop rax
  pinsrq xmm6, rax, 1
  pop rax
  pinsrq xmm6, rax, 0
  pop rax
  pinsrq xmm7, rax, 1
  pop rax
  pinsrq xmm7, rax, 0
  pop rax
  pinsrq xmm8, rax, 1
  pop rax
  pinsrq xmm8, rax, 0
  pop rax
  pinsrq xmm9, rax, 1
  pop rax
  pinsrq xmm9, rax, 0
  pop rax
  pinsrq xmm10, rax, 1
  pop rax
  pinsrq xmm10, rax, 0
  pop rax
  pinsrq xmm11, rax, 1
  pop rax
  pinsrq xmm11, rax, 0
  pop rax
  pinsrq xmm12, rax, 1
  pop rax
  pinsrq xmm12, rax, 0
  pop rax
  pinsrq xmm13, rax, 1
  pop rax
  pinsrq xmm13, rax, 0
  pop rax
  pinsrq xmm14, rax, 1
  pop rax
  pinsrq xmm14, rax, 0
  pop rax
  pinsrq xmm15, rax, 1
  pop rax
  pinsrq xmm15, rax, 0
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
  mov rax, rcx
  ret
gcm256_decrypt_opt endp
end
