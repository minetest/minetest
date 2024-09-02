.code
ALIGN 16
sha256_update proc
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
  push r15
  push r14
  push r13
  push r12
  push rsi
  push rdi
  push rbp
  push rbx
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  mov rcx, r9
  movdqu xmm1, xmmword ptr [rdi + 0]
  movdqu xmm2, xmmword ptr [rdi + 16]
  mov rax, 289644378169868803
  pinsrq xmm7, rax, 0
  mov rax, 868365760874482187
  pinsrq xmm7, rax, 1
  pshufd xmm0, xmm1, 27
  pshufd xmm1, xmm1, 177
  pshufd xmm2, xmm2, 27
  movdqu xmm8, xmm7
  palignr xmm1, xmm2, 8
  shufpd xmm2, xmm0, 0
  jmp L1
ALIGN 16
L0:
  movdqu xmm3, xmmword ptr [rsi + 0]
  movdqu xmm4, xmmword ptr [rsi + 16]
  movdqu xmm5, xmmword ptr [rsi + 32]
  pshufb xmm3, xmm7
  movdqu xmm6, xmmword ptr [rsi + 48]
  movdqu xmm0, xmmword ptr [rcx + 0]
  paddd xmm0, xmm3
  pshufb xmm4, xmm7
  movdqu xmm10, xmm2
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm9, xmm1
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm0, xmmword ptr [rcx + 16]
  paddd xmm0, xmm4
  pshufb xmm5, xmm7
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  add rsi, 64
  sha256msg1 xmm3, xmm4
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm0, xmmword ptr [rcx + 32]
  paddd xmm0, xmm5
  pshufb xmm6, xmm7
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm6
  palignr xmm7, xmm5, 4
  paddd xmm3, xmm7
  sha256msg1 xmm4, xmm5
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm0, xmmword ptr [rcx + 48]
  paddd xmm0, xmm6
  sha256msg2 xmm3, xmm6
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm3
  palignr xmm7, xmm6, 4
  paddd xmm4, xmm7
  sha256msg1 xmm5, xmm6
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm0, xmmword ptr [rcx + 64]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 80]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 96]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 112]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 128]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 144]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 160]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 176]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 192]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  paddd xmm5, xmm7
  sha256msg1 xmm6, xmm3
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm7, xmm3
  movdqu xmm0, xmm6
  movdqu xmm3, xmm4
  movdqu xmm4, xmm5
  movdqu xmm6, xmm7
  movdqu xmm5, xmm0
  movdqu xmm0, xmmword ptr [rcx + 208]
  paddd xmm0, xmm3
  sha256msg2 xmm4, xmm3
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  movdqu xmm7, xmm4
  palignr xmm7, xmm3, 4
  sha256rnds2 xmm1, xmm2, xmm0
  paddd xmm5, xmm7
  movdqu xmm0, xmmword ptr [rcx + 224]
  paddd xmm0, xmm4
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  sha256msg2 xmm5, xmm4
  movdqu xmm7, xmm8
  sha256rnds2 xmm1, xmm2, xmm0
  movdqu xmm0, xmmword ptr [rcx + 240]
  paddd xmm0, xmm5
  sha256rnds2 xmm2, xmm1, xmm0
  pshufd xmm0, xmm0, 14
  sub rdx, 1
  sha256rnds2 xmm1, xmm2, xmm0
  paddd xmm2, xmm10
  paddd xmm1, xmm9
ALIGN 16
L1:
  cmp rdx, 0
  ja L0
  pshufd xmm2, xmm2, 177
  pshufd xmm7, xmm1, 27
  pshufd xmm1, xmm1, 177
  shufpd xmm1, xmm2, 3
  palignr xmm2, xmm7, 8
  movdqu xmmword ptr [rdi + 0], xmm1
  movdqu xmmword ptr [rdi + 16], xmm2
  pop rbx
  pop rbp
  pop rdi
  pop rsi
  pop r12
  pop r13
  pop r14
  pop r15
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
  ret
sha256_update endp
end
