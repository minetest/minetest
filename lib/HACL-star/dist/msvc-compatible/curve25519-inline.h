#ifdef __GNUC__
#if defined(__x86_64__) || defined(_M_X64)
#pragma once
#include <inttypes.h>

// Computes the addition of four-element f1 with value in f2
// and returns the carry (if any)
static inline void add_scalar (uint64_t *out, uint64_t *f1, uint64_t f2) 
{
  __asm__ volatile(
    // Clear registers to propagate the carry bit
    "  xor %%r8d, %%r8d;"
    "  xor %%r9d, %%r9d;"
    "  xor %%r10d, %%r10d;"
    "  xor %%r11d, %%r11d;"
    "  xor %%eax, %%eax;"

    // Begin addition chain
    "  addq 0(%2), %0;"
    "  movq %0, 0(%1);"
    "  adcxq 8(%2), %%r8;"
    "  movq %%r8, 8(%1);"
    "  adcxq 16(%2), %%r9;"
    "  movq %%r9, 16(%1);"
    "  adcxq 24(%2), %%r10;"
    "  movq %%r10, 24(%1);"

    // Return the carry bit in a register
    "  adcx %%r11, %%rax;"
  : "+&r" (f2)
  : "r" (out), "r" (f1)
  : "%rax", "%r8", "%r9", "%r10", "%r11", "memory", "cc"
  );
}

// Computes the field addition of two field elements
static inline void fadd (uint64_t *out, uint64_t *f1, uint64_t *f2) 
{
  __asm__ volatile(
    // Compute the raw addition of f1 + f2
    "  movq 0(%0), %%r8;"
    "  addq 0(%2), %%r8;"
    "  movq 8(%0), %%r9;"
    "  adcxq 8(%2), %%r9;"
    "  movq 16(%0), %%r10;"
    "  adcxq 16(%2), %%r10;"
    "  movq 24(%0), %%r11;"
    "  adcxq 24(%2), %%r11;"

    /////// Wrap the result back into the field ////// 

    // Step 1: Compute carry*38
    "  mov $0, %%rax;"
    "  mov $38, %0;"
    "  cmovc %0, %%rax;"

    // Step 2: Add carry*38 to the original sum
    "  xor %%ecx, %%ecx;"
    "  add %%rax, %%r8;"
    "  adcx %%rcx, %%r9;"
    "  movq %%r9, 8(%1);"
    "  adcx %%rcx, %%r10;"
    "  movq %%r10, 16(%1);"
    "  adcx %%rcx, %%r11;"
    "  movq %%r11, 24(%1);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %0, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%1);"
  : "+&r" (f2)
  : "r" (out), "r" (f1)
  : "%rax", "%rcx", "%r8", "%r9", "%r10", "%r11", "memory", "cc"
  );
}

// Computes the field substraction of two field elements
static inline void fsub (uint64_t *out, uint64_t *f1, uint64_t *f2) 
{
  __asm__ volatile(
    // Compute the raw substraction of f1-f2
    "  movq 0(%1), %%r8;"
    "  subq 0(%2), %%r8;"
    "  movq 8(%1), %%r9;"
    "  sbbq 8(%2), %%r9;"
    "  movq 16(%1), %%r10;"
    "  sbbq 16(%2), %%r10;"
    "  movq 24(%1), %%r11;"
    "  sbbq 24(%2), %%r11;"

    /////// Wrap the result back into the field ////// 

    // Step 1: Compute carry*38
    "  mov $0, %%rax;"
    "  mov $38, %%rcx;"
    "  cmovc %%rcx, %%rax;"

    // Step 2: Substract carry*38 from the original difference
    "  sub %%rax, %%r8;"
    "  sbb $0, %%r9;"
    "  sbb $0, %%r10;"
    "  sbb $0, %%r11;"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rcx, %%rax;"
    "  sub %%rax, %%r8;"

    // Store the result
    "  movq %%r8, 0(%0);"
    "  movq %%r9, 8(%0);"
    "  movq %%r10, 16(%0);"
    "  movq %%r11, 24(%0);"
  : 
  : "r" (out), "r" (f1), "r" (f2)
  : "%rax", "%rcx", "%r8", "%r9", "%r10", "%r11", "memory", "cc"
  );
}

// Computes a field multiplication: out <- f1 * f2
// Uses the 8-element buffer tmp for intermediate results
static inline void fmul (uint64_t *out, uint64_t *f1, uint64_t *f2, uint64_t *tmp) 
{
  __asm__ volatile(

    /////// Compute the raw multiplication: tmp <- src1 * src2 ////// 

    // Compute src1[0] * src2
    "  movq 0(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  movq %%r8, 0(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  movq %%r10, 8(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"

    // Compute src1[1] * src2
    "  movq 8(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  adcxq 8(%2), %%r8;"    "  movq %%r8, 8(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 16(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[2] * src2
    "  movq 16(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 16(%2), %%r8;"    "  movq %%r8, 16(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 24(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[3] * src2
    "  movq 24(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 24(%2), %%r8;"    "  movq %%r8, 24(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 32(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  movq %%rbx, 40(%2);"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  movq %%r14, 48(%2);"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"     "  movq %%rax, 56(%2);"

    // Line up pointers
    "  mov %2, %0;"
    "  mov %3, %2;"

    /////// Wrap the result back into the field ////// 

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 32(%0), %%r8, %%r13;"
    "  xor %k1, %k1;"
    "  adoxq 0(%0), %%r8;"
    "  mulxq 40(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 8(%0), %%r9;"
    "  mulxq 48(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 16(%0), %%r10;"
    "  mulxq 56(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 24(%0), %%r11;"
    "  adcx %1, %%rax;"
    "  adox %1, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %1, %%r9;"
    "  movq %%r9, 8(%2);"
    "  adcx %1, %%r10;"
    "  movq %%r10, 16(%2);"
    "  adcx %1, %%r11;"
    "  movq %%r11, 24(%2);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%2);"
  : "+&r" (f1), "+&r" (f2), "+&r" (tmp)
  : "r" (out)
  : "%rax", "%rbx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r13", "%r14", "memory", "cc"
  );
}

// Computes two field multiplications:
//   out[0] <- f1[0] * f2[0]
//   out[1] <- f1[1] * f2[1]
// Uses the 16-element buffer tmp for intermediate results:
static inline void fmul2 (uint64_t *out, uint64_t *f1, uint64_t *f2, uint64_t *tmp) 
{
  __asm__ volatile(

    /////// Compute the raw multiplication tmp[0] <- f1[0] * f2[0] ////// 

    // Compute src1[0] * src2
    "  movq 0(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  movq %%r8, 0(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  movq %%r10, 8(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"

    // Compute src1[1] * src2
    "  movq 8(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  adcxq 8(%2), %%r8;"    "  movq %%r8, 8(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 16(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[2] * src2
    "  movq 16(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 16(%2), %%r8;"    "  movq %%r8, 16(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 24(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[3] * src2
    "  movq 24(%0), %%rdx;"
    "  mulxq 0(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 24(%2), %%r8;"    "  movq %%r8, 24(%2);"
    "  mulxq 8(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 32(%2);"
    "  mulxq 16(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  movq %%rbx, 40(%2);"    "  mov $0, %%r8;"
    "  mulxq 24(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  movq %%r14, 48(%2);"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"     "  movq %%rax, 56(%2);"

    /////// Compute the raw multiplication tmp[1] <- f1[1] * f2[1] ////// 

    // Compute src1[0] * src2
    "  movq 32(%0), %%rdx;"
    "  mulxq 32(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  movq %%r8, 64(%2);"
    "  mulxq 40(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  movq %%r10, 72(%2);"
    "  mulxq 48(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"
    "  mulxq 56(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"

    // Compute src1[1] * src2
    "  movq 40(%0), %%rdx;"
    "  mulxq 32(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"     "  adcxq 72(%2), %%r8;"    "  movq %%r8, 72(%2);"
    "  mulxq 40(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 80(%2);"
    "  mulxq 48(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 56(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[2] * src2
    "  movq 48(%0), %%rdx;"
    "  mulxq 32(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 80(%2), %%r8;"    "  movq %%r8, 80(%2);"
    "  mulxq 40(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 88(%2);"
    "  mulxq 48(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  mov $0, %%r8;"
    "  mulxq 56(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"


    // Compute src1[3] * src2
    "  movq 56(%0), %%rdx;"
    "  mulxq 32(%1), %%r8, %%r9;"       "  xor %%r10d, %%r10d;"    "  adcxq 88(%2), %%r8;"    "  movq %%r8, 88(%2);"
    "  mulxq 40(%1), %%r10, %%r11;"     "  adox %%r9, %%r10;"     "  adcx %%rbx, %%r10;"    "  movq %%r10, 96(%2);"
    "  mulxq 48(%1), %%rbx, %%r13;"    "  adox %%r11, %%rbx;"    "  adcx %%r14, %%rbx;"    "  movq %%rbx, 104(%2);"    "  mov $0, %%r8;"
    "  mulxq 56(%1), %%r14, %%rdx;"    "  adox %%r13, %%r14;"    "  adcx %%rax, %%r14;"    "  movq %%r14, 112(%2);"    "  mov $0, %%rax;"
                                       "  adox %%rdx, %%rax;"    "  adcx %%r8, %%rax;"     "  movq %%rax, 120(%2);"

    // Line up pointers
    "  mov %2, %0;"
    "  mov %3, %2;"

    /////// Wrap the results back into the field ////// 

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 32(%0), %%r8, %%r13;"
    "  xor %k1, %k1;"
    "  adoxq 0(%0), %%r8;"
    "  mulxq 40(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 8(%0), %%r9;"
    "  mulxq 48(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 16(%0), %%r10;"
    "  mulxq 56(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 24(%0), %%r11;"
    "  adcx %1, %%rax;"
    "  adox %1, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %1, %%r9;"
    "  movq %%r9, 8(%2);"
    "  adcx %1, %%r10;"
    "  movq %%r10, 16(%2);"
    "  adcx %1, %%r11;"
    "  movq %%r11, 24(%2);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%2);"

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 96(%0), %%r8, %%r13;"
    "  xor %k1, %k1;"
    "  adoxq 64(%0), %%r8;"
    "  mulxq 104(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 72(%0), %%r9;"
    "  mulxq 112(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 80(%0), %%r10;"
    "  mulxq 120(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 88(%0), %%r11;"
    "  adcx %1, %%rax;"
    "  adox %1, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %1, %%r9;"
    "  movq %%r9, 40(%2);"
    "  adcx %1, %%r10;"
    "  movq %%r10, 48(%2);"
    "  adcx %1, %%r11;"
    "  movq %%r11, 56(%2);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 32(%2);"
  : "+&r" (f1), "+&r" (f2), "+&r" (tmp)
  : "r" (out)
  : "%rax", "%rbx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r13", "%r14", "memory", "cc"
  );
}

// Computes the field multiplication of four-element f1 with value in f2
// Requires f2 to be smaller than 2^17
static inline void fmul_scalar (uint64_t *out, uint64_t *f1, uint64_t f2) 
{
  register uint64_t f2_r __asm__("rdx") = f2;

  __asm__ volatile(
    // Compute the raw multiplication of f1*f2
    "  mulxq 0(%2), %%r8, %%rcx;"      // f1[0]*f2
    "  mulxq 8(%2), %%r9, %%rbx;"      // f1[1]*f2
    "  add %%rcx, %%r9;"
    "  mov $0, %%rcx;"
    "  mulxq 16(%2), %%r10, %%r13;"    // f1[2]*f2
    "  adcx %%rbx, %%r10;"
    "  mulxq 24(%2), %%r11, %%rax;"    // f1[3]*f2
    "  adcx %%r13, %%r11;"
    "  adcx %%rcx, %%rax;"

    /////// Wrap the result back into the field ////// 

    // Step 1: Compute carry*38
    "  mov $38, %%rdx;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %%rcx, %%r9;"
    "  movq %%r9, 8(%1);"
    "  adcx %%rcx, %%r10;"
    "  movq %%r10, 16(%1);"
    "  adcx %%rcx, %%r11;"
    "  movq %%r11, 24(%1);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%1);"
  : "+&r" (f2_r)
  : "r" (out), "r" (f1)
  : "%rax", "%rbx", "%rcx", "%r8", "%r9", "%r10", "%r11", "%r13", "memory", "cc"
  );
}

// Computes p1 <- bit ? p2 : p1 in constant time
static inline void cswap2 (uint64_t bit, uint64_t *p1, uint64_t *p2) 
{
  __asm__ volatile(
    // Transfer bit into CF flag
    "  add $18446744073709551615, %0;"

    // cswap p1[0], p2[0]
    "  movq 0(%1), %%r8;"
    "  movq 0(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 0(%1);"
    "  movq %%r9, 0(%2);"

    // cswap p1[1], p2[1]
    "  movq 8(%1), %%r8;"
    "  movq 8(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 8(%1);"
    "  movq %%r9, 8(%2);"

    // cswap p1[2], p2[2]
    "  movq 16(%1), %%r8;"
    "  movq 16(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 16(%1);"
    "  movq %%r9, 16(%2);"

    // cswap p1[3], p2[3]
    "  movq 24(%1), %%r8;"
    "  movq 24(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 24(%1);"
    "  movq %%r9, 24(%2);"

    // cswap p1[4], p2[4]
    "  movq 32(%1), %%r8;"
    "  movq 32(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 32(%1);"
    "  movq %%r9, 32(%2);"

    // cswap p1[5], p2[5]
    "  movq 40(%1), %%r8;"
    "  movq 40(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 40(%1);"
    "  movq %%r9, 40(%2);"

    // cswap p1[6], p2[6]
    "  movq 48(%1), %%r8;"
    "  movq 48(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 48(%1);"
    "  movq %%r9, 48(%2);"

    // cswap p1[7], p2[7]
    "  movq 56(%1), %%r8;"
    "  movq 56(%2), %%r9;"
    "  mov %%r8, %%r10;"
    "  cmovc %%r9, %%r8;"
    "  cmovc %%r10, %%r9;"
    "  movq %%r8, 56(%1);"
    "  movq %%r9, 56(%2);"
  : "+&r" (bit)
  : "r" (p1), "r" (p2)
  : "%r8", "%r9", "%r10", "memory", "cc"
  );
}

// Computes the square of a field element: out <- f * f
// Uses the 8-element buffer tmp for intermediate results
static inline void fsqr (uint64_t *out, uint64_t *f, uint64_t *tmp) 
{
  __asm__ volatile(

    /////// Compute the raw multiplication: tmp <- f * f ////// 

    // Step 1: Compute all partial products
    "  movq 0(%0), %%rdx;"                                       // f[0]
    "  mulxq 8(%0), %%r8, %%r14;"      "  xor %%r15d, %%r15d;"     // f[1]*f[0]
    "  mulxq 16(%0), %%r9, %%r10;"     "  adcx %%r14, %%r9;"     // f[2]*f[0]
    "  mulxq 24(%0), %%rax, %%rcx;"    "  adcx %%rax, %%r10;"    // f[3]*f[0]
    "  movq 24(%0), %%rdx;"                                      // f[3]
    "  mulxq 8(%0), %%r11, %%rbx;"     "  adcx %%rcx, %%r11;"    // f[1]*f[3]
    "  mulxq 16(%0), %%rax, %%r13;"    "  adcx %%rax, %%rbx;"    // f[2]*f[3]
    "  movq 8(%0), %%rdx;"             "  adcx %%r15, %%r13;"    // f1
    "  mulxq 16(%0), %%rax, %%rcx;"    "  mov $0, %%r14;"        // f[2]*f[1]

    // Step 2: Compute two parallel carry chains
    "  xor %%r15d, %%r15d;"
    "  adox %%rax, %%r10;"
    "  adcx %%r8, %%r8;"
    "  adox %%rcx, %%r11;"
    "  adcx %%r9, %%r9;"
    "  adox %%r15, %%rbx;"
    "  adcx %%r10, %%r10;"
    "  adox %%r15, %%r13;"
    "  adcx %%r11, %%r11;"
    "  adox %%r15, %%r14;"
    "  adcx %%rbx, %%rbx;"
    "  adcx %%r13, %%r13;"
    "  adcx %%r14, %%r14;"

    // Step 3: Compute intermediate squares
    "  movq 0(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[0]^2
                               "  movq %%rax, 0(%1);"
    "  add %%rcx, %%r8;"       "  movq %%r8, 8(%1);"
    "  movq 8(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[1]^2
    "  adcx %%rax, %%r9;"      "  movq %%r9, 16(%1);"
    "  adcx %%rcx, %%r10;"     "  movq %%r10, 24(%1);"
    "  movq 16(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[2]^2
    "  adcx %%rax, %%r11;"     "  movq %%r11, 32(%1);"
    "  adcx %%rcx, %%rbx;"     "  movq %%rbx, 40(%1);"
    "  movq 24(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[3]^2
    "  adcx %%rax, %%r13;"     "  movq %%r13, 48(%1);"
    "  adcx %%rcx, %%r14;"     "  movq %%r14, 56(%1);"

    // Line up pointers
    "  mov %1, %0;"
    "  mov %2, %1;"

    /////// Wrap the result back into the field ////// 

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 32(%0), %%r8, %%r13;"
    "  xor %%ecx, %%ecx;"
    "  adoxq 0(%0), %%r8;"
    "  mulxq 40(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 8(%0), %%r9;"
    "  mulxq 48(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 16(%0), %%r10;"
    "  mulxq 56(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 24(%0), %%r11;"
    "  adcx %%rcx, %%rax;"
    "  adox %%rcx, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %%rcx, %%r9;"
    "  movq %%r9, 8(%1);"
    "  adcx %%rcx, %%r10;"
    "  movq %%r10, 16(%1);"
    "  adcx %%rcx, %%r11;"
    "  movq %%r11, 24(%1);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%1);"
  : "+&r" (f), "+&r" (tmp)
  : "r" (out)
  : "%rax", "%rbx", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r13", "%r14", "%r15", "memory", "cc"
  );
}

// Computes two field squarings:
//   out[0] <- f[0] * f[0]
//   out[1] <- f[1] * f[1]
// Uses the 16-element buffer tmp for intermediate results
static inline void fsqr2 (uint64_t *out, uint64_t *f, uint64_t *tmp) 
{
  __asm__ volatile(
    // Step 1: Compute all partial products
    "  movq 0(%0), %%rdx;"                                       // f[0]
    "  mulxq 8(%0), %%r8, %%r14;"      "  xor %%r15d, %%r15d;"     // f[1]*f[0]
    "  mulxq 16(%0), %%r9, %%r10;"     "  adcx %%r14, %%r9;"     // f[2]*f[0]
    "  mulxq 24(%0), %%rax, %%rcx;"    "  adcx %%rax, %%r10;"    // f[3]*f[0]
    "  movq 24(%0), %%rdx;"                                      // f[3]
    "  mulxq 8(%0), %%r11, %%rbx;"     "  adcx %%rcx, %%r11;"    // f[1]*f[3]
    "  mulxq 16(%0), %%rax, %%r13;"    "  adcx %%rax, %%rbx;"    // f[2]*f[3]
    "  movq 8(%0), %%rdx;"             "  adcx %%r15, %%r13;"    // f1
    "  mulxq 16(%0), %%rax, %%rcx;"    "  mov $0, %%r14;"        // f[2]*f[1]

    // Step 2: Compute two parallel carry chains
    "  xor %%r15d, %%r15d;"
    "  adox %%rax, %%r10;"
    "  adcx %%r8, %%r8;"
    "  adox %%rcx, %%r11;"
    "  adcx %%r9, %%r9;"
    "  adox %%r15, %%rbx;"
    "  adcx %%r10, %%r10;"
    "  adox %%r15, %%r13;"
    "  adcx %%r11, %%r11;"
    "  adox %%r15, %%r14;"
    "  adcx %%rbx, %%rbx;"
    "  adcx %%r13, %%r13;"
    "  adcx %%r14, %%r14;"

    // Step 3: Compute intermediate squares
    "  movq 0(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[0]^2
                               "  movq %%rax, 0(%1);"
    "  add %%rcx, %%r8;"       "  movq %%r8, 8(%1);"
    "  movq 8(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[1]^2
    "  adcx %%rax, %%r9;"      "  movq %%r9, 16(%1);"
    "  adcx %%rcx, %%r10;"     "  movq %%r10, 24(%1);"
    "  movq 16(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[2]^2
    "  adcx %%rax, %%r11;"     "  movq %%r11, 32(%1);"
    "  adcx %%rcx, %%rbx;"     "  movq %%rbx, 40(%1);"
    "  movq 24(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[3]^2
    "  adcx %%rax, %%r13;"     "  movq %%r13, 48(%1);"
    "  adcx %%rcx, %%r14;"     "  movq %%r14, 56(%1);"

    // Step 1: Compute all partial products
    "  movq 32(%0), %%rdx;"                                       // f[0]
    "  mulxq 40(%0), %%r8, %%r14;"      "  xor %%r15d, %%r15d;"     // f[1]*f[0]
    "  mulxq 48(%0), %%r9, %%r10;"     "  adcx %%r14, %%r9;"     // f[2]*f[0]
    "  mulxq 56(%0), %%rax, %%rcx;"    "  adcx %%rax, %%r10;"    // f[3]*f[0]
    "  movq 56(%0), %%rdx;"                                      // f[3]
    "  mulxq 40(%0), %%r11, %%rbx;"     "  adcx %%rcx, %%r11;"    // f[1]*f[3]
    "  mulxq 48(%0), %%rax, %%r13;"    "  adcx %%rax, %%rbx;"    // f[2]*f[3]
    "  movq 40(%0), %%rdx;"             "  adcx %%r15, %%r13;"    // f1
    "  mulxq 48(%0), %%rax, %%rcx;"    "  mov $0, %%r14;"        // f[2]*f[1]

    // Step 2: Compute two parallel carry chains
    "  xor %%r15d, %%r15d;"
    "  adox %%rax, %%r10;"
    "  adcx %%r8, %%r8;"
    "  adox %%rcx, %%r11;"
    "  adcx %%r9, %%r9;"
    "  adox %%r15, %%rbx;"
    "  adcx %%r10, %%r10;"
    "  adox %%r15, %%r13;"
    "  adcx %%r11, %%r11;"
    "  adox %%r15, %%r14;"
    "  adcx %%rbx, %%rbx;"
    "  adcx %%r13, %%r13;"
    "  adcx %%r14, %%r14;"

    // Step 3: Compute intermediate squares
    "  movq 32(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[0]^2
                               "  movq %%rax, 64(%1);"
    "  add %%rcx, %%r8;"       "  movq %%r8, 72(%1);"
    "  movq 40(%0), %%rdx;"     "  mulx %%rdx, %%rax, %%rcx;"    // f[1]^2
    "  adcx %%rax, %%r9;"      "  movq %%r9, 80(%1);"
    "  adcx %%rcx, %%r10;"     "  movq %%r10, 88(%1);"
    "  movq 48(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[2]^2
    "  adcx %%rax, %%r11;"     "  movq %%r11, 96(%1);"
    "  adcx %%rcx, %%rbx;"     "  movq %%rbx, 104(%1);"
    "  movq 56(%0), %%rdx;"    "  mulx %%rdx, %%rax, %%rcx;"    // f[3]^2
    "  adcx %%rax, %%r13;"     "  movq %%r13, 112(%1);"
    "  adcx %%rcx, %%r14;"     "  movq %%r14, 120(%1);"

    // Line up pointers
    "  mov %1, %0;"
    "  mov %2, %1;"

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 32(%0), %%r8, %%r13;"
    "  xor %%ecx, %%ecx;"
    "  adoxq 0(%0), %%r8;"
    "  mulxq 40(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 8(%0), %%r9;"
    "  mulxq 48(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 16(%0), %%r10;"
    "  mulxq 56(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 24(%0), %%r11;"
    "  adcx %%rcx, %%rax;"
    "  adox %%rcx, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %%rcx, %%r9;"
    "  movq %%r9, 8(%1);"
    "  adcx %%rcx, %%r10;"
    "  movq %%r10, 16(%1);"
    "  adcx %%rcx, %%r11;"
    "  movq %%r11, 24(%1);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 0(%1);"

    // Step 1: Compute dst + carry == tmp_hi * 38 + tmp_lo
    "  mov $38, %%rdx;"
    "  mulxq 96(%0), %%r8, %%r13;"
    "  xor %%ecx, %%ecx;"
    "  adoxq 64(%0), %%r8;"
    "  mulxq 104(%0), %%r9, %%rbx;"
    "  adcx %%r13, %%r9;"
    "  adoxq 72(%0), %%r9;"
    "  mulxq 112(%0), %%r10, %%r13;"
    "  adcx %%rbx, %%r10;"
    "  adoxq 80(%0), %%r10;"
    "  mulxq 120(%0), %%r11, %%rax;"
    "  adcx %%r13, %%r11;"
    "  adoxq 88(%0), %%r11;"
    "  adcx %%rcx, %%rax;"
    "  adox %%rcx, %%rax;"
    "  imul %%rdx, %%rax;"

    // Step 2: Fold the carry back into dst
    "  add %%rax, %%r8;"
    "  adcx %%rcx, %%r9;"
    "  movq %%r9, 40(%1);"
    "  adcx %%rcx, %%r10;"
    "  movq %%r10, 48(%1);"
    "  adcx %%rcx, %%r11;"
    "  movq %%r11, 56(%1);"

    // Step 3: Fold the carry bit back in; guaranteed not to carry at this point
    "  mov $0, %%rax;"
    "  cmovc %%rdx, %%rax;"
    "  add %%rax, %%r8;"
    "  movq %%r8, 32(%1);"
  : "+&r" (f), "+&r" (tmp)
  : "r" (out)
  : "%rax", "%rbx", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r13", "%r14", "%r15", "memory", "cc"
  );
}

#endif /* defined(__x86_64__) || defined(_M_X64) */
#endif /* __GNUC__ */
