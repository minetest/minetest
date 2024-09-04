#ifndef __EVERCRYPT_TARGETCONFIG_H
#define __EVERCRYPT_TARGETCONFIG_H

// Instead of listing the identifiers for the target architectures
// then defining the constant TARGET_ARCHITECTURE in config.h, we might simply
// define exactly one tag of the form TARGET_ARCHITECTURE_IS_... in config.h.
// However, for maintenance purposes, we use the first method in
// order to have all the possible values listed in one place.
// Note that for now, the only important id is TARGET_ARCHITECTURE_ID_X64,
// but the other ids might prove useful in the future if we make
// the dynamic feature detection more precise (see the functions
// has_vec128_not_avx/has_vec256_not_avx2 below).
#define TARGET_ARCHITECTURE_ID_UNKNOWN 0
#define TARGET_ARCHITECTURE_ID_X86 1
#define TARGET_ARCHITECTURE_ID_X64 2
#define TARGET_ARCHITECTURE_ID_ARM7 3
#define TARGET_ARCHITECTURE_ID_ARM8 4
#define TARGET_ARCHITECTURE_ID_SYSTEMZ 5
#define TARGET_ARCHITECTURE_ID_POWERPC64 6

#if defined(__has_include)
#if __has_include("config.h")
#include "config.h"
#else
#define TARGET_ARCHITECTURE TARGET_ARCHITECTURE_ID_UNKNOWN
#endif
#endif

// Those functions are called on non-x64 platforms for which the feature detection
// is not covered by vale's CPUID support; therefore, we hand-write in C ourselves.
// For now, on non-x64 platforms, if we can compile 128-bit vector code, we can
// also execute it; this is true of: Z, Power, ARM8. In the future, if we consider
// cross-compilation scenarios, we'll have to refine this predicate; it could be the case,
// for instance, that we want our code to run on old revisions of a system without
// vector instructions, in which case we'll have to do run-time feature detection
// in addition to compile-time detection.

#include <stdbool.h>

static inline bool has_vec128_not_avx () {
#if (TARGET_ARCHITECTURE != TARGET_ARCHITECTURE_ID_X64) && HACL_CAN_COMPILE_VEC128
  return true;
#else
  return false;
#endif
}

static inline bool has_vec256_not_avx2 () {
#if (TARGET_ARCHITECTURE != TARGET_ARCHITECTURE_ID_X64) && HACL_CAN_COMPILE_VEC256
  return true;
#else
  return false;
#endif
}

#endif
