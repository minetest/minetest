.code
ALIGN 16
add_scalar_e proc
  push rdi
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  ;# Clear registers to propagate the carry bit
  xor r8d, r8d
  xor r9d, r9d
  xor r10d, r10d
  xor r11d, r11d
  xor eax, eax
  
  ;# Begin addition chain
  add rdx, qword ptr [rsi + 0]
  mov qword ptr [rdi + 0], rdx
  adcx r8, qword ptr [rsi + 8]
  mov qword ptr [rdi + 8], r8
  adcx r9, qword ptr [rsi + 16]
  mov qword ptr [rdi + 16], r9
  adcx r10, qword ptr [rsi + 24]
  mov qword ptr [rdi + 24], r10
  
  ;# Return the carry bit in a register
  adcx rax, r11
  pop rsi
  pop rdi
  ret
add_scalar_e endp
ALIGN 16
fadd_e proc
  push rdi
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  ;# Compute the raw addition of f1 + f2
  mov r8, qword ptr [rdx + 0]
  add r8, qword ptr [rsi + 0]
  mov r9, qword ptr [rdx + 8]
  adcx r9, qword ptr [rsi + 8]
  mov r10, qword ptr [rdx + 16]
  adcx r10, qword ptr [rsi + 16]
  mov r11, qword ptr [rdx + 24]
  adcx r11, qword ptr [rsi + 24]
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov rax, 0
  mov rdx, 38
  cmovc rax, rdx
  
  ;# Step 2: Add carry*38 to the original sum
  xor ecx, ecx
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  pop rsi
  pop rdi
  ret
fadd_e endp
ALIGN 16
fsub_e proc
  push rdi
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  ;# Compute the raw substraction of f1-f2
  mov r8, qword ptr [rsi + 0]
  sub r8, qword ptr [rdx + 0]
  mov r9, qword ptr [rsi + 8]
  sbb r9, qword ptr [rdx + 8]
  mov r10, qword ptr [rsi + 16]
  sbb r10, qword ptr [rdx + 16]
  mov r11, qword ptr [rsi + 24]
  sbb r11, qword ptr [rdx + 24]
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov rax, 0
  mov rcx, 38
  cmovc rax, rcx
  
  ;# Step 2: Substract carry*38 from the original difference
  sub r8, rax
  sbb r9, 0
  sbb r10, 0
  sbb r11, 0
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rcx
  sub r8, rax
  
  ;# Store the result
  mov qword ptr [rdi + 0], r8
  mov qword ptr [rdi + 8], r9
  mov qword ptr [rdi + 16], r10
  mov qword ptr [rdi + 24], r11
  pop rsi
  pop rdi
  ret
fsub_e endp
ALIGN 16
fmul_scalar_e proc
  push rdi
  push r13
  push rbx
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  ;# Compute the raw multiplication of f1*f2
  mulx rcx, r8, qword ptr [rsi + 0]
  ;# f1[0]*f2
  mulx rbx, r9, qword ptr [rsi + 8]
  ;# f1[1]*f2
  add r9, rcx
  mov rcx, 0
  mulx r13, r10, qword ptr [rsi + 16]
  ;# f1[2]*f2
  adcx r10, rbx
  mulx rax, r11, qword ptr [rsi + 24]
  ;# f1[3]*f2
  adcx r11, r13
  adcx rax, rcx
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov rdx, 38
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  pop rsi
  pop rbx
  pop r13
  pop rdi
  ret
fmul_scalar_e endp
ALIGN 16
fmul_e proc
  push r13
  push r14
  push r15
  push rbx
  push rsi
  push rdi
  mov rdi, rcx
  mov rsi, rdx
  mov r15, r8
  mov rcx, r9
  ;# Compute the raw multiplication: tmp <- src1 * src2
  ;# Compute src1[0] * src2
  mov rdx, qword ptr [rsi + 0]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  mov qword ptr [rdi + 0], r8
  
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  mov qword ptr [rdi + 8], r10
  
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  mov rax, 0
  adox rax, rdx
  
  
  ;# Compute src1[1] * src2
  mov rdx, qword ptr [rsi + 8]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 8]
  mov qword ptr [rdi + 8], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 16], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[2] * src2
  mov rdx, qword ptr [rsi + 16]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 16]
  mov qword ptr [rdi + 16], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 24], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[3] * src2
  mov rdx, qword ptr [rsi + 24]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 24]
  mov qword ptr [rdi + 24], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 32], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov qword ptr [rdi + 40], rbx
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov qword ptr [rdi + 48], r14
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  mov qword ptr [rdi + 56], rax
  
  
  ;# Line up pointers
  mov rsi, rdi
  mov rdi, r15
  ;# Wrap the result back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 32]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 0]
  mulx rbx, r9, qword ptr [rsi + 40]
  adcx r9, r13
  adox r9, qword ptr [rsi + 8]
  mulx r13, r10, qword ptr [rsi + 48]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 16]
  mulx rax, r11, qword ptr [rsi + 56]
  adcx r11, r13
  adox r11, qword ptr [rsi + 24]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  pop rdi
  pop rsi
  pop rbx
  pop r15
  pop r14
  pop r13
  ret
fmul_e endp
ALIGN 16
fmul2_e proc
  push r13
  push r14
  push r15
  push rbx
  push rsi
  push rdi
  mov rdi, rcx
  mov rsi, rdx
  mov r15, r8
  mov rcx, r9
  ;# Compute the raw multiplication tmp[0] <- f1[0] * f2[0]
  ;# Compute src1[0] * src2
  mov rdx, qword ptr [rsi + 0]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  mov qword ptr [rdi + 0], r8
  
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  mov qword ptr [rdi + 8], r10
  
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  mov rax, 0
  adox rax, rdx
  
  
  ;# Compute src1[1] * src2
  mov rdx, qword ptr [rsi + 8]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 8]
  mov qword ptr [rdi + 8], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 16], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[2] * src2
  mov rdx, qword ptr [rsi + 16]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 16]
  mov qword ptr [rdi + 16], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 24], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[3] * src2
  mov rdx, qword ptr [rsi + 24]
  mulx r9, r8, qword ptr [rcx + 0]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 24]
  mov qword ptr [rdi + 24], r8
  mulx r11, r10, qword ptr [rcx + 8]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 32], r10
  mulx r13, rbx, qword ptr [rcx + 16]
  adox rbx, r11
  adcx rbx, r14
  mov qword ptr [rdi + 40], rbx
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 24]
  adox r14, r13
  adcx r14, rax
  mov qword ptr [rdi + 48], r14
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  mov qword ptr [rdi + 56], rax
  
  ;# Compute the raw multiplication tmp[1] <- f1[1] * f2[1]
  ;# Compute src1[0] * src2
  mov rdx, qword ptr [rsi + 32]
  mulx r9, r8, qword ptr [rcx + 32]
  xor r10d, r10d
  mov qword ptr [rdi + 64], r8
  
  mulx r11, r10, qword ptr [rcx + 40]
  adox r10, r9
  mov qword ptr [rdi + 72], r10
  
  mulx r13, rbx, qword ptr [rcx + 48]
  adox rbx, r11
  mulx rdx, r14, qword ptr [rcx + 56]
  adox r14, r13
  mov rax, 0
  adox rax, rdx
  
  
  ;# Compute src1[1] * src2
  mov rdx, qword ptr [rsi + 40]
  mulx r9, r8, qword ptr [rcx + 32]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 72]
  mov qword ptr [rdi + 72], r8
  mulx r11, r10, qword ptr [rcx + 40]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 80], r10
  mulx r13, rbx, qword ptr [rcx + 48]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 56]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[2] * src2
  mov rdx, qword ptr [rsi + 48]
  mulx r9, r8, qword ptr [rcx + 32]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 80]
  mov qword ptr [rdi + 80], r8
  mulx r11, r10, qword ptr [rcx + 40]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 88], r10
  mulx r13, rbx, qword ptr [rcx + 48]
  adox rbx, r11
  adcx rbx, r14
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 56]
  adox r14, r13
  adcx r14, rax
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  
  
  ;# Compute src1[3] * src2
  mov rdx, qword ptr [rsi + 56]
  mulx r9, r8, qword ptr [rcx + 32]
  xor r10d, r10d
  adcx r8, qword ptr [rdi + 88]
  mov qword ptr [rdi + 88], r8
  mulx r11, r10, qword ptr [rcx + 40]
  adox r10, r9
  adcx r10, rbx
  mov qword ptr [rdi + 96], r10
  mulx r13, rbx, qword ptr [rcx + 48]
  adox rbx, r11
  adcx rbx, r14
  mov qword ptr [rdi + 104], rbx
  mov r8, 0
  mulx rdx, r14, qword ptr [rcx + 56]
  adox r14, r13
  adcx r14, rax
  mov qword ptr [rdi + 112], r14
  mov rax, 0
  adox rax, rdx
  adcx rax, r8
  mov qword ptr [rdi + 120], rax
  
  
  ;# Line up pointers
  mov rsi, rdi
  mov rdi, r15
  ;# Wrap the results back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 32]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 0]
  mulx rbx, r9, qword ptr [rsi + 40]
  adcx r9, r13
  adox r9, qword ptr [rsi + 8]
  mulx r13, r10, qword ptr [rsi + 48]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 16]
  mulx rax, r11, qword ptr [rsi + 56]
  adcx r11, r13
  adox r11, qword ptr [rsi + 24]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 96]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 64]
  mulx rbx, r9, qword ptr [rsi + 104]
  adcx r9, r13
  adox r9, qword ptr [rsi + 72]
  mulx r13, r10, qword ptr [rsi + 112]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 80]
  mulx rax, r11, qword ptr [rsi + 120]
  adcx r11, r13
  adox r11, qword ptr [rsi + 88]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 40], r9
  adcx r10, rcx
  mov qword ptr [rdi + 48], r10
  adcx r11, rcx
  mov qword ptr [rdi + 56], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 32], r8
  pop rdi
  pop rsi
  pop rbx
  pop r15
  pop r14
  pop r13
  ret
fmul2_e endp
ALIGN 16
fsqr_e proc
  push r15
  push r13
  push r14
  push r12
  push rbx
  push rsi
  push rdi
  mov rdi, rcx
  mov rsi, rdx
  mov r12, r8
  ;# Compute the raw multiplication: tmp <- f * f
  ;# Step 1: Compute all partial products
  mov rdx, qword ptr [rsi + 0]
  ;# f[0]
  mulx r14, r8, qword ptr [rsi + 8]
  xor r15d, r15d
  ;# f[1]*f[0]
  mulx r10, r9, qword ptr [rsi + 16]
  adcx r9, r14
  ;# f[2]*f[0]
  mulx rcx, rax, qword ptr [rsi + 24]
  adcx r10, rax
  ;# f[3]*f[0]
  mov rdx, qword ptr [rsi + 24]
  ;# f[3]
  mulx rbx, r11, qword ptr [rsi + 8]
  adcx r11, rcx
  ;# f[1]*f[3]
  mulx r13, rax, qword ptr [rsi + 16]
  adcx rbx, rax
  ;# f[2]*f[3]
  mov rdx, qword ptr [rsi + 8]
  adcx r13, r15
  ;# f1
  mulx rcx, rax, qword ptr [rsi + 16]
  mov r14, 0
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor r15d, r15d
  adox r10, rax
  adcx r8, r8
  adox r11, rcx
  adcx r9, r9
  adox rbx, r15
  adcx r10, r10
  adox r13, r15
  adcx r11, r11
  adox r14, r15
  adcx rbx, rbx
  adcx r13, r13
  adcx r14, r14
  
  ;# Step 3: Compute intermediate squares
  mov rdx, qword ptr [rsi + 0]
  mulx rcx, rax, rdx
  ;# f[0]^2
  mov qword ptr [rdi + 0], rax
  
  add r8, rcx
  mov qword ptr [rdi + 8], r8
  
  mov rdx, qword ptr [rsi + 8]
  mulx rcx, rax, rdx
  ;# f[1]^2
  adcx r9, rax
  mov qword ptr [rdi + 16], r9
  
  adcx r10, rcx
  mov qword ptr [rdi + 24], r10
  
  mov rdx, qword ptr [rsi + 16]
  mulx rcx, rax, rdx
  ;# f[2]^2
  adcx r11, rax
  mov qword ptr [rdi + 32], r11
  
  adcx rbx, rcx
  mov qword ptr [rdi + 40], rbx
  
  mov rdx, qword ptr [rsi + 24]
  mulx rcx, rax, rdx
  ;# f[3]^2
  adcx r13, rax
  mov qword ptr [rdi + 48], r13
  
  adcx r14, rcx
  mov qword ptr [rdi + 56], r14
  
  
  ;# Line up pointers
  mov rsi, rdi
  mov rdi, r12
  ;# Wrap the result back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 32]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 0]
  mulx rbx, r9, qword ptr [rsi + 40]
  adcx r9, r13
  adox r9, qword ptr [rsi + 8]
  mulx r13, r10, qword ptr [rsi + 48]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 16]
  mulx rax, r11, qword ptr [rsi + 56]
  adcx r11, r13
  adox r11, qword ptr [rsi + 24]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  pop rdi
  pop rsi
  pop rbx
  pop r12
  pop r14
  pop r13
  pop r15
  ret
fsqr_e endp
ALIGN 16
fsqr2_e proc
  push r15
  push r13
  push r14
  push r12
  push rbx
  push rsi
  push rdi
  mov rdi, rcx
  mov rsi, rdx
  mov r12, r8
  ;# Step 1: Compute all partial products
  mov rdx, qword ptr [rsi + 0]
  ;# f[0]
  mulx r14, r8, qword ptr [rsi + 8]
  xor r15d, r15d
  ;# f[1]*f[0]
  mulx r10, r9, qword ptr [rsi + 16]
  adcx r9, r14
  ;# f[2]*f[0]
  mulx rcx, rax, qword ptr [rsi + 24]
  adcx r10, rax
  ;# f[3]*f[0]
  mov rdx, qword ptr [rsi + 24]
  ;# f[3]
  mulx rbx, r11, qword ptr [rsi + 8]
  adcx r11, rcx
  ;# f[1]*f[3]
  mulx r13, rax, qword ptr [rsi + 16]
  adcx rbx, rax
  ;# f[2]*f[3]
  mov rdx, qword ptr [rsi + 8]
  adcx r13, r15
  ;# f1
  mulx rcx, rax, qword ptr [rsi + 16]
  mov r14, 0
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor r15d, r15d
  adox r10, rax
  adcx r8, r8
  adox r11, rcx
  adcx r9, r9
  adox rbx, r15
  adcx r10, r10
  adox r13, r15
  adcx r11, r11
  adox r14, r15
  adcx rbx, rbx
  adcx r13, r13
  adcx r14, r14
  
  ;# Step 3: Compute intermediate squares
  mov rdx, qword ptr [rsi + 0]
  mulx rcx, rax, rdx
  ;# f[0]^2
  mov qword ptr [rdi + 0], rax
  
  add r8, rcx
  mov qword ptr [rdi + 8], r8
  
  mov rdx, qword ptr [rsi + 8]
  mulx rcx, rax, rdx
  ;# f[1]^2
  adcx r9, rax
  mov qword ptr [rdi + 16], r9
  
  adcx r10, rcx
  mov qword ptr [rdi + 24], r10
  
  mov rdx, qword ptr [rsi + 16]
  mulx rcx, rax, rdx
  ;# f[2]^2
  adcx r11, rax
  mov qword ptr [rdi + 32], r11
  
  adcx rbx, rcx
  mov qword ptr [rdi + 40], rbx
  
  mov rdx, qword ptr [rsi + 24]
  mulx rcx, rax, rdx
  ;# f[3]^2
  adcx r13, rax
  mov qword ptr [rdi + 48], r13
  
  adcx r14, rcx
  mov qword ptr [rdi + 56], r14
  
  
  ;# Step 1: Compute all partial products
  mov rdx, qword ptr [rsi + 32]
  ;# f[0]
  mulx r14, r8, qword ptr [rsi + 40]
  xor r15d, r15d
  ;# f[1]*f[0]
  mulx r10, r9, qword ptr [rsi + 48]
  adcx r9, r14
  ;# f[2]*f[0]
  mulx rcx, rax, qword ptr [rsi + 56]
  adcx r10, rax
  ;# f[3]*f[0]
  mov rdx, qword ptr [rsi + 56]
  ;# f[3]
  mulx rbx, r11, qword ptr [rsi + 40]
  adcx r11, rcx
  ;# f[1]*f[3]
  mulx r13, rax, qword ptr [rsi + 48]
  adcx rbx, rax
  ;# f[2]*f[3]
  mov rdx, qword ptr [rsi + 40]
  adcx r13, r15
  ;# f1
  mulx rcx, rax, qword ptr [rsi + 48]
  mov r14, 0
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor r15d, r15d
  adox r10, rax
  adcx r8, r8
  adox r11, rcx
  adcx r9, r9
  adox rbx, r15
  adcx r10, r10
  adox r13, r15
  adcx r11, r11
  adox r14, r15
  adcx rbx, rbx
  adcx r13, r13
  adcx r14, r14
  
  ;# Step 3: Compute intermediate squares
  mov rdx, qword ptr [rsi + 32]
  mulx rcx, rax, rdx
  ;# f[0]^2
  mov qword ptr [rdi + 64], rax
  
  add r8, rcx
  mov qword ptr [rdi + 72], r8
  
  mov rdx, qword ptr [rsi + 40]
  mulx rcx, rax, rdx
  ;# f[1]^2
  adcx r9, rax
  mov qword ptr [rdi + 80], r9
  
  adcx r10, rcx
  mov qword ptr [rdi + 88], r10
  
  mov rdx, qword ptr [rsi + 48]
  mulx rcx, rax, rdx
  ;# f[2]^2
  adcx r11, rax
  mov qword ptr [rdi + 96], r11
  
  adcx rbx, rcx
  mov qword ptr [rdi + 104], rbx
  
  mov rdx, qword ptr [rsi + 56]
  mulx rcx, rax, rdx
  ;# f[3]^2
  adcx r13, rax
  mov qword ptr [rdi + 112], r13
  
  adcx r14, rcx
  mov qword ptr [rdi + 120], r14
  
  
  ;# Line up pointers
  mov rsi, rdi
  mov rdi, r12
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 32]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 0]
  mulx rbx, r9, qword ptr [rsi + 40]
  adcx r9, r13
  adox r9, qword ptr [rsi + 8]
  mulx r13, r10, qword ptr [rsi + 48]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 16]
  mulx rax, r11, qword ptr [rsi + 56]
  adcx r11, r13
  adox r11, qword ptr [rsi + 24]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 8], r9
  adcx r10, rcx
  mov qword ptr [rdi + 16], r10
  adcx r11, rcx
  mov qword ptr [rdi + 24], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 0], r8
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov rdx, 38
  mulx r13, r8, qword ptr [rsi + 96]
  xor ecx, ecx
  adox r8, qword ptr [rsi + 64]
  mulx rbx, r9, qword ptr [rsi + 104]
  adcx r9, r13
  adox r9, qword ptr [rsi + 72]
  mulx r13, r10, qword ptr [rsi + 112]
  adcx r10, rbx
  adox r10, qword ptr [rsi + 80]
  mulx rax, r11, qword ptr [rsi + 120]
  adcx r11, r13
  adox r11, qword ptr [rsi + 88]
  adcx rax, rcx
  adox rax, rcx
  imul rax, rdx
  
  ;# Step 2: Fold the carry back into dst
  add r8, rax
  adcx r9, rcx
  mov qword ptr [rdi + 40], r9
  adcx r10, rcx
  mov qword ptr [rdi + 48], r10
  adcx r11, rcx
  mov qword ptr [rdi + 56], r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov rax, 0
  cmovc rax, rdx
  add r8, rax
  mov qword ptr [rdi + 32], r8
  pop rdi
  pop rsi
  pop rbx
  pop r12
  pop r14
  pop r13
  pop r15
  ret
fsqr2_e endp
ALIGN 16
cswap2_e proc
  push rdi
  push rsi
  mov rdi, rcx
  mov rsi, rdx
  mov rdx, r8
  ;# Transfer bit into CF flag
  add rdi, 18446744073709551615
  
  ;# cswap p1[0], p2[0]
  mov r8, qword ptr [rsi + 0]
  mov r9, qword ptr [rdx + 0]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 0], r8
  mov qword ptr [rdx + 0], r9
  
  ;# cswap p1[1], p2[1]
  mov r8, qword ptr [rsi + 8]
  mov r9, qword ptr [rdx + 8]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 8], r8
  mov qword ptr [rdx + 8], r9
  
  ;# cswap p1[2], p2[2]
  mov r8, qword ptr [rsi + 16]
  mov r9, qword ptr [rdx + 16]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 16], r8
  mov qword ptr [rdx + 16], r9
  
  ;# cswap p1[3], p2[3]
  mov r8, qword ptr [rsi + 24]
  mov r9, qword ptr [rdx + 24]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 24], r8
  mov qword ptr [rdx + 24], r9
  
  ;# cswap p1[4], p2[4]
  mov r8, qword ptr [rsi + 32]
  mov r9, qword ptr [rdx + 32]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 32], r8
  mov qword ptr [rdx + 32], r9
  
  ;# cswap p1[5], p2[5]
  mov r8, qword ptr [rsi + 40]
  mov r9, qword ptr [rdx + 40]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 40], r8
  mov qword ptr [rdx + 40], r9
  
  ;# cswap p1[6], p2[6]
  mov r8, qword ptr [rsi + 48]
  mov r9, qword ptr [rdx + 48]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 48], r8
  mov qword ptr [rdx + 48], r9
  
  ;# cswap p1[7], p2[7]
  mov r8, qword ptr [rsi + 56]
  mov r9, qword ptr [rdx + 56]
  mov r10, r8
  cmovc r8, r9
  cmovc r9, r10
  mov qword ptr [rsi + 56], r8
  mov qword ptr [rdx + 56], r9
  pop rsi
  pop rdi
  ret
cswap2_e endp
end
