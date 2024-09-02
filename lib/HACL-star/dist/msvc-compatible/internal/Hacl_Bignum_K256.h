/* MIT License
 *
 * Copyright (c) 2016-2022 INRIA, CMU and Microsoft Corporation
 * Copyright (c) 2022-2023 HACL* Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef __internal_Hacl_Bignum_K256_H
#define __internal_Hacl_Bignum_K256_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "internal/Hacl_Krmllib.h"
#include "Hacl_Krmllib.h"

static inline bool Hacl_K256_Field_is_felem_zero_vartime(uint64_t *f)
{
  uint64_t f0 = f[0U];
  uint64_t f1 = f[1U];
  uint64_t f2 = f[2U];
  uint64_t f3 = f[3U];
  uint64_t f4 = f[4U];
  return f0 == 0ULL && f1 == 0ULL && f2 == 0ULL && f3 == 0ULL && f4 == 0ULL;
}

static inline bool Hacl_K256_Field_is_felem_eq_vartime(uint64_t *f1, uint64_t *f2)
{
  uint64_t a0 = f1[0U];
  uint64_t a1 = f1[1U];
  uint64_t a2 = f1[2U];
  uint64_t a3 = f1[3U];
  uint64_t a4 = f1[4U];
  uint64_t b0 = f2[0U];
  uint64_t b1 = f2[1U];
  uint64_t b2 = f2[2U];
  uint64_t b3 = f2[3U];
  uint64_t b4 = f2[4U];
  return a0 == b0 && a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4;
}

static inline bool Hacl_K256_Field_is_felem_lt_prime_minus_order_vartime(uint64_t *f)
{
  uint64_t f0 = f[0U];
  uint64_t f1 = f[1U];
  uint64_t f2 = f[2U];
  uint64_t f3 = f[3U];
  uint64_t f4 = f[4U];
  if (f4 > 0ULL)
  {
    return false;
  }
  if (f3 > 0ULL)
  {
    return false;
  }
  if (f2 < 0x1455123ULL)
  {
    return true;
  }
  if (f2 > 0x1455123ULL)
  {
    return false;
  }
  if (f1 < 0x1950b75fc4402ULL)
  {
    return true;
  }
  if (f1 > 0x1950b75fc4402ULL)
  {
    return false;
  }
  return f0 < 0xda1722fc9baeeULL;
}

static inline void Hacl_K256_Field_load_felem(uint64_t *f, uint8_t *b)
{
  uint64_t tmp[4U] = { 0U };
  KRML_MAYBE_FOR4(i,
    0U,
    4U,
    1U,
    uint64_t *os = tmp;
    uint8_t *bj = b + i * 8U;
    uint64_t u = load64_be(bj);
    uint64_t r = u;
    uint64_t x = r;
    os[i] = x;);
  uint64_t s0 = tmp[3U];
  uint64_t s1 = tmp[2U];
  uint64_t s2 = tmp[1U];
  uint64_t s3 = tmp[0U];
  uint64_t f00 = s0 & 0xfffffffffffffULL;
  uint64_t f10 = s0 >> 52U | (s1 & 0xffffffffffULL) << 12U;
  uint64_t f20 = s1 >> 40U | (s2 & 0xfffffffULL) << 24U;
  uint64_t f30 = s2 >> 28U | (s3 & 0xffffULL) << 36U;
  uint64_t f40 = s3 >> 16U;
  uint64_t f0 = f00;
  uint64_t f1 = f10;
  uint64_t f2 = f20;
  uint64_t f3 = f30;
  uint64_t f4 = f40;
  f[0U] = f0;
  f[1U] = f1;
  f[2U] = f2;
  f[3U] = f3;
  f[4U] = f4;
}

static inline bool Hacl_K256_Field_load_felem_lt_prime_vartime(uint64_t *f, uint8_t *b)
{
  Hacl_K256_Field_load_felem(f, b);
  uint64_t f0 = f[0U];
  uint64_t f1 = f[1U];
  uint64_t f2 = f[2U];
  uint64_t f3 = f[3U];
  uint64_t f4 = f[4U];
  bool
  is_ge_p =
    f0
    >= 0xffffefffffc2fULL
    && f1 == 0xfffffffffffffULL
    && f2 == 0xfffffffffffffULL
    && f3 == 0xfffffffffffffULL
    && f4 == 0xffffffffffffULL;
  return !is_ge_p;
}

static inline void Hacl_K256_Field_store_felem(uint8_t *b, uint64_t *f)
{
  uint64_t tmp[4U] = { 0U };
  uint64_t f00 = f[0U];
  uint64_t f10 = f[1U];
  uint64_t f20 = f[2U];
  uint64_t f30 = f[3U];
  uint64_t f4 = f[4U];
  uint64_t o0 = f00 | f10 << 52U;
  uint64_t o1 = f10 >> 12U | f20 << 40U;
  uint64_t o2 = f20 >> 24U | f30 << 28U;
  uint64_t o3 = f30 >> 36U | f4 << 16U;
  uint64_t f0 = o0;
  uint64_t f1 = o1;
  uint64_t f2 = o2;
  uint64_t f3 = o3;
  tmp[0U] = f3;
  tmp[1U] = f2;
  tmp[2U] = f1;
  tmp[3U] = f0;
  KRML_MAYBE_FOR4(i, 0U, 4U, 1U, store64_be(b + i * 8U, tmp[i]););
}

static inline void Hacl_K256_Field_fmul_small_num(uint64_t *out, uint64_t *f, uint64_t num)
{
  uint64_t f00 = f[0U];
  uint64_t f10 = f[1U];
  uint64_t f20 = f[2U];
  uint64_t f30 = f[3U];
  uint64_t f40 = f[4U];
  uint64_t o0 = f00 * num;
  uint64_t o1 = f10 * num;
  uint64_t o2 = f20 * num;
  uint64_t o3 = f30 * num;
  uint64_t o4 = f40 * num;
  uint64_t f0 = o0;
  uint64_t f1 = o1;
  uint64_t f2 = o2;
  uint64_t f3 = o3;
  uint64_t f4 = o4;
  out[0U] = f0;
  out[1U] = f1;
  out[2U] = f2;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fadd(uint64_t *out, uint64_t *f1, uint64_t *f2)
{
  uint64_t a0 = f1[0U];
  uint64_t a1 = f1[1U];
  uint64_t a2 = f1[2U];
  uint64_t a3 = f1[3U];
  uint64_t a4 = f1[4U];
  uint64_t b0 = f2[0U];
  uint64_t b1 = f2[1U];
  uint64_t b2 = f2[2U];
  uint64_t b3 = f2[3U];
  uint64_t b4 = f2[4U];
  uint64_t o0 = a0 + b0;
  uint64_t o1 = a1 + b1;
  uint64_t o2 = a2 + b2;
  uint64_t o3 = a3 + b3;
  uint64_t o4 = a4 + b4;
  uint64_t f0 = o0;
  uint64_t f11 = o1;
  uint64_t f21 = o2;
  uint64_t f3 = o3;
  uint64_t f4 = o4;
  out[0U] = f0;
  out[1U] = f11;
  out[2U] = f21;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fsub(uint64_t *out, uint64_t *f1, uint64_t *f2, uint64_t x)
{
  uint64_t a0 = f1[0U];
  uint64_t a1 = f1[1U];
  uint64_t a2 = f1[2U];
  uint64_t a3 = f1[3U];
  uint64_t a4 = f1[4U];
  uint64_t b0 = f2[0U];
  uint64_t b1 = f2[1U];
  uint64_t b2 = f2[2U];
  uint64_t b3 = f2[3U];
  uint64_t b4 = f2[4U];
  uint64_t r00 = 9007190664804446ULL * x - b0;
  uint64_t r10 = 9007199254740990ULL * x - b1;
  uint64_t r20 = 9007199254740990ULL * x - b2;
  uint64_t r30 = 9007199254740990ULL * x - b3;
  uint64_t r40 = 562949953421310ULL * x - b4;
  uint64_t r0 = r00;
  uint64_t r1 = r10;
  uint64_t r2 = r20;
  uint64_t r3 = r30;
  uint64_t r4 = r40;
  uint64_t o0 = a0 + r0;
  uint64_t o1 = a1 + r1;
  uint64_t o2 = a2 + r2;
  uint64_t o3 = a3 + r3;
  uint64_t o4 = a4 + r4;
  uint64_t f0 = o0;
  uint64_t f11 = o1;
  uint64_t f21 = o2;
  uint64_t f3 = o3;
  uint64_t f4 = o4;
  out[0U] = f0;
  out[1U] = f11;
  out[2U] = f21;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fmul(uint64_t *out, uint64_t *f1, uint64_t *f2)
{
  uint64_t a0 = f1[0U];
  uint64_t a1 = f1[1U];
  uint64_t a2 = f1[2U];
  uint64_t a3 = f1[3U];
  uint64_t a4 = f1[4U];
  uint64_t b0 = f2[0U];
  uint64_t b1 = f2[1U];
  uint64_t b2 = f2[2U];
  uint64_t b3 = f2[3U];
  uint64_t b4 = f2[4U];
  uint64_t r = 0x1000003D10ULL;
  FStar_UInt128_uint128
  d0 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_mul_wide(a0,
            b3),
          FStar_UInt128_mul_wide(a1, b2)),
        FStar_UInt128_mul_wide(a2, b1)),
      FStar_UInt128_mul_wide(a3, b0));
  FStar_UInt128_uint128 c0 = FStar_UInt128_mul_wide(a4, b4);
  FStar_UInt128_uint128
  d1 = FStar_UInt128_add_mod(d0, FStar_UInt128_mul_wide(r, FStar_UInt128_uint128_to_uint64(c0)));
  uint64_t c1 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(c0, 64U));
  uint64_t t3 = FStar_UInt128_uint128_to_uint64(d1) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d2 = FStar_UInt128_shift_right(d1, 52U);
  FStar_UInt128_uint128
  d3 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(d2,
              FStar_UInt128_mul_wide(a0, b4)),
            FStar_UInt128_mul_wide(a1, b3)),
          FStar_UInt128_mul_wide(a2, b2)),
        FStar_UInt128_mul_wide(a3, b1)),
      FStar_UInt128_mul_wide(a4, b0));
  FStar_UInt128_uint128 d4 = FStar_UInt128_add_mod(d3, FStar_UInt128_mul_wide(r << 12U, c1));
  uint64_t t4 = FStar_UInt128_uint128_to_uint64(d4) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d5 = FStar_UInt128_shift_right(d4, 52U);
  uint64_t tx = t4 >> 48U;
  uint64_t t4_ = t4 & 0xffffffffffffULL;
  FStar_UInt128_uint128 c2 = FStar_UInt128_mul_wide(a0, b0);
  FStar_UInt128_uint128
  d6 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(d5,
            FStar_UInt128_mul_wide(a1, b4)),
          FStar_UInt128_mul_wide(a2, b3)),
        FStar_UInt128_mul_wide(a3, b2)),
      FStar_UInt128_mul_wide(a4, b1));
  uint64_t u0 = FStar_UInt128_uint128_to_uint64(d6) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d7 = FStar_UInt128_shift_right(d6, 52U);
  uint64_t u0_ = tx | u0 << 4U;
  FStar_UInt128_uint128 c3 = FStar_UInt128_add_mod(c2, FStar_UInt128_mul_wide(u0_, r >> 4U));
  uint64_t r0 = FStar_UInt128_uint128_to_uint64(c3) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c4 = FStar_UInt128_shift_right(c3, 52U);
  FStar_UInt128_uint128
  c5 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(c4, FStar_UInt128_mul_wide(a0, b1)),
      FStar_UInt128_mul_wide(a1, b0));
  FStar_UInt128_uint128
  d8 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(d7,
          FStar_UInt128_mul_wide(a2, b4)),
        FStar_UInt128_mul_wide(a3, b3)),
      FStar_UInt128_mul_wide(a4, b2));
  FStar_UInt128_uint128
  c6 =
    FStar_UInt128_add_mod(c5,
      FStar_UInt128_mul_wide(FStar_UInt128_uint128_to_uint64(d8) & 0xfffffffffffffULL, r));
  FStar_UInt128_uint128 d9 = FStar_UInt128_shift_right(d8, 52U);
  uint64_t r1 = FStar_UInt128_uint128_to_uint64(c6) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c7 = FStar_UInt128_shift_right(c6, 52U);
  FStar_UInt128_uint128
  c8 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(c7,
          FStar_UInt128_mul_wide(a0, b2)),
        FStar_UInt128_mul_wide(a1, b1)),
      FStar_UInt128_mul_wide(a2, b0));
  FStar_UInt128_uint128
  d10 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(d9, FStar_UInt128_mul_wide(a3, b4)),
      FStar_UInt128_mul_wide(a4, b3));
  FStar_UInt128_uint128
  c9 = FStar_UInt128_add_mod(c8, FStar_UInt128_mul_wide(r, FStar_UInt128_uint128_to_uint64(d10)));
  uint64_t d11 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(d10, 64U));
  uint64_t r2 = FStar_UInt128_uint128_to_uint64(c9) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c10 = FStar_UInt128_shift_right(c9, 52U);
  FStar_UInt128_uint128
  c11 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(c10, FStar_UInt128_mul_wide(r << 12U, d11)),
      FStar_UInt128_uint64_to_uint128(t3));
  uint64_t r3 = FStar_UInt128_uint128_to_uint64(c11) & 0xfffffffffffffULL;
  uint64_t c12 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(c11, 52U));
  uint64_t r4 = c12 + t4_;
  uint64_t f0 = r0;
  uint64_t f11 = r1;
  uint64_t f21 = r2;
  uint64_t f3 = r3;
  uint64_t f4 = r4;
  out[0U] = f0;
  out[1U] = f11;
  out[2U] = f21;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fsqr(uint64_t *out, uint64_t *f)
{
  uint64_t a0 = f[0U];
  uint64_t a1 = f[1U];
  uint64_t a2 = f[2U];
  uint64_t a3 = f[3U];
  uint64_t a4 = f[4U];
  uint64_t r = 0x1000003D10ULL;
  FStar_UInt128_uint128
  d0 =
    FStar_UInt128_add_mod(FStar_UInt128_mul_wide(a0 * 2ULL, a3),
      FStar_UInt128_mul_wide(a1 * 2ULL, a2));
  FStar_UInt128_uint128 c0 = FStar_UInt128_mul_wide(a4, a4);
  FStar_UInt128_uint128
  d1 = FStar_UInt128_add_mod(d0, FStar_UInt128_mul_wide(r, FStar_UInt128_uint128_to_uint64(c0)));
  uint64_t c1 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(c0, 64U));
  uint64_t t3 = FStar_UInt128_uint128_to_uint64(d1) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d2 = FStar_UInt128_shift_right(d1, 52U);
  uint64_t a41 = a4 * 2ULL;
  FStar_UInt128_uint128
  d3 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(FStar_UInt128_add_mod(d2,
          FStar_UInt128_mul_wide(a0, a41)),
        FStar_UInt128_mul_wide(a1 * 2ULL, a3)),
      FStar_UInt128_mul_wide(a2, a2));
  FStar_UInt128_uint128 d4 = FStar_UInt128_add_mod(d3, FStar_UInt128_mul_wide(r << 12U, c1));
  uint64_t t4 = FStar_UInt128_uint128_to_uint64(d4) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d5 = FStar_UInt128_shift_right(d4, 52U);
  uint64_t tx = t4 >> 48U;
  uint64_t t4_ = t4 & 0xffffffffffffULL;
  FStar_UInt128_uint128 c2 = FStar_UInt128_mul_wide(a0, a0);
  FStar_UInt128_uint128
  d6 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(d5, FStar_UInt128_mul_wide(a1, a41)),
      FStar_UInt128_mul_wide(a2 * 2ULL, a3));
  uint64_t u0 = FStar_UInt128_uint128_to_uint64(d6) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 d7 = FStar_UInt128_shift_right(d6, 52U);
  uint64_t u0_ = tx | u0 << 4U;
  FStar_UInt128_uint128 c3 = FStar_UInt128_add_mod(c2, FStar_UInt128_mul_wide(u0_, r >> 4U));
  uint64_t r0 = FStar_UInt128_uint128_to_uint64(c3) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c4 = FStar_UInt128_shift_right(c3, 52U);
  uint64_t a01 = a0 * 2ULL;
  FStar_UInt128_uint128 c5 = FStar_UInt128_add_mod(c4, FStar_UInt128_mul_wide(a01, a1));
  FStar_UInt128_uint128
  d8 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(d7, FStar_UInt128_mul_wide(a2, a41)),
      FStar_UInt128_mul_wide(a3, a3));
  FStar_UInt128_uint128
  c6 =
    FStar_UInt128_add_mod(c5,
      FStar_UInt128_mul_wide(FStar_UInt128_uint128_to_uint64(d8) & 0xfffffffffffffULL, r));
  FStar_UInt128_uint128 d9 = FStar_UInt128_shift_right(d8, 52U);
  uint64_t r1 = FStar_UInt128_uint128_to_uint64(c6) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c7 = FStar_UInt128_shift_right(c6, 52U);
  FStar_UInt128_uint128
  c8 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(c7, FStar_UInt128_mul_wide(a01, a2)),
      FStar_UInt128_mul_wide(a1, a1));
  FStar_UInt128_uint128 d10 = FStar_UInt128_add_mod(d9, FStar_UInt128_mul_wide(a3, a41));
  FStar_UInt128_uint128
  c9 = FStar_UInt128_add_mod(c8, FStar_UInt128_mul_wide(r, FStar_UInt128_uint128_to_uint64(d10)));
  uint64_t d11 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(d10, 64U));
  uint64_t r2 = FStar_UInt128_uint128_to_uint64(c9) & 0xfffffffffffffULL;
  FStar_UInt128_uint128 c10 = FStar_UInt128_shift_right(c9, 52U);
  FStar_UInt128_uint128
  c11 =
    FStar_UInt128_add_mod(FStar_UInt128_add_mod(c10, FStar_UInt128_mul_wide(r << 12U, d11)),
      FStar_UInt128_uint64_to_uint128(t3));
  uint64_t r3 = FStar_UInt128_uint128_to_uint64(c11) & 0xfffffffffffffULL;
  uint64_t c12 = FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(c11, 52U));
  uint64_t r4 = c12 + t4_;
  uint64_t f0 = r0;
  uint64_t f1 = r1;
  uint64_t f2 = r2;
  uint64_t f3 = r3;
  uint64_t f4 = r4;
  out[0U] = f0;
  out[1U] = f1;
  out[2U] = f2;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fnormalize_weak(uint64_t *out, uint64_t *f)
{
  uint64_t t0 = f[0U];
  uint64_t t1 = f[1U];
  uint64_t t2 = f[2U];
  uint64_t t3 = f[3U];
  uint64_t t4 = f[4U];
  uint64_t x0 = t4 >> 48U;
  uint64_t t410 = t4 & 0xffffffffffffULL;
  uint64_t x = x0;
  uint64_t t01 = t0;
  uint64_t t11 = t1;
  uint64_t t21 = t2;
  uint64_t t31 = t3;
  uint64_t t41 = t410;
  uint64_t t02 = t01 + x * 0x1000003D1ULL;
  uint64_t t12 = t11 + (t02 >> 52U);
  uint64_t t03 = t02 & 0xfffffffffffffULL;
  uint64_t t22 = t21 + (t12 >> 52U);
  uint64_t t13 = t12 & 0xfffffffffffffULL;
  uint64_t t32 = t31 + (t22 >> 52U);
  uint64_t t23 = t22 & 0xfffffffffffffULL;
  uint64_t t42 = t41 + (t32 >> 52U);
  uint64_t t33 = t32 & 0xfffffffffffffULL;
  uint64_t f0 = t03;
  uint64_t f1 = t13;
  uint64_t f2 = t23;
  uint64_t f3 = t33;
  uint64_t f4 = t42;
  out[0U] = f0;
  out[1U] = f1;
  out[2U] = f2;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fnormalize(uint64_t *out, uint64_t *f)
{
  uint64_t f00 = f[0U];
  uint64_t f10 = f[1U];
  uint64_t f20 = f[2U];
  uint64_t f30 = f[3U];
  uint64_t f40 = f[4U];
  uint64_t x0 = f40 >> 48U;
  uint64_t t40 = f40 & 0xffffffffffffULL;
  uint64_t x1 = x0;
  uint64_t t00 = f00;
  uint64_t t10 = f10;
  uint64_t t20 = f20;
  uint64_t t30 = f30;
  uint64_t t42 = t40;
  uint64_t t01 = t00 + x1 * 0x1000003D1ULL;
  uint64_t t110 = t10 + (t01 >> 52U);
  uint64_t t020 = t01 & 0xfffffffffffffULL;
  uint64_t t210 = t20 + (t110 >> 52U);
  uint64_t t120 = t110 & 0xfffffffffffffULL;
  uint64_t t310 = t30 + (t210 >> 52U);
  uint64_t t220 = t210 & 0xfffffffffffffULL;
  uint64_t t410 = t42 + (t310 >> 52U);
  uint64_t t320 = t310 & 0xfffffffffffffULL;
  uint64_t t0 = t020;
  uint64_t t1 = t120;
  uint64_t t2 = t220;
  uint64_t t3 = t320;
  uint64_t t4 = t410;
  uint64_t x2 = t4 >> 48U;
  uint64_t t411 = t4 & 0xffffffffffffULL;
  uint64_t x = x2;
  uint64_t r0 = t0;
  uint64_t r1 = t1;
  uint64_t r2 = t2;
  uint64_t r3 = t3;
  uint64_t r4 = t411;
  uint64_t m4 = FStar_UInt64_eq_mask(r4, 0xffffffffffffULL);
  uint64_t m3 = FStar_UInt64_eq_mask(r3, 0xfffffffffffffULL);
  uint64_t m2 = FStar_UInt64_eq_mask(r2, 0xfffffffffffffULL);
  uint64_t m1 = FStar_UInt64_eq_mask(r1, 0xfffffffffffffULL);
  uint64_t m0 = FStar_UInt64_gte_mask(r0, 0xffffefffffc2fULL);
  uint64_t is_ge_p_m = (((m0 & m1) & m2) & m3) & m4;
  uint64_t m_to_one = is_ge_p_m & 1ULL;
  uint64_t x10 = m_to_one | x;
  uint64_t t010 = r0 + x10 * 0x1000003D1ULL;
  uint64_t t11 = r1 + (t010 >> 52U);
  uint64_t t02 = t010 & 0xfffffffffffffULL;
  uint64_t t21 = r2 + (t11 >> 52U);
  uint64_t t12 = t11 & 0xfffffffffffffULL;
  uint64_t t31 = r3 + (t21 >> 52U);
  uint64_t t22 = t21 & 0xfffffffffffffULL;
  uint64_t t41 = r4 + (t31 >> 52U);
  uint64_t t32 = t31 & 0xfffffffffffffULL;
  uint64_t s0 = t02;
  uint64_t s1 = t12;
  uint64_t s2 = t22;
  uint64_t s3 = t32;
  uint64_t s4 = t41;
  uint64_t t412 = s4 & 0xffffffffffffULL;
  uint64_t k0 = s0;
  uint64_t k1 = s1;
  uint64_t k2 = s2;
  uint64_t k3 = s3;
  uint64_t k4 = t412;
  uint64_t f0 = k0;
  uint64_t f1 = k1;
  uint64_t f2 = k2;
  uint64_t f3 = k3;
  uint64_t f4 = k4;
  out[0U] = f0;
  out[1U] = f1;
  out[2U] = f2;
  out[3U] = f3;
  out[4U] = f4;
}

static inline void Hacl_K256_Field_fnegate_conditional_vartime(uint64_t *f, bool is_negate)
{
  if (is_negate)
  {
    uint64_t a0 = f[0U];
    uint64_t a1 = f[1U];
    uint64_t a2 = f[2U];
    uint64_t a3 = f[3U];
    uint64_t a4 = f[4U];
    uint64_t r0 = 9007190664804446ULL - a0;
    uint64_t r1 = 9007199254740990ULL - a1;
    uint64_t r2 = 9007199254740990ULL - a2;
    uint64_t r3 = 9007199254740990ULL - a3;
    uint64_t r4 = 562949953421310ULL - a4;
    uint64_t f0 = r0;
    uint64_t f1 = r1;
    uint64_t f2 = r2;
    uint64_t f3 = r3;
    uint64_t f4 = r4;
    f[0U] = f0;
    f[1U] = f1;
    f[2U] = f2;
    f[3U] = f3;
    f[4U] = f4;
    Hacl_K256_Field_fnormalize(f, f);
    return;
  }
}

static inline void Hacl_Impl_K256_Finv_fsquare_times_in_place(uint64_t *out, uint32_t b)
{
  for (uint32_t i = 0U; i < b; i++)
  {
    Hacl_K256_Field_fsqr(out, out);
  }
}

static inline void Hacl_Impl_K256_Finv_fsquare_times(uint64_t *out, uint64_t *a, uint32_t b)
{
  memcpy(out, a, 5U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < b; i++)
  {
    Hacl_K256_Field_fsqr(out, out);
  }
}

static inline void Hacl_Impl_K256_Finv_fexp_223_23(uint64_t *out, uint64_t *x2, uint64_t *f)
{
  uint64_t x3[5U] = { 0U };
  uint64_t x22[5U] = { 0U };
  uint64_t x44[5U] = { 0U };
  uint64_t x88[5U] = { 0U };
  Hacl_Impl_K256_Finv_fsquare_times(x2, f, 1U);
  Hacl_K256_Field_fmul(x2, x2, f);
  Hacl_Impl_K256_Finv_fsquare_times(x3, x2, 1U);
  Hacl_K256_Field_fmul(x3, x3, f);
  Hacl_Impl_K256_Finv_fsquare_times(out, x3, 3U);
  Hacl_K256_Field_fmul(out, out, x3);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 3U);
  Hacl_K256_Field_fmul(out, out, x3);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 2U);
  Hacl_K256_Field_fmul(out, out, x2);
  Hacl_Impl_K256_Finv_fsquare_times(x22, out, 11U);
  Hacl_K256_Field_fmul(x22, x22, out);
  Hacl_Impl_K256_Finv_fsquare_times(x44, x22, 22U);
  Hacl_K256_Field_fmul(x44, x44, x22);
  Hacl_Impl_K256_Finv_fsquare_times(x88, x44, 44U);
  Hacl_K256_Field_fmul(x88, x88, x44);
  Hacl_Impl_K256_Finv_fsquare_times(out, x88, 88U);
  Hacl_K256_Field_fmul(out, out, x88);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 44U);
  Hacl_K256_Field_fmul(out, out, x44);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 3U);
  Hacl_K256_Field_fmul(out, out, x3);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 23U);
  Hacl_K256_Field_fmul(out, out, x22);
}

static inline void Hacl_Impl_K256_Finv_finv(uint64_t *out, uint64_t *f)
{
  uint64_t x2[5U] = { 0U };
  Hacl_Impl_K256_Finv_fexp_223_23(out, x2, f);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 5U);
  Hacl_K256_Field_fmul(out, out, f);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 3U);
  Hacl_K256_Field_fmul(out, out, x2);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 2U);
  Hacl_K256_Field_fmul(out, out, f);
}

static inline void Hacl_Impl_K256_Finv_fsqrt(uint64_t *out, uint64_t *f)
{
  uint64_t x2[5U] = { 0U };
  Hacl_Impl_K256_Finv_fexp_223_23(out, x2, f);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 6U);
  Hacl_K256_Field_fmul(out, out, x2);
  Hacl_Impl_K256_Finv_fsquare_times_in_place(out, 2U);
}

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Bignum_K256_H_DEFINED
#endif
