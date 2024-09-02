.text
.global add_scalar_e
add_scalar_e:
  push %rdi
  push %rsi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %rdx
  ;# Clear registers to propagate the carry bit
  xor %r8d, %r8d
  xor %r9d, %r9d
  xor %r10d, %r10d
  xor %r11d, %r11d
  xor %eax, %eax
  
  ;# Begin addition chain
  addq 0(%rsi), %rdx
  movq %rdx, 0(%rdi)
  adcxq 8(%rsi), %r8
  movq %r8, 8(%rdi)
  adcxq 16(%rsi), %r9
  movq %r9, 16(%rdi)
  adcxq 24(%rsi), %r10
  movq %r10, 24(%rdi)
  
  ;# Return the carry bit in a register
  adcx %r11, %rax
  pop %rsi
  pop %rdi
  ret

.global fadd_e
fadd_e:
  push %rdi
  push %rsi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %rdx
  ;# Compute the raw addition of f1 + f2
  movq 0(%rdx), %r8
  addq 0(%rsi), %r8
  movq 8(%rdx), %r9
  adcxq 8(%rsi), %r9
  movq 16(%rdx), %r10
  adcxq 16(%rsi), %r10
  movq 24(%rdx), %r11
  adcxq 24(%rsi), %r11
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov $0, %rax
  mov $38, %rdx
  cmovc %rdx, %rax
  
  ;# Step 2: Add carry*38 to the original sum
  xor %ecx, %ecx
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  pop %rsi
  pop %rdi
  ret

.global fsub_e
fsub_e:
  push %rdi
  push %rsi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %rdx
  ;# Compute the raw substraction of f1-f2
  movq 0(%rsi), %r8
  subq 0(%rdx), %r8
  movq 8(%rsi), %r9
  sbbq 8(%rdx), %r9
  movq 16(%rsi), %r10
  sbbq 16(%rdx), %r10
  movq 24(%rsi), %r11
  sbbq 24(%rdx), %r11
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov $0, %rax
  mov $38, %rcx
  cmovc %rcx, %rax
  
  ;# Step 2: Substract carry*38 from the original difference
  sub %rax, %r8
  sbb $0, %r9
  sbb $0, %r10
  sbb $0, %r11
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rcx, %rax
  sub %rax, %r8
  
  ;# Store the result
  movq %r8, 0(%rdi)
  movq %r9, 8(%rdi)
  movq %r10, 16(%rdi)
  movq %r11, 24(%rdi)
  pop %rsi
  pop %rdi
  ret

.global fmul_scalar_e
fmul_scalar_e:
  push %rdi
  push %r13
  push %rbx
  push %rsi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %rdx
  ;# Compute the raw multiplication of f1*f2
  mulxq 0(%rsi), %r8, %rcx
  ;# f1[0]*f2
  mulxq 8(%rsi), %r9, %rbx
  ;# f1[1]*f2
  add %rcx, %r9
  mov $0, %rcx
  mulxq 16(%rsi), %r10, %r13
  ;# f1[2]*f2
  adcx %rbx, %r10
  mulxq 24(%rsi), %r11, %rax
  ;# f1[3]*f2
  adcx %r13, %r11
  adcx %rcx, %rax
  ;# Wrap the result back into the field
  ;# Step 1: Compute carry*38
  mov $38, %rdx
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  pop %rsi
  pop %rbx
  pop %r13
  pop %rdi
  ret

.global fmul_e
fmul_e:
  push %r13
  push %r14
  push %r15
  push %rbx
  push %rsi
  push %rdi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %r15
  mov %r9, %rcx
  ;# Compute the raw multiplication: tmp <- src1 * src2
  ;# Compute src1[0] * src2
  movq 0(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  movq %r8, 0(%rdi)
  
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  movq %r10, 8(%rdi)
  
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  mov $0, %rax
  adox %rdx, %rax
  
  
  ;# Compute src1[1] * src2
  movq 8(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 8(%rdi), %r8
  movq %r8, 8(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 16(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[2] * src2
  movq 16(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 16(%rdi), %r8
  movq %r8, 16(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 24(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[3] * src2
  movq 24(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 24(%rdi), %r8
  movq %r8, 24(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 32(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  movq %rbx, 40(%rdi)
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  movq %r14, 48(%rdi)
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  movq %rax, 56(%rdi)
  
  
  ;# Line up pointers
  mov %rdi, %rsi
  mov %r15, %rdi
  ;# Wrap the result back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 32(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 0(%rsi), %r8
  mulxq 40(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 8(%rsi), %r9
  mulxq 48(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 16(%rsi), %r10
  mulxq 56(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 24(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  pop %rdi
  pop %rsi
  pop %rbx
  pop %r15
  pop %r14
  pop %r13
  ret

.global fmul2_e
fmul2_e:
  push %r13
  push %r14
  push %r15
  push %rbx
  push %rsi
  push %rdi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %r15
  mov %r9, %rcx
  ;# Compute the raw multiplication tmp[0] <- f1[0] * f2[0]
  ;# Compute src1[0] * src2
  movq 0(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  movq %r8, 0(%rdi)
  
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  movq %r10, 8(%rdi)
  
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  mov $0, %rax
  adox %rdx, %rax
  
  
  ;# Compute src1[1] * src2
  movq 8(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 8(%rdi), %r8
  movq %r8, 8(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 16(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[2] * src2
  movq 16(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 16(%rdi), %r8
  movq %r8, 16(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 24(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[3] * src2
  movq 24(%rsi), %rdx
  mulxq 0(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 24(%rdi), %r8
  movq %r8, 24(%rdi)
  mulxq 8(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 32(%rdi)
  mulxq 16(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  movq %rbx, 40(%rdi)
  mov $0, %r8
  mulxq 24(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  movq %r14, 48(%rdi)
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  movq %rax, 56(%rdi)
  
  ;# Compute the raw multiplication tmp[1] <- f1[1] * f2[1]
  ;# Compute src1[0] * src2
  movq 32(%rsi), %rdx
  mulxq 32(%rcx), %r8, %r9
  xor %r10d, %r10d
  movq %r8, 64(%rdi)
  
  mulxq 40(%rcx), %r10, %r11
  adox %r9, %r10
  movq %r10, 72(%rdi)
  
  mulxq 48(%rcx), %rbx, %r13
  adox %r11, %rbx
  mulxq 56(%rcx), %r14, %rdx
  adox %r13, %r14
  mov $0, %rax
  adox %rdx, %rax
  
  
  ;# Compute src1[1] * src2
  movq 40(%rsi), %rdx
  mulxq 32(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 72(%rdi), %r8
  movq %r8, 72(%rdi)
  mulxq 40(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 80(%rdi)
  mulxq 48(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 56(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[2] * src2
  movq 48(%rsi), %rdx
  mulxq 32(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 80(%rdi), %r8
  movq %r8, 80(%rdi)
  mulxq 40(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 88(%rdi)
  mulxq 48(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  mov $0, %r8
  mulxq 56(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  
  
  ;# Compute src1[3] * src2
  movq 56(%rsi), %rdx
  mulxq 32(%rcx), %r8, %r9
  xor %r10d, %r10d
  adcxq 88(%rdi), %r8
  movq %r8, 88(%rdi)
  mulxq 40(%rcx), %r10, %r11
  adox %r9, %r10
  adcx %rbx, %r10
  movq %r10, 96(%rdi)
  mulxq 48(%rcx), %rbx, %r13
  adox %r11, %rbx
  adcx %r14, %rbx
  movq %rbx, 104(%rdi)
  mov $0, %r8
  mulxq 56(%rcx), %r14, %rdx
  adox %r13, %r14
  adcx %rax, %r14
  movq %r14, 112(%rdi)
  mov $0, %rax
  adox %rdx, %rax
  adcx %r8, %rax
  movq %rax, 120(%rdi)
  
  
  ;# Line up pointers
  mov %rdi, %rsi
  mov %r15, %rdi
  ;# Wrap the results back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 32(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 0(%rsi), %r8
  mulxq 40(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 8(%rsi), %r9
  mulxq 48(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 16(%rsi), %r10
  mulxq 56(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 24(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 96(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 64(%rsi), %r8
  mulxq 104(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 72(%rsi), %r9
  mulxq 112(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 80(%rsi), %r10
  mulxq 120(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 88(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 40(%rdi)
  adcx %rcx, %r10
  movq %r10, 48(%rdi)
  adcx %rcx, %r11
  movq %r11, 56(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 32(%rdi)
  pop %rdi
  pop %rsi
  pop %rbx
  pop %r15
  pop %r14
  pop %r13
  ret

.global fsqr_e
fsqr_e:
  push %r15
  push %r13
  push %r14
  push %r12
  push %rbx
  push %rsi
  push %rdi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %r12
  ;# Compute the raw multiplication: tmp <- f * f
  ;# Step 1: Compute all partial products
  movq 0(%rsi), %rdx
  ;# f[0]
  mulxq 8(%rsi), %r8, %r14
  xor %r15d, %r15d
  ;# f[1]*f[0]
  mulxq 16(%rsi), %r9, %r10
  adcx %r14, %r9
  ;# f[2]*f[0]
  mulxq 24(%rsi), %rax, %rcx
  adcx %rax, %r10
  ;# f[3]*f[0]
  movq 24(%rsi), %rdx
  ;# f[3]
  mulxq 8(%rsi), %r11, %rbx
  adcx %rcx, %r11
  ;# f[1]*f[3]
  mulxq 16(%rsi), %rax, %r13
  adcx %rax, %rbx
  ;# f[2]*f[3]
  movq 8(%rsi), %rdx
  adcx %r15, %r13
  ;# f1
  mulxq 16(%rsi), %rax, %rcx
  mov $0, %r14
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor %r15d, %r15d
  adox %rax, %r10
  adcx %r8, %r8
  adox %rcx, %r11
  adcx %r9, %r9
  adox %r15, %rbx
  adcx %r10, %r10
  adox %r15, %r13
  adcx %r11, %r11
  adox %r15, %r14
  adcx %rbx, %rbx
  adcx %r13, %r13
  adcx %r14, %r14
  
  ;# Step 3: Compute intermediate squares
  movq 0(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[0]^2
  movq %rax, 0(%rdi)
  
  add %rcx, %r8
  movq %r8, 8(%rdi)
  
  movq 8(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[1]^2
  adcx %rax, %r9
  movq %r9, 16(%rdi)
  
  adcx %rcx, %r10
  movq %r10, 24(%rdi)
  
  movq 16(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[2]^2
  adcx %rax, %r11
  movq %r11, 32(%rdi)
  
  adcx %rcx, %rbx
  movq %rbx, 40(%rdi)
  
  movq 24(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[3]^2
  adcx %rax, %r13
  movq %r13, 48(%rdi)
  
  adcx %rcx, %r14
  movq %r14, 56(%rdi)
  
  
  ;# Line up pointers
  mov %rdi, %rsi
  mov %r12, %rdi
  ;# Wrap the result back into the field
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 32(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 0(%rsi), %r8
  mulxq 40(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 8(%rsi), %r9
  mulxq 48(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 16(%rsi), %r10
  mulxq 56(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 24(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  pop %rdi
  pop %rsi
  pop %rbx
  pop %r12
  pop %r14
  pop %r13
  pop %r15
  ret

.global fsqr2_e
fsqr2_e:
  push %r15
  push %r13
  push %r14
  push %r12
  push %rbx
  push %rsi
  push %rdi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %r12
  ;# Step 1: Compute all partial products
  movq 0(%rsi), %rdx
  ;# f[0]
  mulxq 8(%rsi), %r8, %r14
  xor %r15d, %r15d
  ;# f[1]*f[0]
  mulxq 16(%rsi), %r9, %r10
  adcx %r14, %r9
  ;# f[2]*f[0]
  mulxq 24(%rsi), %rax, %rcx
  adcx %rax, %r10
  ;# f[3]*f[0]
  movq 24(%rsi), %rdx
  ;# f[3]
  mulxq 8(%rsi), %r11, %rbx
  adcx %rcx, %r11
  ;# f[1]*f[3]
  mulxq 16(%rsi), %rax, %r13
  adcx %rax, %rbx
  ;# f[2]*f[3]
  movq 8(%rsi), %rdx
  adcx %r15, %r13
  ;# f1
  mulxq 16(%rsi), %rax, %rcx
  mov $0, %r14
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor %r15d, %r15d
  adox %rax, %r10
  adcx %r8, %r8
  adox %rcx, %r11
  adcx %r9, %r9
  adox %r15, %rbx
  adcx %r10, %r10
  adox %r15, %r13
  adcx %r11, %r11
  adox %r15, %r14
  adcx %rbx, %rbx
  adcx %r13, %r13
  adcx %r14, %r14
  
  ;# Step 3: Compute intermediate squares
  movq 0(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[0]^2
  movq %rax, 0(%rdi)
  
  add %rcx, %r8
  movq %r8, 8(%rdi)
  
  movq 8(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[1]^2
  adcx %rax, %r9
  movq %r9, 16(%rdi)
  
  adcx %rcx, %r10
  movq %r10, 24(%rdi)
  
  movq 16(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[2]^2
  adcx %rax, %r11
  movq %r11, 32(%rdi)
  
  adcx %rcx, %rbx
  movq %rbx, 40(%rdi)
  
  movq 24(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[3]^2
  adcx %rax, %r13
  movq %r13, 48(%rdi)
  
  adcx %rcx, %r14
  movq %r14, 56(%rdi)
  
  
  ;# Step 1: Compute all partial products
  movq 32(%rsi), %rdx
  ;# f[0]
  mulxq 40(%rsi), %r8, %r14
  xor %r15d, %r15d
  ;# f[1]*f[0]
  mulxq 48(%rsi), %r9, %r10
  adcx %r14, %r9
  ;# f[2]*f[0]
  mulxq 56(%rsi), %rax, %rcx
  adcx %rax, %r10
  ;# f[3]*f[0]
  movq 56(%rsi), %rdx
  ;# f[3]
  mulxq 40(%rsi), %r11, %rbx
  adcx %rcx, %r11
  ;# f[1]*f[3]
  mulxq 48(%rsi), %rax, %r13
  adcx %rax, %rbx
  ;# f[2]*f[3]
  movq 40(%rsi), %rdx
  adcx %r15, %r13
  ;# f1
  mulxq 48(%rsi), %rax, %rcx
  mov $0, %r14
  ;# f[2]*f[1]
  
  ;# Step 2: Compute two parallel carry chains
  xor %r15d, %r15d
  adox %rax, %r10
  adcx %r8, %r8
  adox %rcx, %r11
  adcx %r9, %r9
  adox %r15, %rbx
  adcx %r10, %r10
  adox %r15, %r13
  adcx %r11, %r11
  adox %r15, %r14
  adcx %rbx, %rbx
  adcx %r13, %r13
  adcx %r14, %r14
  
  ;# Step 3: Compute intermediate squares
  movq 32(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[0]^2
  movq %rax, 64(%rdi)
  
  add %rcx, %r8
  movq %r8, 72(%rdi)
  
  movq 40(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[1]^2
  adcx %rax, %r9
  movq %r9, 80(%rdi)
  
  adcx %rcx, %r10
  movq %r10, 88(%rdi)
  
  movq 48(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[2]^2
  adcx %rax, %r11
  movq %r11, 96(%rdi)
  
  adcx %rcx, %rbx
  movq %rbx, 104(%rdi)
  
  movq 56(%rsi), %rdx
  mulx %rdx, %rax, %rcx
  ;# f[3]^2
  adcx %rax, %r13
  movq %r13, 112(%rdi)
  
  adcx %rcx, %r14
  movq %r14, 120(%rdi)
  
  
  ;# Line up pointers
  mov %rdi, %rsi
  mov %r12, %rdi
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 32(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 0(%rsi), %r8
  mulxq 40(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 8(%rsi), %r9
  mulxq 48(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 16(%rsi), %r10
  mulxq 56(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 24(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 8(%rdi)
  adcx %rcx, %r10
  movq %r10, 16(%rdi)
  adcx %rcx, %r11
  movq %r11, 24(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 0(%rdi)
  
  ;# Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
  mov $38, %rdx
  mulxq 96(%rsi), %r8, %r13
  xor %ecx, %ecx
  adoxq 64(%rsi), %r8
  mulxq 104(%rsi), %r9, %rbx
  adcx %r13, %r9
  adoxq 72(%rsi), %r9
  mulxq 112(%rsi), %r10, %r13
  adcx %rbx, %r10
  adoxq 80(%rsi), %r10
  mulxq 120(%rsi), %r11, %rax
  adcx %r13, %r11
  adoxq 88(%rsi), %r11
  adcx %rcx, %rax
  adox %rcx, %rax
  imul %rdx, %rax
  
  ;# Step 2: Fold the carry back into dst
  add %rax, %r8
  adcx %rcx, %r9
  movq %r9, 40(%rdi)
  adcx %rcx, %r10
  movq %r10, 48(%rdi)
  adcx %rcx, %r11
  movq %r11, 56(%rdi)
  
  ;# Step 3: Fold the carry bit back in; guaranteed not to carry at this point
  mov $0, %rax
  cmovc %rdx, %rax
  add %rax, %r8
  movq %r8, 32(%rdi)
  pop %rdi
  pop %rsi
  pop %rbx
  pop %r12
  pop %r14
  pop %r13
  pop %r15
  ret

.global cswap2_e
cswap2_e:
  push %rdi
  push %rsi
  mov %rcx, %rdi
  mov %rdx, %rsi
  mov %r8, %rdx
  ;# Transfer bit into CF flag
  add $18446744073709551615, %rdi
  
  ;# cswap p1[0], p2[0]
  movq 0(%rsi), %r8
  movq 0(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 0(%rsi)
  movq %r9, 0(%rdx)
  
  ;# cswap p1[1], p2[1]
  movq 8(%rsi), %r8
  movq 8(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 8(%rsi)
  movq %r9, 8(%rdx)
  
  ;# cswap p1[2], p2[2]
  movq 16(%rsi), %r8
  movq 16(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 16(%rsi)
  movq %r9, 16(%rdx)
  
  ;# cswap p1[3], p2[3]
  movq 24(%rsi), %r8
  movq 24(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 24(%rsi)
  movq %r9, 24(%rdx)
  
  ;# cswap p1[4], p2[4]
  movq 32(%rsi), %r8
  movq 32(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 32(%rsi)
  movq %r9, 32(%rdx)
  
  ;# cswap p1[5], p2[5]
  movq 40(%rsi), %r8
  movq 40(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 40(%rsi)
  movq %r9, 40(%rdx)
  
  ;# cswap p1[6], p2[6]
  movq 48(%rsi), %r8
  movq 48(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 48(%rsi)
  movq %r9, 48(%rdx)
  
  ;# cswap p1[7], p2[7]
  movq 56(%rsi), %r8
  movq 56(%rdx), %r9
  mov %r8, %r10
  cmovc %r9, %r8
  cmovc %r10, %r9
  movq %r8, 56(%rsi)
  movq %r9, 56(%rdx)
  pop %rsi
  pop %rdi
  ret


