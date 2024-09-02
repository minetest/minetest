.text
.global check_aesni
check_aesni:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $33554432, %rax
  shr $24, %rax
  and $2, %rcx
  and %rcx, %rax
  mov %r9, %rbx
  ret

.global check_sha
check_sha:
  mov %rbx, %r9
  mov $7, %rax
  mov $0, %rcx
  cpuid
  and $536870912, %rbx
  mov %rbx, %rax
  mov %r9, %rbx
  ret

.global check_adx_bmi2
check_adx_bmi2:
  mov %rbx, %r9
  mov $7, %rax
  mov $0, %rcx
  cpuid
  mov %rbx, %rax
  and $524288, %rax
  shr $11, %rax
  and $256, %rbx
  and %rbx, %rax
  mov %r9, %rbx
  ret

.global check_avx
check_avx:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $268435456, %rax
  shr $27, %rax
  mov %r9, %rbx
  ret

.global check_avx2
check_avx2:
  mov %rbx, %r9
  mov $7, %rax
  mov $0, %rcx
  cpuid
  and $32, %rbx
  mov %rbx, %rax
  mov %r9, %rbx
  ret

.global check_movbe
check_movbe:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $4194304, %rax
  shr $21, %rax
  mov %r9, %rbx
  ret

.global check_sse
check_sse:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $524288, %rax
  and $512, %rcx
  and $67108864, %rdx
  shr $10, %rax
  shr $17, %rdx
  and %rdx, %rax
  and %rcx, %rax
  mov %r9, %rbx
  ret

.global check_rdrand
check_rdrand:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $1073741824, %rax
  shr $29, %rax
  mov %r9, %rbx
  ret

.global check_avx512
check_avx512:
  mov %rbx, %r9
  mov $7, %rax
  mov $0, %rcx
  cpuid
  mov %rbx, %rax
  mov %rbx, %r10
  mov %rbx, %r11
  and $65536, %rbx
  and $131072, %rax
  and $1073741824, %r10
  shr $1, %rax
  shr $14, %r10
  and %rbx, %rax
  mov $2147483648, %rbx
  and %rbx, %r11
  shr $15, %r11
  and %r10, %rax
  and %r11, %rax
  mov %r9, %rbx
  ret

.global check_osxsave
check_osxsave:
  mov %rbx, %r9
  mov $0, %rcx
  mov $1, %rax
  cpuid
  mov %rcx, %rax
  and $134217728, %rax
  shr $26, %rax
  mov %r9, %rbx
  ret

.global check_avx_xcr0
check_avx_xcr0:
  mov $0, %rcx
  xgetbv
  mov %rax, %rcx
  and $4, %rax
  shr $1, %rax
  and $2, %rcx
  and %rcx, %rax
  ret

.global check_avx512_xcr0
check_avx512_xcr0:
  mov $0, %rcx
  xgetbv
  mov %rax, %rcx
  mov %rax, %rdx
  and $32, %rax
  and $64, %rcx
  and $128, %rdx
  shr $2, %rdx
  shr $1, %rcx
  and %rdx, %rax
  and %rcx, %rax
  ret


