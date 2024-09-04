#ifndef __FSTAR_INT_H
#define __FSTAR_INT_H

#include "internal/types.h"

/*
 * Arithmetic Shift Right operator
 *
 * In all C standards, a >> b is implementation-defined when a has a signed
 * type and a negative value. See e.g. 6.5.7 in
 * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 *
 * GCC, MSVC, and Clang implement a >> b as an arithmetic shift.
 *
 * GCC: https://gcc.gnu.org/onlinedocs/gcc-9.1.0/gcc/Integers-implementation.html#Integers-implementation
 * MSVC: https://docs.microsoft.com/en-us/cpp/cpp/left-shift-and-right-shift-operators-input-and-output?view=vs-2019#right-shifts
 * Clang: tested that Clang 7, 8 and 9 compile this to an arithmetic shift
 *
 * We implement arithmetic shift right simply as >> in these compilers
 * and bail out in others.
 */

#if !(defined(_MSC_VER) || defined(__GNUC__) || (defined(__clang__) && (__clang_major__ >= 7)))

static inline
int8_t FStar_Int8_shift_arithmetic_right(int8_t a, uint32_t b) {
  do {
    KRML_HOST_EPRINTF("Could not identify compiler so could not provide an implementation of signed arithmetic shift right.\n");
    KRML_HOST_EXIT(255);
  } while (0);
}

static inline
int16_t FStar_Int16_shift_arithmetic_right(int16_t a, uint32_t b) {
  do {
    KRML_HOST_EPRINTF("Could not identify compiler so could not provide an implementation of signed arithmetic shift right.\n");
    KRML_HOST_EXIT(255);
  } while (0);
}

static inline
int32_t FStar_Int32_shift_arithmetic_right(int32_t a, uint32_t b) {
  do {
    KRML_HOST_EPRINTF("Could not identify compiler so could not provide an implementation of signed arithmetic shift right.\n");
    KRML_HOST_EXIT(255);
  } while (0);
}

static inline
int64_t FStar_Int64_shift_arithmetic_right(int64_t a, uint32_t b) {
  do {
    KRML_HOST_EPRINTF("Could not identify compiler so could not provide an implementation of signed arithmetic shift right.\n");
    KRML_HOST_EXIT(255);
  } while (0);
}

#else

static inline
int8_t FStar_Int8_shift_arithmetic_right(int8_t a, uint32_t b) {
  return (a >> b);
}

static inline
int16_t FStar_Int16_shift_arithmetic_right(int16_t a, uint32_t b) {
  return (a >> b);
}

static inline
int32_t FStar_Int32_shift_arithmetic_right(int32_t a, uint32_t b) {
  return (a >> b);
}

static inline
int64_t FStar_Int64_shift_arithmetic_right(int64_t a, uint32_t b) {
  return (a >> b);
}

#endif     /* !(defined(_MSC_VER) ... ) */

#endif     /* __FSTAR_INT_H */
