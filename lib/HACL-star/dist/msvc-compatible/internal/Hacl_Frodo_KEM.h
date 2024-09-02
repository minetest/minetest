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


#ifndef __internal_Hacl_Frodo_KEM_H
#define __internal_Hacl_Frodo_KEM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "internal/Hacl_Spec.h"
#include "internal/Hacl_Krmllib.h"
#include "Lib_RandomBuffer_System.h"
#include "Hacl_Krmllib.h"
#include "Hacl_Hash_SHA3.h"

static inline void
Hacl_Keccak_shake128_4x(
  uint32_t input_len,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t output_len,
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3
)
{
  Hacl_Hash_SHA3_shake128(output0, output_len, input0, input_len);
  Hacl_Hash_SHA3_shake128(output1, output_len, input1, input_len);
  Hacl_Hash_SHA3_shake128(output2, output_len, input2, input_len);
  Hacl_Hash_SHA3_shake128(output3, output_len, input3, input_len);
}

static inline void
Hacl_Impl_Matrix_mod_pow2(uint32_t n1, uint32_t n2, uint32_t logq, uint16_t *a)
{
  if (logq < 16U)
  {
    for (uint32_t i0 = 0U; i0 < n1; i0++)
    {
      for (uint32_t i = 0U; i < n2; i++)
      {
        a[i0 * n2 + i] = (uint32_t)a[i0 * n2 + i] & ((1U << logq) - 1U);
      }
    }
    return;
  }
}

static inline void
Hacl_Impl_Matrix_matrix_add(uint32_t n1, uint32_t n2, uint16_t *a, uint16_t *b)
{
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i = 0U; i < n2; i++)
    {
      a[i0 * n2 + i] = (uint32_t)a[i0 * n2 + i] + (uint32_t)b[i0 * n2 + i];
    }
  }
}

static inline void
Hacl_Impl_Matrix_matrix_sub(uint32_t n1, uint32_t n2, uint16_t *a, uint16_t *b)
{
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i = 0U; i < n2; i++)
    {
      b[i0 * n2 + i] = (uint32_t)a[i0 * n2 + i] - (uint32_t)b[i0 * n2 + i];
    }
  }
}

static inline void
Hacl_Impl_Matrix_matrix_mul(
  uint32_t n1,
  uint32_t n2,
  uint32_t n3,
  uint16_t *a,
  uint16_t *b,
  uint16_t *c
)
{
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n3; i1++)
    {
      uint16_t res = 0U;
      for (uint32_t i = 0U; i < n2; i++)
      {
        uint16_t aij = a[i0 * n2 + i];
        uint16_t bjk = b[i * n3 + i1];
        uint16_t res0 = res;
        res = (uint32_t)res0 + (uint32_t)aij * (uint32_t)bjk;
      }
      c[i0 * n3 + i1] = res;
    }
  }
}

static inline void
Hacl_Impl_Matrix_matrix_mul_s(
  uint32_t n1,
  uint32_t n2,
  uint32_t n3,
  uint16_t *a,
  uint16_t *b,
  uint16_t *c
)
{
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n3; i1++)
    {
      uint16_t res = 0U;
      for (uint32_t i = 0U; i < n2; i++)
      {
        uint16_t aij = a[i0 * n2 + i];
        uint16_t bjk = b[i1 * n2 + i];
        uint16_t res0 = res;
        res = (uint32_t)res0 + (uint32_t)aij * (uint32_t)bjk;
      }
      c[i0 * n3 + i1] = res;
    }
  }
}

static inline uint16_t
Hacl_Impl_Matrix_matrix_eq(uint32_t n1, uint32_t n2, uint16_t *a, uint16_t *b)
{
  uint16_t res = 0xFFFFU;
  for (uint32_t i = 0U; i < n1 * n2; i++)
  {
    uint16_t uu____0 = FStar_UInt16_eq_mask(a[i], b[i]);
    res = (uint32_t)uu____0 & (uint32_t)res;
  }
  uint16_t r = res;
  return r;
}

static inline void
Hacl_Impl_Matrix_matrix_to_lbytes(uint32_t n1, uint32_t n2, uint16_t *m, uint8_t *res)
{
  for (uint32_t i = 0U; i < n1 * n2; i++)
  {
    store16_le(res + 2U * i, m[i]);
  }
}

static inline void
Hacl_Impl_Matrix_matrix_from_lbytes(uint32_t n1, uint32_t n2, uint8_t *b, uint16_t *res)
{
  for (uint32_t i = 0U; i < n1 * n2; i++)
  {
    uint16_t *os = res;
    uint16_t u = load16_le(b + 2U * i);
    uint16_t x = u;
    os[i] = x;
  }
}

static inline void
Hacl_Impl_Frodo_Gen_frodo_gen_matrix_shake_4x(uint32_t n, uint8_t *seed, uint16_t *res)
{
  KRML_CHECK_SIZE(sizeof (uint8_t), 8U * n);
  uint8_t *r = (uint8_t *)alloca(8U * n * sizeof (uint8_t));
  memset(r, 0U, 8U * n * sizeof (uint8_t));
  uint8_t tmp_seed[72U] = { 0U };
  memcpy(tmp_seed + 2U, seed, 16U * sizeof (uint8_t));
  memcpy(tmp_seed + 20U, seed, 16U * sizeof (uint8_t));
  memcpy(tmp_seed + 38U, seed, 16U * sizeof (uint8_t));
  memcpy(tmp_seed + 56U, seed, 16U * sizeof (uint8_t));
  memset(res, 0U, n * n * sizeof (uint16_t));
  for (uint32_t i = 0U; i < n / 4U; i++)
  {
    uint8_t *r0 = r + 0U * n;
    uint8_t *r1 = r + 2U * n;
    uint8_t *r2 = r + 4U * n;
    uint8_t *r3 = r + 6U * n;
    uint8_t *tmp_seed0 = tmp_seed;
    uint8_t *tmp_seed1 = tmp_seed + 18U;
    uint8_t *tmp_seed2 = tmp_seed + 36U;
    uint8_t *tmp_seed3 = tmp_seed + 54U;
    store16_le(tmp_seed0, (uint16_t)(4U * i + 0U));
    store16_le(tmp_seed1, (uint16_t)(4U * i + 1U));
    store16_le(tmp_seed2, (uint16_t)(4U * i + 2U));
    store16_le(tmp_seed3, (uint16_t)(4U * i + 3U));
    Hacl_Keccak_shake128_4x(18U,
      tmp_seed0,
      tmp_seed1,
      tmp_seed2,
      tmp_seed3,
      2U * n,
      r0,
      r1,
      r2,
      r3);
    for (uint32_t i0 = 0U; i0 < n; i0++)
    {
      uint8_t *resij0 = r0 + i0 * 2U;
      uint8_t *resij1 = r1 + i0 * 2U;
      uint8_t *resij2 = r2 + i0 * 2U;
      uint8_t *resij3 = r3 + i0 * 2U;
      uint16_t u = load16_le(resij0);
      res[(4U * i + 0U) * n + i0] = u;
      uint16_t u0 = load16_le(resij1);
      res[(4U * i + 1U) * n + i0] = u0;
      uint16_t u1 = load16_le(resij2);
      res[(4U * i + 2U) * n + i0] = u1;
      uint16_t u2 = load16_le(resij3);
      res[(4U * i + 3U) * n + i0] = u2;
    }
  }
}

static inline void
Hacl_Impl_Frodo_Params_frodo_gen_matrix(
  Spec_Frodo_Params_frodo_gen_a a,
  uint32_t n,
  uint8_t *seed,
  uint16_t *a_matrix
)
{
  switch (a)
  {
    case Spec_Frodo_Params_SHAKE128:
      {
        Hacl_Impl_Frodo_Gen_frodo_gen_matrix_shake_4x(n, seed, a_matrix);
        break;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static const
uint16_t
Hacl_Impl_Frodo_Params_cdf_table640[13U] =
  {
    4643U, 13363U, 20579U, 25843U, 29227U, 31145U, 32103U, 32525U, 32689U, 32745U, 32762U, 32766U,
    32767U
  };

static const
uint16_t
Hacl_Impl_Frodo_Params_cdf_table976[11U] =
  { 5638U, 15915U, 23689U, 28571U, 31116U, 32217U, 32613U, 32731U, 32760U, 32766U, 32767U };

static const
uint16_t
Hacl_Impl_Frodo_Params_cdf_table1344[7U] =
  { 9142U, 23462U, 30338U, 32361U, 32725U, 32765U, 32767U };

static inline void
Hacl_Impl_Frodo_Sample_frodo_sample_matrix64(
  uint32_t n1,
  uint32_t n2,
  uint8_t *r,
  uint16_t *res
)
{
  memset(res, 0U, n1 * n2 * sizeof (uint16_t));
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n2; i1++)
    {
      uint8_t *resij = r + 2U * (n2 * i0 + i1);
      uint16_t u = load16_le(resij);
      uint16_t uu____0 = u;
      uint16_t prnd = (uint32_t)uu____0 >> 1U;
      uint16_t sign = (uint32_t)uu____0 & 1U;
      uint16_t sample = 0U;
      uint32_t bound = 12U;
      for (uint32_t i = 0U; i < bound; i++)
      {
        uint16_t sample0 = sample;
        uint16_t ti = Hacl_Impl_Frodo_Params_cdf_table640[i];
        uint16_t samplei = (uint32_t)(uint16_t)(uint32_t)((uint32_t)ti - (uint32_t)prnd) >> 15U;
        sample = (uint32_t)samplei + (uint32_t)sample0;
      }
      uint16_t sample0 = sample;
      res[i0 * n2 + i1] = (((uint32_t)~sign + 1U) ^ (uint32_t)sample0) + (uint32_t)sign;
    }
  }
}

static inline void
Hacl_Impl_Frodo_Sample_frodo_sample_matrix640(
  uint32_t n1,
  uint32_t n2,
  uint8_t *r,
  uint16_t *res
)
{
  memset(res, 0U, n1 * n2 * sizeof (uint16_t));
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n2; i1++)
    {
      uint8_t *resij = r + 2U * (n2 * i0 + i1);
      uint16_t u = load16_le(resij);
      uint16_t uu____0 = u;
      uint16_t prnd = (uint32_t)uu____0 >> 1U;
      uint16_t sign = (uint32_t)uu____0 & 1U;
      uint16_t sample = 0U;
      uint32_t bound = 12U;
      for (uint32_t i = 0U; i < bound; i++)
      {
        uint16_t sample0 = sample;
        uint16_t ti = Hacl_Impl_Frodo_Params_cdf_table640[i];
        uint16_t samplei = (uint32_t)(uint16_t)(uint32_t)((uint32_t)ti - (uint32_t)prnd) >> 15U;
        sample = (uint32_t)samplei + (uint32_t)sample0;
      }
      uint16_t sample0 = sample;
      res[i0 * n2 + i1] = (((uint32_t)~sign + 1U) ^ (uint32_t)sample0) + (uint32_t)sign;
    }
  }
}

static inline void
Hacl_Impl_Frodo_Sample_frodo_sample_matrix976(
  uint32_t n1,
  uint32_t n2,
  uint8_t *r,
  uint16_t *res
)
{
  memset(res, 0U, n1 * n2 * sizeof (uint16_t));
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n2; i1++)
    {
      uint8_t *resij = r + 2U * (n2 * i0 + i1);
      uint16_t u = load16_le(resij);
      uint16_t uu____0 = u;
      uint16_t prnd = (uint32_t)uu____0 >> 1U;
      uint16_t sign = (uint32_t)uu____0 & 1U;
      uint16_t sample = 0U;
      uint32_t bound = 10U;
      for (uint32_t i = 0U; i < bound; i++)
      {
        uint16_t sample0 = sample;
        uint16_t ti = Hacl_Impl_Frodo_Params_cdf_table976[i];
        uint16_t samplei = (uint32_t)(uint16_t)(uint32_t)((uint32_t)ti - (uint32_t)prnd) >> 15U;
        sample = (uint32_t)samplei + (uint32_t)sample0;
      }
      uint16_t sample0 = sample;
      res[i0 * n2 + i1] = (((uint32_t)~sign + 1U) ^ (uint32_t)sample0) + (uint32_t)sign;
    }
  }
}

static inline void
Hacl_Impl_Frodo_Sample_frodo_sample_matrix1344(
  uint32_t n1,
  uint32_t n2,
  uint8_t *r,
  uint16_t *res
)
{
  memset(res, 0U, n1 * n2 * sizeof (uint16_t));
  for (uint32_t i0 = 0U; i0 < n1; i0++)
  {
    for (uint32_t i1 = 0U; i1 < n2; i1++)
    {
      uint8_t *resij = r + 2U * (n2 * i0 + i1);
      uint16_t u = load16_le(resij);
      uint16_t uu____0 = u;
      uint16_t prnd = (uint32_t)uu____0 >> 1U;
      uint16_t sign = (uint32_t)uu____0 & 1U;
      uint16_t sample = 0U;
      uint32_t bound = 6U;
      for (uint32_t i = 0U; i < bound; i++)
      {
        uint16_t sample0 = sample;
        uint16_t ti = Hacl_Impl_Frodo_Params_cdf_table1344[i];
        uint16_t samplei = (uint32_t)(uint16_t)(uint32_t)((uint32_t)ti - (uint32_t)prnd) >> 15U;
        sample = (uint32_t)samplei + (uint32_t)sample0;
      }
      uint16_t sample0 = sample;
      res[i0 * n2 + i1] = (((uint32_t)~sign + 1U) ^ (uint32_t)sample0) + (uint32_t)sign;
    }
  }
}

void randombytes_(uint32_t len, uint8_t *res);

static inline void
Hacl_Impl_Frodo_Pack_frodo_pack(
  uint32_t n1,
  uint32_t n2,
  uint32_t d,
  uint16_t *a,
  uint8_t *res
)
{
  uint32_t n = n1 * n2 / 8U;
  for (uint32_t i = 0U; i < n; i++)
  {
    uint16_t *a1 = a + 8U * i;
    uint8_t *r = res + d * i;
    uint16_t maskd = (uint32_t)(uint16_t)(1U << d) - 1U;
    uint8_t v16[16U] = { 0U };
    uint16_t a0 = (uint32_t)a1[0U] & (uint32_t)maskd;
    uint16_t a11 = (uint32_t)a1[1U] & (uint32_t)maskd;
    uint16_t a2 = (uint32_t)a1[2U] & (uint32_t)maskd;
    uint16_t a3 = (uint32_t)a1[3U] & (uint32_t)maskd;
    uint16_t a4 = (uint32_t)a1[4U] & (uint32_t)maskd;
    uint16_t a5 = (uint32_t)a1[5U] & (uint32_t)maskd;
    uint16_t a6 = (uint32_t)a1[6U] & (uint32_t)maskd;
    uint16_t a7 = (uint32_t)a1[7U] & (uint32_t)maskd;
    FStar_UInt128_uint128
    templong =
      FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_logor(FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a0),
                      7U * d),
                    FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a11), 6U * d)),
                  FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a2), 5U * d)),
                FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a3), 4U * d)),
              FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a4), 3U * d)),
            FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a5), 2U * d)),
          FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a6), 1U * d)),
        FStar_UInt128_shift_left(FStar_UInt128_uint64_to_uint128((uint64_t)a7), 0U * d));
    store128_be(v16, templong);
    uint8_t *src = v16 + 16U - d;
    memcpy(r, src, d * sizeof (uint8_t));
  }
}

static inline void
Hacl_Impl_Frodo_Pack_frodo_unpack(
  uint32_t n1,
  uint32_t n2,
  uint32_t d,
  uint8_t *b,
  uint16_t *res
)
{
  uint32_t n = n1 * n2 / 8U;
  for (uint32_t i = 0U; i < n; i++)
  {
    uint8_t *b1 = b + d * i;
    uint16_t *r = res + 8U * i;
    uint16_t maskd = (uint32_t)(uint16_t)(1U << d) - 1U;
    uint8_t src[16U] = { 0U };
    memcpy(src + 16U - d, b1, d * sizeof (uint8_t));
    FStar_UInt128_uint128 u = load128_be(src);
    FStar_UInt128_uint128 templong = u;
    r[0U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          7U * d))
      & (uint32_t)maskd;
    r[1U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          6U * d))
      & (uint32_t)maskd;
    r[2U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          5U * d))
      & (uint32_t)maskd;
    r[3U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          4U * d))
      & (uint32_t)maskd;
    r[4U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          3U * d))
      & (uint32_t)maskd;
    r[5U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          2U * d))
      & (uint32_t)maskd;
    r[6U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          1U * d))
      & (uint32_t)maskd;
    r[7U] =
      (uint32_t)(uint16_t)FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(templong,
          0U * d))
      & (uint32_t)maskd;
  }
}

static inline void
Hacl_Impl_Frodo_Encode_frodo_key_encode(
  uint32_t logq,
  uint32_t b,
  uint32_t n,
  uint8_t *a,
  uint16_t *res
)
{
  for (uint32_t i0 = 0U; i0 < n; i0++)
  {
    uint8_t v8[8U] = { 0U };
    uint8_t *chunk = a + i0 * b;
    memcpy(v8, chunk, b * sizeof (uint8_t));
    uint64_t u = load64_le(v8);
    uint64_t x = u;
    uint64_t x0 = x;
    KRML_MAYBE_FOR8(i,
      0U,
      8U,
      1U,
      uint64_t rk = x0 >> b * i & ((1ULL << b) - 1ULL);
      res[i0 * n + i] = (uint32_t)(uint16_t)rk << (logq - b););
  }
}

static inline void
Hacl_Impl_Frodo_Encode_frodo_key_decode(
  uint32_t logq,
  uint32_t b,
  uint32_t n,
  uint16_t *a,
  uint8_t *res
)
{
  for (uint32_t i0 = 0U; i0 < n; i0++)
  {
    uint64_t templong = 0ULL;
    KRML_MAYBE_FOR8(i,
      0U,
      8U,
      1U,
      uint16_t aik = a[i0 * n + i];
      uint16_t res1 = (((uint32_t)aik + (1U << (logq - b - 1U))) & 0xFFFFU) >> (logq - b);
      templong = templong | (uint64_t)((uint32_t)res1 & ((1U << b) - 1U)) << b * i;);
    uint64_t templong0 = templong;
    uint8_t v8[8U] = { 0U };
    store64_le(v8, templong0);
    uint8_t *tmp = v8;
    memcpy(res + i0 * b, tmp, b * sizeof (uint8_t));
  }
}

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Frodo_KEM_H_DEFINED
#endif
