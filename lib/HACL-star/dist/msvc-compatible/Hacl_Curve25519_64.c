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


#include "Hacl_Curve25519_64.h"

#include "internal/Vale.h"
#include "internal/Hacl_Krmllib.h"
#include "config.h"
#include "curve25519-inline.h"

static inline void add_scalar0(uint64_t *out, uint64_t *f1, uint64_t f2)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  add_scalar(out, f1, f2);
  #else
  add_scalar_e(out, f1, f2);
  #endif
}

static inline void fadd0(uint64_t *out, uint64_t *f1, uint64_t *f2)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fadd(out, f1, f2);
  #else
  fadd_e(out, f1, f2);
  #endif
}

static inline void fsub0(uint64_t *out, uint64_t *f1, uint64_t *f2)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fsub(out, f1, f2);
  #else
  fsub_e(out, f1, f2);
  #endif
}

static inline void fmul0(uint64_t *out, uint64_t *f1, uint64_t *f2, uint64_t *tmp)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fmul(out, f1, f2, tmp);
  #else
  fmul_e(tmp, f1, out, f2);
  #endif
}

static inline void fmul20(uint64_t *out, uint64_t *f1, uint64_t *f2, uint64_t *tmp)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fmul2(out, f1, f2, tmp);
  #else
  fmul2_e(tmp, f1, out, f2);
  #endif
}

static inline void fmul_scalar0(uint64_t *out, uint64_t *f1, uint64_t f2)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fmul_scalar(out, f1, f2);
  #else
  fmul_scalar_e(out, f1, f2);
  #endif
}

static inline void fsqr0(uint64_t *out, uint64_t *f1, uint64_t *tmp)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fsqr(out, f1, tmp);
  #else
  fsqr_e(tmp, f1, out);
  #endif
}

static inline void fsqr20(uint64_t *out, uint64_t *f, uint64_t *tmp)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  fsqr2(out, f, tmp);
  #else
  fsqr2_e(tmp, f, out);
  #endif
}

static inline void cswap20(uint64_t bit, uint64_t *p1, uint64_t *p2)
{
  #if HACL_CAN_COMPILE_INLINE_ASM
  cswap2(bit, p1, p2);
  #else
  cswap2_e(bit, p1, p2);
  #endif
}

static const uint8_t g25519[32U] = { 9U };

static void point_add_and_double(uint64_t *q, uint64_t *p01_tmp1, uint64_t *tmp2)
{
  uint64_t *nq = p01_tmp1;
  uint64_t *nq_p1 = p01_tmp1 + 8U;
  uint64_t *tmp1 = p01_tmp1 + 16U;
  uint64_t *x1 = q;
  uint64_t *x2 = nq;
  uint64_t *z2 = nq + 4U;
  uint64_t *z3 = nq_p1 + 4U;
  uint64_t *a = tmp1;
  uint64_t *b = tmp1 + 4U;
  uint64_t *ab = tmp1;
  uint64_t *dc = tmp1 + 8U;
  fadd0(a, x2, z2);
  fsub0(b, x2, z2);
  uint64_t *x3 = nq_p1;
  uint64_t *z31 = nq_p1 + 4U;
  uint64_t *d0 = dc;
  uint64_t *c0 = dc + 4U;
  fadd0(c0, x3, z31);
  fsub0(d0, x3, z31);
  fmul20(dc, dc, ab, tmp2);
  fadd0(x3, d0, c0);
  fsub0(z31, d0, c0);
  uint64_t *a1 = tmp1;
  uint64_t *b1 = tmp1 + 4U;
  uint64_t *d = tmp1 + 8U;
  uint64_t *c = tmp1 + 12U;
  uint64_t *ab1 = tmp1;
  uint64_t *dc1 = tmp1 + 8U;
  fsqr20(dc1, ab1, tmp2);
  fsqr20(nq_p1, nq_p1, tmp2);
  a1[0U] = c[0U];
  a1[1U] = c[1U];
  a1[2U] = c[2U];
  a1[3U] = c[3U];
  fsub0(c, d, c);
  fmul_scalar0(b1, c, 121665ULL);
  fadd0(b1, b1, d);
  fmul20(nq, dc1, ab1, tmp2);
  fmul0(z3, z3, x1, tmp2);
}

static void point_double(uint64_t *nq, uint64_t *tmp1, uint64_t *tmp2)
{
  uint64_t *x2 = nq;
  uint64_t *z2 = nq + 4U;
  uint64_t *a = tmp1;
  uint64_t *b = tmp1 + 4U;
  uint64_t *d = tmp1 + 8U;
  uint64_t *c = tmp1 + 12U;
  uint64_t *ab = tmp1;
  uint64_t *dc = tmp1 + 8U;
  fadd0(a, x2, z2);
  fsub0(b, x2, z2);
  fsqr20(dc, ab, tmp2);
  a[0U] = c[0U];
  a[1U] = c[1U];
  a[2U] = c[2U];
  a[3U] = c[3U];
  fsub0(c, d, c);
  fmul_scalar0(b, c, 121665ULL);
  fadd0(b, b, d);
  fmul20(nq, dc, ab, tmp2);
}

static void montgomery_ladder(uint64_t *out, uint8_t *key, uint64_t *init)
{
  uint64_t tmp2[16U] = { 0U };
  uint64_t p01_tmp1_swap[33U] = { 0U };
  uint64_t *p0 = p01_tmp1_swap;
  uint64_t *p01 = p01_tmp1_swap;
  uint64_t *p03 = p01;
  uint64_t *p11 = p01 + 8U;
  memcpy(p11, init, 8U * sizeof (uint64_t));
  uint64_t *x0 = p03;
  uint64_t *z0 = p03 + 4U;
  x0[0U] = 1ULL;
  x0[1U] = 0ULL;
  x0[2U] = 0ULL;
  x0[3U] = 0ULL;
  z0[0U] = 0ULL;
  z0[1U] = 0ULL;
  z0[2U] = 0ULL;
  z0[3U] = 0ULL;
  uint64_t *p01_tmp1 = p01_tmp1_swap;
  uint64_t *p01_tmp11 = p01_tmp1_swap;
  uint64_t *nq1 = p01_tmp1_swap;
  uint64_t *nq_p11 = p01_tmp1_swap + 8U;
  uint64_t *swap = p01_tmp1_swap + 32U;
  cswap20(1ULL, nq1, nq_p11);
  point_add_and_double(init, p01_tmp11, tmp2);
  swap[0U] = 1ULL;
  for (uint32_t i = 0U; i < 251U; i++)
  {
    uint64_t *p01_tmp12 = p01_tmp1_swap;
    uint64_t *swap1 = p01_tmp1_swap + 32U;
    uint64_t *nq2 = p01_tmp12;
    uint64_t *nq_p12 = p01_tmp12 + 8U;
    uint64_t bit = (uint64_t)((uint32_t)key[(253U - i) / 8U] >> (253U - i) % 8U & 1U);
    uint64_t sw = swap1[0U] ^ bit;
    cswap20(sw, nq2, nq_p12);
    point_add_and_double(init, p01_tmp12, tmp2);
    swap1[0U] = bit;
  }
  uint64_t sw = swap[0U];
  cswap20(sw, nq1, nq_p11);
  uint64_t *nq10 = p01_tmp1;
  uint64_t *tmp1 = p01_tmp1 + 16U;
  point_double(nq10, tmp1, tmp2);
  point_double(nq10, tmp1, tmp2);
  point_double(nq10, tmp1, tmp2);
  memcpy(out, p0, 8U * sizeof (uint64_t));
}

static void fsquare_times(uint64_t *o, uint64_t *inp, uint64_t *tmp, uint32_t n)
{
  fsqr0(o, inp, tmp);
  for (uint32_t i = 0U; i < n - 1U; i++)
  {
    fsqr0(o, o, tmp);
  }
}

static void finv(uint64_t *o, uint64_t *i, uint64_t *tmp)
{
  uint64_t t1[16U] = { 0U };
  uint64_t *a1 = t1;
  uint64_t *b1 = t1 + 4U;
  uint64_t *t010 = t1 + 12U;
  uint64_t *tmp10 = tmp;
  fsquare_times(a1, i, tmp10, 1U);
  fsquare_times(t010, a1, tmp10, 2U);
  fmul0(b1, t010, i, tmp);
  fmul0(a1, b1, a1, tmp);
  fsquare_times(t010, a1, tmp10, 1U);
  fmul0(b1, t010, b1, tmp);
  fsquare_times(t010, b1, tmp10, 5U);
  fmul0(b1, t010, b1, tmp);
  uint64_t *b10 = t1 + 4U;
  uint64_t *c10 = t1 + 8U;
  uint64_t *t011 = t1 + 12U;
  uint64_t *tmp11 = tmp;
  fsquare_times(t011, b10, tmp11, 10U);
  fmul0(c10, t011, b10, tmp);
  fsquare_times(t011, c10, tmp11, 20U);
  fmul0(t011, t011, c10, tmp);
  fsquare_times(t011, t011, tmp11, 10U);
  fmul0(b10, t011, b10, tmp);
  fsquare_times(t011, b10, tmp11, 50U);
  fmul0(c10, t011, b10, tmp);
  uint64_t *b11 = t1 + 4U;
  uint64_t *c1 = t1 + 8U;
  uint64_t *t01 = t1 + 12U;
  uint64_t *tmp1 = tmp;
  fsquare_times(t01, c1, tmp1, 100U);
  fmul0(t01, t01, c1, tmp);
  fsquare_times(t01, t01, tmp1, 50U);
  fmul0(t01, t01, b11, tmp);
  fsquare_times(t01, t01, tmp1, 5U);
  uint64_t *a = t1;
  uint64_t *t0 = t1 + 12U;
  fmul0(o, t0, a, tmp);
}

static void store_felem(uint64_t *b, uint64_t *f)
{
  uint64_t f30 = f[3U];
  uint64_t top_bit0 = f30 >> 63U;
  f[3U] = f30 & 0x7fffffffffffffffULL;
  add_scalar0(f, f, 19ULL * top_bit0);
  uint64_t f31 = f[3U];
  uint64_t top_bit = f31 >> 63U;
  f[3U] = f31 & 0x7fffffffffffffffULL;
  add_scalar0(f, f, 19ULL * top_bit);
  uint64_t f0 = f[0U];
  uint64_t f1 = f[1U];
  uint64_t f2 = f[2U];
  uint64_t f3 = f[3U];
  uint64_t m0 = FStar_UInt64_gte_mask(f0, 0xffffffffffffffedULL);
  uint64_t m1 = FStar_UInt64_eq_mask(f1, 0xffffffffffffffffULL);
  uint64_t m2 = FStar_UInt64_eq_mask(f2, 0xffffffffffffffffULL);
  uint64_t m3 = FStar_UInt64_eq_mask(f3, 0x7fffffffffffffffULL);
  uint64_t mask = ((m0 & m1) & m2) & m3;
  uint64_t f0_ = f0 - (mask & 0xffffffffffffffedULL);
  uint64_t f1_ = f1 - (mask & 0xffffffffffffffffULL);
  uint64_t f2_ = f2 - (mask & 0xffffffffffffffffULL);
  uint64_t f3_ = f3 - (mask & 0x7fffffffffffffffULL);
  uint64_t o0 = f0_;
  uint64_t o1 = f1_;
  uint64_t o2 = f2_;
  uint64_t o3 = f3_;
  b[0U] = o0;
  b[1U] = o1;
  b[2U] = o2;
  b[3U] = o3;
}

static void encode_point(uint8_t *o, uint64_t *i)
{
  uint64_t *x = i;
  uint64_t *z = i + 4U;
  uint64_t tmp[4U] = { 0U };
  uint64_t u64s[4U] = { 0U };
  uint64_t tmp_w[16U] = { 0U };
  finv(tmp, z, tmp_w);
  fmul0(tmp, tmp, x, tmp_w);
  store_felem(u64s, tmp);
  KRML_MAYBE_FOR4(i0, 0U, 4U, 1U, store64_le(o + i0 * 8U, u64s[i0]););
}

/**
Compute the scalar multiple of a point.

@param out Pointer to 32 bytes of memory, allocated by the caller, where the resulting point is written to.
@param priv Pointer to 32 bytes of memory where the secret/private key is read from.
@param pub Pointer to 32 bytes of memory where the public point is read from.
*/
void Hacl_Curve25519_64_scalarmult(uint8_t *out, uint8_t *priv, uint8_t *pub)
{
  uint64_t init[8U] = { 0U };
  uint64_t tmp[4U] = { 0U };
  KRML_MAYBE_FOR4(i,
    0U,
    4U,
    1U,
    uint64_t *os = tmp;
    uint8_t *bj = pub + i * 8U;
    uint64_t u = load64_le(bj);
    uint64_t r = u;
    uint64_t x = r;
    os[i] = x;);
  uint64_t tmp3 = tmp[3U];
  tmp[3U] = tmp3 & 0x7fffffffffffffffULL;
  uint64_t *x = init;
  uint64_t *z = init + 4U;
  z[0U] = 1ULL;
  z[1U] = 0ULL;
  z[2U] = 0ULL;
  z[3U] = 0ULL;
  x[0U] = tmp[0U];
  x[1U] = tmp[1U];
  x[2U] = tmp[2U];
  x[3U] = tmp[3U];
  montgomery_ladder(init, priv, init);
  encode_point(out, init);
}

/**
Calculate a public point from a secret/private key.

This computes a scalar multiplication of the secret/private key with the curve's basepoint.

@param pub Pointer to 32 bytes of memory, allocated by the caller, where the resulting point is written to.
@param priv Pointer to 32 bytes of memory where the secret/private key is read from.
*/
void Hacl_Curve25519_64_secret_to_public(uint8_t *pub, uint8_t *priv)
{
  uint8_t basepoint[32U] = { 0U };
  for (uint32_t i = 0U; i < 32U; i++)
  {
    uint8_t *os = basepoint;
    uint8_t x = g25519[i];
    os[i] = x;
  }
  Hacl_Curve25519_64_scalarmult(pub, priv, basepoint);
}

/**
Execute the diffie-hellmann key exchange.

@param out Pointer to 32 bytes of memory, allocated by the caller, where the resulting point is written to.
@param priv Pointer to 32 bytes of memory where **our** secret/private key is read from.
@param pub Pointer to 32 bytes of memory where **their** public point is read from.
*/
bool Hacl_Curve25519_64_ecdh(uint8_t *out, uint8_t *priv, uint8_t *pub)
{
  uint8_t zeros[32U] = { 0U };
  Hacl_Curve25519_64_scalarmult(out, priv, pub);
  uint8_t res = 255U;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    uint8_t uu____0 = FStar_UInt8_eq_mask(out[i], zeros[i]);
    res = (uint32_t)uu____0 & (uint32_t)res;
  }
  uint8_t z = res;
  bool r = z == 255U;
  return !r;
}

