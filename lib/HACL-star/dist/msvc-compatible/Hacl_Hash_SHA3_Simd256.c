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


#include "Hacl_Hash_SHA3_Simd256.h"

#include "internal/Hacl_Hash_SHA3.h"

void
Hacl_Hash_SHA3_Simd256_absorb_inner_256(
  uint32_t rateInBytes,
  Hacl_Hash_SHA2_uint8_4p b,
  Lib_IntVector_Intrinsics_vec256 *s
)
{
  KRML_MAYBE_UNUSED_VAR(rateInBytes);
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b2 = b.snd.snd.fst;
  uint8_t *b1 = b.snd.fst;
  uint8_t *b0 = b.fst;
  ws[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0);
  ws[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1);
  ws[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2);
  ws[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3);
  ws[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 32U);
  ws[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 32U);
  ws[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 32U);
  ws[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 32U);
  ws[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 64U);
  ws[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 64U);
  ws[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 64U);
  ws[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 64U);
  ws[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 96U);
  ws[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 96U);
  ws[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 96U);
  ws[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 96U);
  ws[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 128U);
  ws[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 128U);
  ws[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 128U);
  ws[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 128U);
  ws[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 160U);
  ws[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 160U);
  ws[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 160U);
  ws[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 160U);
  ws[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 192U);
  ws[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 192U);
  ws[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 192U);
  ws[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 192U);
  ws[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b0 + 224U);
  ws[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b1 + 224U);
  ws[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b2 + 224U);
  ws[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b3 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__6;
  ws[0U] = ws0;
  ws[1U] = ws1;
  ws[2U] = ws2;
  ws[3U] = ws3;
  ws[4U] = ws4;
  ws[5U] = ws5;
  ws[6U] = ws6;
  ws[7U] = ws7;
  ws[8U] = ws8;
  ws[9U] = ws9;
  ws[10U] = ws10;
  ws[11U] = ws11;
  ws[12U] = ws12;
  ws[13U] = ws13;
  ws[14U] = ws14;
  ws[15U] = ws15;
  ws[16U] = ws16;
  ws[17U] = ws17;
  ws[18U] = ws18;
  ws[19U] = ws19;
  ws[20U] = ws20;
  ws[21U] = ws21;
  ws[22U] = ws22;
  ws[23U] = ws23;
  ws[24U] = ws24;
  ws[25U] = ws25;
  ws[26U] = ws26;
  ws[27U] = ws27;
  ws[28U] = ws28;
  ws[29U] = ws29;
  ws[30U] = ws30;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws[i]);
  }
  for (uint32_t i0 = 0U; i0 < 24U; i0++)
  {
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
      Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
      Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
      _C[i] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____0,
          Lib_IntVector_Intrinsics_vec256_xor(uu____1,
            Lib_IntVector_Intrinsics_vec256_xor(uu____2,
              Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
    KRML_MAYBE_FOR5(i1,
      0U,
      5U,
      1U,
      Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i1 + 4U) % 5U];
      Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i1 + 1U) % 5U];
      Lib_IntVector_Intrinsics_vec256
      _D =
        Lib_IntVector_Intrinsics_vec256_xor(uu____3,
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
              1U),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        s[i1 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i1 + 5U * i], _D);););
    Lib_IntVector_Intrinsics_vec256 x = s[1U];
    Lib_IntVector_Intrinsics_vec256 current = x;
    for (uint32_t i = 0U; i < 24U; i++)
    {
      uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
      uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
      Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
      Lib_IntVector_Intrinsics_vec256 uu____5 = current;
      s[_Y] =
        Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5, r),
          Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
      current = temp;
    }
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
      Lib_IntVector_Intrinsics_vec256
      uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
      Lib_IntVector_Intrinsics_vec256
      v07 =
        Lib_IntVector_Intrinsics_vec256_xor(uu____6,
          Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
      Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
      Lib_IntVector_Intrinsics_vec256
      uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
      Lib_IntVector_Intrinsics_vec256
      v17 =
        Lib_IntVector_Intrinsics_vec256_xor(uu____8,
          Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
      Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
      Lib_IntVector_Intrinsics_vec256
      uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
      Lib_IntVector_Intrinsics_vec256
      v27 =
        Lib_IntVector_Intrinsics_vec256_xor(uu____10,
          Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
      Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
      Lib_IntVector_Intrinsics_vec256
      uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
      Lib_IntVector_Intrinsics_vec256
      v37 =
        Lib_IntVector_Intrinsics_vec256_xor(uu____12,
          Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
      Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
      Lib_IntVector_Intrinsics_vec256
      uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
      Lib_IntVector_Intrinsics_vec256
      v4 =
        Lib_IntVector_Intrinsics_vec256_xor(uu____14,
          Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
      s[0U + 5U * i] = v07;
      s[1U + 5U * i] = v17;
      s[2U + 5U * i] = v27;
      s[3U + 5U * i] = v37;
      s[4U + 5U * i] = v4;);
    uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i0];
    Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
    s[0U] =
      Lib_IntVector_Intrinsics_vec256_xor(uu____16,
        Lib_IntVector_Intrinsics_vec256_load64(c));
  }
}

void
Hacl_Hash_SHA3_Simd256_shake128(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint32_t outputByteLen,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 168U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x1FU;
  b12[inputByteLen % rateInBytes1] = 0x1FU;
  b22[inputByteLen % rateInBytes1] = 0x1FU;
  b32[inputByteLen % rateInBytes1] = 0x1FU;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < outputByteLen / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = outputByteLen % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + outputByteLen - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + outputByteLen - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + outputByteLen - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + outputByteLen - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_Simd256_shake256(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint32_t outputByteLen,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 136U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x1FU;
  b12[inputByteLen % rateInBytes1] = 0x1FU;
  b22[inputByteLen % rateInBytes1] = 0x1FU;
  b32[inputByteLen % rateInBytes1] = 0x1FU;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < outputByteLen / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = outputByteLen % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + outputByteLen - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + outputByteLen - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + outputByteLen - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + outputByteLen - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_Simd256_sha3_224(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 144U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x06U;
  b12[inputByteLen % rateInBytes1] = 0x06U;
  b22[inputByteLen % rateInBytes1] = 0x06U;
  b32[inputByteLen % rateInBytes1] = 0x06U;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < 28U / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = 28U % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + 28U - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + 28U - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + 28U - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + 28U - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_Simd256_sha3_256(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 136U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x06U;
  b12[inputByteLen % rateInBytes1] = 0x06U;
  b22[inputByteLen % rateInBytes1] = 0x06U;
  b32[inputByteLen % rateInBytes1] = 0x06U;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < 32U / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = 32U % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + 32U - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + 32U - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + 32U - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + 32U - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_Simd256_sha3_384(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 104U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x06U;
  b12[inputByteLen % rateInBytes1] = 0x06U;
  b22[inputByteLen % rateInBytes1] = 0x06U;
  b32[inputByteLen % rateInBytes1] = 0x06U;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < 48U / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = 48U % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + 48U - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + 48U - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + 48U - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + 48U - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_Simd256_sha3_512(
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  Hacl_Hash_SHA2_uint8_4p
  ib = { .fst = input0, .snd = { .fst = input1, .snd = { .fst = input2, .snd = input3 } } };
  Hacl_Hash_SHA2_uint8_4p
  rb = { .fst = output0, .snd = { .fst = output1, .snd = { .fst = output2, .snd = output3 } } };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 s[25U] KRML_POST_ALIGN(32) = { 0U };
  uint32_t rateInBytes1 = 72U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b3 = ib.snd.snd.snd;
    uint8_t *b2 = ib.snd.snd.fst;
    uint8_t *b1 = ib.snd.fst;
    uint8_t *b0 = ib.fst;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl1, b1 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl2, b2 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    memcpy(bl3, b3 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b_, s);
  }
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b31 = ib.snd.snd.snd;
  uint8_t *b21 = ib.snd.snd.fst;
  uint8_t *b11 = ib.snd.fst;
  uint8_t *b01 = ib.fst;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % rateInBytes1] = 0x06U;
  b12[inputByteLen % rateInBytes1] = 0x06U;
  b22[inputByteLen % rateInBytes1] = 0x06U;
  b32[inputByteLen % rateInBytes1] = 0x06U;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws32[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws32[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws32[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws32[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws32[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws32[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws32[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws32[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws32[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws32[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws32[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws32[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws32[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws32[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws32[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws32[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws32[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws32[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws32[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws32[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws32[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws32[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws32[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws32[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws32[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws32[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws32[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws32[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws32[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws32[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws32[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws32[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws32[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws32[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws32[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws32[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws32[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws00 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws110 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws210 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws33 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws32[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws32[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws32[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws32[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws40 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws50 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws60 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws70 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws32[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws32[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws32[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws32[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws80 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws90 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws100 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws111 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws32[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws32[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws32[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws32[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws120 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws130 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws140 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws150 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws32[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws32[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws32[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws32[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws160 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws170 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws180 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws190 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws32[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws32[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws32[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws32[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws200 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws211 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws220 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws230 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws32[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws32[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws32[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws32[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws240 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws250 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws260 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws270 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v07 = ws32[28U];
  Lib_IntVector_Intrinsics_vec256 v17 = ws32[29U];
  Lib_IntVector_Intrinsics_vec256 v27 = ws32[30U];
  Lib_IntVector_Intrinsics_vec256 v37 = ws32[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v07, v17);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v27, v37);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws280 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws290 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws300 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws310 = v3__6;
  ws32[0U] = ws00;
  ws32[1U] = ws110;
  ws32[2U] = ws210;
  ws32[3U] = ws33;
  ws32[4U] = ws40;
  ws32[5U] = ws50;
  ws32[6U] = ws60;
  ws32[7U] = ws70;
  ws32[8U] = ws80;
  ws32[9U] = ws90;
  ws32[10U] = ws100;
  ws32[11U] = ws111;
  ws32[12U] = ws120;
  ws32[13U] = ws130;
  ws32[14U] = ws140;
  ws32[15U] = ws150;
  ws32[16U] = ws160;
  ws32[17U] = ws170;
  ws32[18U] = ws180;
  ws32[19U] = ws190;
  ws32[20U] = ws200;
  ws32[21U] = ws211;
  ws32[22U] = ws220;
  ws32[23U] = ws230;
  ws32[24U] = ws240;
  ws32[25U] = ws250;
  ws32[26U] = ws260;
  ws32[27U] = ws270;
  ws32[28U] = ws280;
  ws32[29U] = ws290;
  ws32[30U] = ws300;
  ws32[31U] = ws310;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = Lib_IntVector_Intrinsics_vec256_xor(s[i], ws32[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b25 = b.snd.snd.fst;
  uint8_t *b15 = b.snd.fst;
  uint8_t *b05 = b.fst;
  b05[rateInBytes1 - 1U] = 0x80U;
  b15[rateInBytes1 - 1U] = 0x80U;
  b25[rateInBytes1 - 1U] = 0x80U;
  b3[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(rateInBytes1, b, s);
  for (uint32_t i0 = 0U; i0 < 64U / rateInBytes1; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
    Lib_IntVector_Intrinsics_vec256
    v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
    Lib_IntVector_Intrinsics_vec256
    v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
    Lib_IntVector_Intrinsics_vec256
    v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256
    v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
    Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
    Lib_IntVector_Intrinsics_vec256
    v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
    Lib_IntVector_Intrinsics_vec256
    v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
    Lib_IntVector_Intrinsics_vec256
    v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256
    v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
    Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
    Lib_IntVector_Intrinsics_vec256
    v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
    Lib_IntVector_Intrinsics_vec256
    v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
    Lib_IntVector_Intrinsics_vec256
    v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256
    v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
    Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
    Lib_IntVector_Intrinsics_vec256
    v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
    Lib_IntVector_Intrinsics_vec256
    v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
    Lib_IntVector_Intrinsics_vec256
    v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256
    v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
    Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
    Lib_IntVector_Intrinsics_vec256
    v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
    Lib_IntVector_Intrinsics_vec256
    v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
    Lib_IntVector_Intrinsics_vec256
    v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256
    v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
    Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
    Lib_IntVector_Intrinsics_vec256
    v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
    Lib_IntVector_Intrinsics_vec256
    v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
    Lib_IntVector_Intrinsics_vec256
    v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256
    v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
    Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
    Lib_IntVector_Intrinsics_vec256
    v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
    Lib_IntVector_Intrinsics_vec256
    v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
    Lib_IntVector_Intrinsics_vec256
    v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256
    v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
    Lib_IntVector_Intrinsics_vec256
    v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256
    v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b35 = rb.snd.snd.snd;
    uint8_t *b2 = rb.snd.snd.fst;
    uint8_t *b1 = rb.snd.fst;
    uint8_t *b0 = rb.fst;
    memcpy(b0 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    memcpy(b1 + i0 * rateInBytes1, hbuf + 256U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b2 + i0 * rateInBytes1, hbuf + 512U, rateInBytes1 * sizeof (uint8_t));
    memcpy(b35 + i0 * rateInBytes1, hbuf + 768U, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = s[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = s[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = s[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(s[i + 15U], s[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          s[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(s[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = s[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = s[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        s[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = s[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(s[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v015 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, s[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = s[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(s[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v115 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, s[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = s[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(s[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v215 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, s[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = s[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(s[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v315 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, s[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = s[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(s[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, s[1U + 5U * i]));
        s[0U + 5U * i] = v015;
        s[1U + 5U * i] = v115;
        s[2U + 5U * i] = v215;
        s[3U + 5U * i] = v315;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = s[0U];
      s[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
  uint32_t remOut = 64U % rateInBytes1;
  uint8_t hbuf[1024U] = { 0U };
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  memcpy(ws, s, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  Lib_IntVector_Intrinsics_vec256 v08 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v18 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v28 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v38 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v1_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v08, v18);
  Lib_IntVector_Intrinsics_vec256
  v2_7 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v3_7 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v28, v38);
  Lib_IntVector_Intrinsics_vec256
  v0__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v1__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_7, v2_7);
  Lib_IntVector_Intrinsics_vec256
  v2__7 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256
  v3__7 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_7, v3_7);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__7;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__7;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__7;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__7;
  Lib_IntVector_Intrinsics_vec256 v09 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v19 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v29 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v39 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v1_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v09, v19);
  Lib_IntVector_Intrinsics_vec256
  v2_8 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v3_8 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v29, v39);
  Lib_IntVector_Intrinsics_vec256
  v0__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v1__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_8, v2_8);
  Lib_IntVector_Intrinsics_vec256
  v2__8 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256
  v3__8 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_8, v3_8);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__8;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__8;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__8;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__8;
  Lib_IntVector_Intrinsics_vec256 v010 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v110 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v210 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v310 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v1_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v010, v110);
  Lib_IntVector_Intrinsics_vec256
  v2_9 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v3_9 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v210, v310);
  Lib_IntVector_Intrinsics_vec256
  v0__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v1__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_9, v2_9);
  Lib_IntVector_Intrinsics_vec256
  v2__9 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256
  v3__9 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_9, v3_9);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__9;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__9;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__9;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__9;
  Lib_IntVector_Intrinsics_vec256 v011 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v111 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v211 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v311 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v1_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v011, v111);
  Lib_IntVector_Intrinsics_vec256
  v2_10 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v3_10 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v211, v311);
  Lib_IntVector_Intrinsics_vec256
  v0__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v1__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_10, v2_10);
  Lib_IntVector_Intrinsics_vec256
  v2__10 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256
  v3__10 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_10, v3_10);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__10;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__10;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__10;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__10;
  Lib_IntVector_Intrinsics_vec256 v012 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v112 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v212 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v312 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v1_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v012, v112);
  Lib_IntVector_Intrinsics_vec256
  v2_11 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v3_11 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v212, v312);
  Lib_IntVector_Intrinsics_vec256
  v0__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v1__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_11, v2_11);
  Lib_IntVector_Intrinsics_vec256
  v2__11 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256
  v3__11 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_11, v3_11);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__11;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__11;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__11;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__11;
  Lib_IntVector_Intrinsics_vec256 v013 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v113 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v213 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v313 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v1_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v013, v113);
  Lib_IntVector_Intrinsics_vec256
  v2_12 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v3_12 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v213, v313);
  Lib_IntVector_Intrinsics_vec256
  v0__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v1__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_12, v2_12);
  Lib_IntVector_Intrinsics_vec256
  v2__12 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256
  v3__12 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_12, v3_12);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__12;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__12;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__12;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__12;
  Lib_IntVector_Intrinsics_vec256 v014 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v114 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v214 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v314 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v1_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v014, v114);
  Lib_IntVector_Intrinsics_vec256
  v2_13 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v3_13 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v214, v314);
  Lib_IntVector_Intrinsics_vec256
  v0__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v1__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_13, v2_13);
  Lib_IntVector_Intrinsics_vec256
  v2__13 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256
  v3__13 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_13, v3_13);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__13;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__13;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__13;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__13;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_14 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_14 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v1__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_14, v2_14);
  Lib_IntVector_Intrinsics_vec256
  v2__14 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256
  v3__14 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_14, v3_14);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__14;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__14;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__14;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__14;
  ws[0U] = ws0;
  ws[1U] = ws4;
  ws[2U] = ws8;
  ws[3U] = ws12;
  ws[4U] = ws16;
  ws[5U] = ws20;
  ws[6U] = ws24;
  ws[7U] = ws28;
  ws[8U] = ws1;
  ws[9U] = ws5;
  ws[10U] = ws9;
  ws[11U] = ws13;
  ws[12U] = ws17;
  ws[13U] = ws21;
  ws[14U] = ws25;
  ws[15U] = ws29;
  ws[16U] = ws2;
  ws[17U] = ws6;
  ws[18U] = ws10;
  ws[19U] = ws14;
  ws[20U] = ws18;
  ws[21U] = ws22;
  ws[22U] = ws26;
  ws[23U] = ws30;
  ws[24U] = ws3;
  ws[25U] = ws7;
  ws[26U] = ws11;
  ws[27U] = ws15;
  ws[28U] = ws19;
  ws[29U] = ws23;
  ws[30U] = ws27;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 32U; i++)
  {
    Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
  }
  uint8_t *b35 = rb.snd.snd.snd;
  uint8_t *b2 = rb.snd.snd.fst;
  uint8_t *b1 = rb.snd.fst;
  uint8_t *b0 = rb.fst;
  memcpy(b0 + 64U - remOut, hbuf, remOut * sizeof (uint8_t));
  memcpy(b1 + 64U - remOut, hbuf + 256U, remOut * sizeof (uint8_t));
  memcpy(b2 + 64U - remOut, hbuf + 512U, remOut * sizeof (uint8_t));
  memcpy(b35 + 64U - remOut, hbuf + 768U, remOut * sizeof (uint8_t));
}

/**
Allocate quadruple state buffer (200-bytes for each)
*/
Lib_IntVector_Intrinsics_vec256 *Hacl_Hash_SHA3_Simd256_state_malloc(void)
{
  Lib_IntVector_Intrinsics_vec256
  *buf =
    (Lib_IntVector_Intrinsics_vec256 *)KRML_ALIGNED_MALLOC(32,
      sizeof (Lib_IntVector_Intrinsics_vec256) * 25U);
  memset(buf, 0U, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
  return buf;
}

/**
Free quadruple state buffer
*/
void Hacl_Hash_SHA3_Simd256_state_free(Lib_IntVector_Intrinsics_vec256 *s)
{
  KRML_ALIGNED_FREE(s);
}

/**
Absorb number of blocks of 4 input buffers and write the output states

  This function is intended to receive a quadruple hash state and 4 input buffers.
  It processes an inputs of multiple of 168-bytes (SHAKE128 block size),
  any additional bytes of final partial block for each buffer are ignored.

  The argument `state` (IN/OUT) points to quadruple hash state,
  i.e., Lib_IntVector_Intrinsics_vec256[25]
  The arguments `input0/input1/input2/input3` (IN) point to `inputByteLen` bytes
  of valid memory for each buffer, i.e., uint8_t[inputByteLen]
*/
void
Hacl_Hash_SHA3_Simd256_shake128_absorb_nblocks(
  Lib_IntVector_Intrinsics_vec256 *state,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  for (uint32_t i = 0U; i < inputByteLen / 168U; i++)
  {
    uint8_t b00[256U] = { 0U };
    uint8_t b10[256U] = { 0U };
    uint8_t b20[256U] = { 0U };
    uint8_t b30[256U] = { 0U };
    Hacl_Hash_SHA2_uint8_4p
    b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
    uint8_t *b0 = input0;
    uint8_t *b1 = input1;
    uint8_t *b2 = input2;
    uint8_t *b3 = input3;
    uint8_t *bl3 = b_.snd.snd.snd;
    uint8_t *bl2 = b_.snd.snd.fst;
    uint8_t *bl1 = b_.snd.fst;
    uint8_t *bl0 = b_.fst;
    memcpy(bl0, b0 + i * 168U, 168U * sizeof (uint8_t));
    memcpy(bl1, b1 + i * 168U, 168U * sizeof (uint8_t));
    memcpy(bl2, b2 + i * 168U, 168U * sizeof (uint8_t));
    memcpy(bl3, b3 + i * 168U, 168U * sizeof (uint8_t));
    Hacl_Hash_SHA3_Simd256_absorb_inner_256(168U, b_, state);
  }
}

/**
Absorb a final partial blocks of 4 input buffers and write the output states

  This function is intended to receive a quadruple hash state and 4 input buffers.
  It processes a sequence of bytes at end of each input buffer that is less
  than 168-bytes (SHAKE128 block size),
  any bytes of full blocks at start of input buffers are ignored.

  The argument `state` (IN/OUT) points to quadruple hash state,
  i.e., Lib_IntVector_Intrinsics_vec256[25]
  The arguments `input0/input1/input2/input3` (IN) point to `inputByteLen` bytes
  of valid memory for each buffer, i.e., uint8_t[inputByteLen]

  Note: Full size of input buffers must be passed to `inputByteLen` including
  the number of full-block bytes at start of each input buffer that are ignored
*/
void
Hacl_Hash_SHA3_Simd256_shake128_absorb_final(
  Lib_IntVector_Intrinsics_vec256 *state,
  uint8_t *input0,
  uint8_t *input1,
  uint8_t *input2,
  uint8_t *input3,
  uint32_t inputByteLen
)
{
  uint8_t b00[256U] = { 0U };
  uint8_t b10[256U] = { 0U };
  uint8_t b20[256U] = { 0U };
  uint8_t b30[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b_ = { .fst = b00, .snd = { .fst = b10, .snd = { .fst = b20, .snd = b30 } } };
  uint32_t rem = inputByteLen % 168U;
  uint8_t *b01 = input0;
  uint8_t *b11 = input1;
  uint8_t *b21 = input2;
  uint8_t *b31 = input3;
  uint8_t *bl3 = b_.snd.snd.snd;
  uint8_t *bl2 = b_.snd.snd.fst;
  uint8_t *bl1 = b_.snd.fst;
  uint8_t *bl0 = b_.fst;
  memcpy(bl0, b01 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl1, b11 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl2, b21 + inputByteLen - rem, rem * sizeof (uint8_t));
  memcpy(bl3, b31 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b32 = b_.snd.snd.snd;
  uint8_t *b22 = b_.snd.snd.fst;
  uint8_t *b12 = b_.snd.fst;
  uint8_t *b02 = b_.fst;
  b02[inputByteLen % 168U] = 0x1FU;
  b12[inputByteLen % 168U] = 0x1FU;
  b22[inputByteLen % 168U] = 0x1FU;
  b32[inputByteLen % 168U] = 0x1FU;
  KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
  uint8_t *b33 = b_.snd.snd.snd;
  uint8_t *b23 = b_.snd.snd.fst;
  uint8_t *b13 = b_.snd.fst;
  uint8_t *b03 = b_.fst;
  ws[0U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03);
  ws[1U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13);
  ws[2U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23);
  ws[3U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33);
  ws[4U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 32U);
  ws[5U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 32U);
  ws[6U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 32U);
  ws[7U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 32U);
  ws[8U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 64U);
  ws[9U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 64U);
  ws[10U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 64U);
  ws[11U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 64U);
  ws[12U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 96U);
  ws[13U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 96U);
  ws[14U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 96U);
  ws[15U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 96U);
  ws[16U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 128U);
  ws[17U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 128U);
  ws[18U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 128U);
  ws[19U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 128U);
  ws[20U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 160U);
  ws[21U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 160U);
  ws[22U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 160U);
  ws[23U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 160U);
  ws[24U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 192U);
  ws[25U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 192U);
  ws[26U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 192U);
  ws[27U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 192U);
  ws[28U] = Lib_IntVector_Intrinsics_vec256_load64_le(b03 + 224U);
  ws[29U] = Lib_IntVector_Intrinsics_vec256_load64_le(b13 + 224U);
  ws[30U] = Lib_IntVector_Intrinsics_vec256_load64_le(b23 + 224U);
  ws[31U] = Lib_IntVector_Intrinsics_vec256_load64_le(b33 + 224U);
  Lib_IntVector_Intrinsics_vec256 v00 = ws[0U];
  Lib_IntVector_Intrinsics_vec256 v10 = ws[1U];
  Lib_IntVector_Intrinsics_vec256 v20 = ws[2U];
  Lib_IntVector_Intrinsics_vec256 v30 = ws[3U];
  Lib_IntVector_Intrinsics_vec256
  v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
  Lib_IntVector_Intrinsics_vec256
  v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
  Lib_IntVector_Intrinsics_vec256
  v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
  Lib_IntVector_Intrinsics_vec256
  v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256
  v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
  Lib_IntVector_Intrinsics_vec256 ws0 = v0__;
  Lib_IntVector_Intrinsics_vec256 ws1 = v2__;
  Lib_IntVector_Intrinsics_vec256 ws2 = v1__;
  Lib_IntVector_Intrinsics_vec256 ws3 = v3__;
  Lib_IntVector_Intrinsics_vec256 v01 = ws[4U];
  Lib_IntVector_Intrinsics_vec256 v11 = ws[5U];
  Lib_IntVector_Intrinsics_vec256 v21 = ws[6U];
  Lib_IntVector_Intrinsics_vec256 v31 = ws[7U];
  Lib_IntVector_Intrinsics_vec256
  v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
  Lib_IntVector_Intrinsics_vec256
  v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
  Lib_IntVector_Intrinsics_vec256
  v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
  Lib_IntVector_Intrinsics_vec256
  v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256
  v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
  Lib_IntVector_Intrinsics_vec256 ws4 = v0__0;
  Lib_IntVector_Intrinsics_vec256 ws5 = v2__0;
  Lib_IntVector_Intrinsics_vec256 ws6 = v1__0;
  Lib_IntVector_Intrinsics_vec256 ws7 = v3__0;
  Lib_IntVector_Intrinsics_vec256 v02 = ws[8U];
  Lib_IntVector_Intrinsics_vec256 v12 = ws[9U];
  Lib_IntVector_Intrinsics_vec256 v22 = ws[10U];
  Lib_IntVector_Intrinsics_vec256 v32 = ws[11U];
  Lib_IntVector_Intrinsics_vec256
  v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
  Lib_IntVector_Intrinsics_vec256
  v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
  Lib_IntVector_Intrinsics_vec256
  v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
  Lib_IntVector_Intrinsics_vec256
  v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256
  v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
  Lib_IntVector_Intrinsics_vec256 ws8 = v0__1;
  Lib_IntVector_Intrinsics_vec256 ws9 = v2__1;
  Lib_IntVector_Intrinsics_vec256 ws10 = v1__1;
  Lib_IntVector_Intrinsics_vec256 ws11 = v3__1;
  Lib_IntVector_Intrinsics_vec256 v03 = ws[12U];
  Lib_IntVector_Intrinsics_vec256 v13 = ws[13U];
  Lib_IntVector_Intrinsics_vec256 v23 = ws[14U];
  Lib_IntVector_Intrinsics_vec256 v33 = ws[15U];
  Lib_IntVector_Intrinsics_vec256
  v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
  Lib_IntVector_Intrinsics_vec256
  v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
  Lib_IntVector_Intrinsics_vec256
  v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
  Lib_IntVector_Intrinsics_vec256
  v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256
  v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
  Lib_IntVector_Intrinsics_vec256 ws12 = v0__2;
  Lib_IntVector_Intrinsics_vec256 ws13 = v2__2;
  Lib_IntVector_Intrinsics_vec256 ws14 = v1__2;
  Lib_IntVector_Intrinsics_vec256 ws15 = v3__2;
  Lib_IntVector_Intrinsics_vec256 v04 = ws[16U];
  Lib_IntVector_Intrinsics_vec256 v14 = ws[17U];
  Lib_IntVector_Intrinsics_vec256 v24 = ws[18U];
  Lib_IntVector_Intrinsics_vec256 v34 = ws[19U];
  Lib_IntVector_Intrinsics_vec256
  v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
  Lib_IntVector_Intrinsics_vec256
  v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
  Lib_IntVector_Intrinsics_vec256
  v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
  Lib_IntVector_Intrinsics_vec256
  v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256
  v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
  Lib_IntVector_Intrinsics_vec256 ws16 = v0__3;
  Lib_IntVector_Intrinsics_vec256 ws17 = v2__3;
  Lib_IntVector_Intrinsics_vec256 ws18 = v1__3;
  Lib_IntVector_Intrinsics_vec256 ws19 = v3__3;
  Lib_IntVector_Intrinsics_vec256 v05 = ws[20U];
  Lib_IntVector_Intrinsics_vec256 v15 = ws[21U];
  Lib_IntVector_Intrinsics_vec256 v25 = ws[22U];
  Lib_IntVector_Intrinsics_vec256 v35 = ws[23U];
  Lib_IntVector_Intrinsics_vec256
  v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
  Lib_IntVector_Intrinsics_vec256
  v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
  Lib_IntVector_Intrinsics_vec256
  v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
  Lib_IntVector_Intrinsics_vec256
  v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256
  v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
  Lib_IntVector_Intrinsics_vec256 ws20 = v0__4;
  Lib_IntVector_Intrinsics_vec256 ws21 = v2__4;
  Lib_IntVector_Intrinsics_vec256 ws22 = v1__4;
  Lib_IntVector_Intrinsics_vec256 ws23 = v3__4;
  Lib_IntVector_Intrinsics_vec256 v06 = ws[24U];
  Lib_IntVector_Intrinsics_vec256 v16 = ws[25U];
  Lib_IntVector_Intrinsics_vec256 v26 = ws[26U];
  Lib_IntVector_Intrinsics_vec256 v36 = ws[27U];
  Lib_IntVector_Intrinsics_vec256
  v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
  Lib_IntVector_Intrinsics_vec256
  v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
  Lib_IntVector_Intrinsics_vec256
  v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
  Lib_IntVector_Intrinsics_vec256
  v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256
  v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
  Lib_IntVector_Intrinsics_vec256 ws24 = v0__5;
  Lib_IntVector_Intrinsics_vec256 ws25 = v2__5;
  Lib_IntVector_Intrinsics_vec256 ws26 = v1__5;
  Lib_IntVector_Intrinsics_vec256 ws27 = v3__5;
  Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
  Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
  Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
  Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
  Lib_IntVector_Intrinsics_vec256
  v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
  Lib_IntVector_Intrinsics_vec256
  v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
  Lib_IntVector_Intrinsics_vec256
  v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
  Lib_IntVector_Intrinsics_vec256
  v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256
  v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
  Lib_IntVector_Intrinsics_vec256 ws28 = v0__6;
  Lib_IntVector_Intrinsics_vec256 ws29 = v2__6;
  Lib_IntVector_Intrinsics_vec256 ws30 = v1__6;
  Lib_IntVector_Intrinsics_vec256 ws31 = v3__6;
  ws[0U] = ws0;
  ws[1U] = ws1;
  ws[2U] = ws2;
  ws[3U] = ws3;
  ws[4U] = ws4;
  ws[5U] = ws5;
  ws[6U] = ws6;
  ws[7U] = ws7;
  ws[8U] = ws8;
  ws[9U] = ws9;
  ws[10U] = ws10;
  ws[11U] = ws11;
  ws[12U] = ws12;
  ws[13U] = ws13;
  ws[14U] = ws14;
  ws[15U] = ws15;
  ws[16U] = ws16;
  ws[17U] = ws17;
  ws[18U] = ws18;
  ws[19U] = ws19;
  ws[20U] = ws20;
  ws[21U] = ws21;
  ws[22U] = ws22;
  ws[23U] = ws23;
  ws[24U] = ws24;
  ws[25U] = ws25;
  ws[26U] = ws26;
  ws[27U] = ws27;
  ws[28U] = ws28;
  ws[29U] = ws29;
  ws[30U] = ws30;
  ws[31U] = ws31;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    state[i] = Lib_IntVector_Intrinsics_vec256_xor(state[i], ws[i]);
  }
  uint8_t b04[256U] = { 0U };
  uint8_t b14[256U] = { 0U };
  uint8_t b24[256U] = { 0U };
  uint8_t b34[256U] = { 0U };
  Hacl_Hash_SHA2_uint8_4p
  b = { .fst = b04, .snd = { .fst = b14, .snd = { .fst = b24, .snd = b34 } } };
  uint8_t *b3 = b.snd.snd.snd;
  uint8_t *b2 = b.snd.snd.fst;
  uint8_t *b1 = b.snd.fst;
  uint8_t *b0 = b.fst;
  b0[167U] = 0x80U;
  b1[167U] = 0x80U;
  b2[167U] = 0x80U;
  b3[167U] = 0x80U;
  Hacl_Hash_SHA3_Simd256_absorb_inner_256(168U, b, state);
}

/**
Squeeze a quadruple hash state to 4 output buffers

  This function is intended to receive a quadruple hash state and 4 output buffers.
  It produces 4 outputs, each is multiple of 168-bytes (SHAKE128 block size),
  any additional bytes of final partial block for each buffer are ignored.

  The argument `state` (IN) points to quadruple hash state,
  i.e., Lib_IntVector_Intrinsics_vec256[25]
  The arguments `output0/output1/output2/output3` (OUT) point to `outputByteLen` bytes
  of valid memory for each buffer, i.e., uint8_t[inputByteLen]
*/
void
Hacl_Hash_SHA3_Simd256_shake128_squeeze_nblocks(
  Lib_IntVector_Intrinsics_vec256 *state,
  uint8_t *output0,
  uint8_t *output1,
  uint8_t *output2,
  uint8_t *output3,
  uint32_t outputByteLen
)
{
  for (uint32_t i0 = 0U; i0 < outputByteLen / 168U; i0++)
  {
    uint8_t hbuf[1024U] = { 0U };
    KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 ws[32U] KRML_POST_ALIGN(32) = { 0U };
    memcpy(ws, state, 25U * sizeof (Lib_IntVector_Intrinsics_vec256));
    Lib_IntVector_Intrinsics_vec256 v00 = ws[0U];
    Lib_IntVector_Intrinsics_vec256 v10 = ws[1U];
    Lib_IntVector_Intrinsics_vec256 v20 = ws[2U];
    Lib_IntVector_Intrinsics_vec256 v30 = ws[3U];
    Lib_IntVector_Intrinsics_vec256
    v0_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v00, v10);
    Lib_IntVector_Intrinsics_vec256
    v1_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v00, v10);
    Lib_IntVector_Intrinsics_vec256
    v2_ = Lib_IntVector_Intrinsics_vec256_interleave_low64(v20, v30);
    Lib_IntVector_Intrinsics_vec256
    v3_ = Lib_IntVector_Intrinsics_vec256_interleave_high64(v20, v30);
    Lib_IntVector_Intrinsics_vec256
    v0__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_, v2_);
    Lib_IntVector_Intrinsics_vec256
    v1__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_, v2_);
    Lib_IntVector_Intrinsics_vec256
    v2__ = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_, v3_);
    Lib_IntVector_Intrinsics_vec256
    v3__ = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_, v3_);
    Lib_IntVector_Intrinsics_vec256 ws0 = v0__;
    Lib_IntVector_Intrinsics_vec256 ws1 = v2__;
    Lib_IntVector_Intrinsics_vec256 ws2 = v1__;
    Lib_IntVector_Intrinsics_vec256 ws3 = v3__;
    Lib_IntVector_Intrinsics_vec256 v01 = ws[4U];
    Lib_IntVector_Intrinsics_vec256 v11 = ws[5U];
    Lib_IntVector_Intrinsics_vec256 v21 = ws[6U];
    Lib_IntVector_Intrinsics_vec256 v31 = ws[7U];
    Lib_IntVector_Intrinsics_vec256
    v0_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v01, v11);
    Lib_IntVector_Intrinsics_vec256
    v1_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v01, v11);
    Lib_IntVector_Intrinsics_vec256
    v2_0 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v21, v31);
    Lib_IntVector_Intrinsics_vec256
    v3_0 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v21, v31);
    Lib_IntVector_Intrinsics_vec256
    v0__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_0, v2_0);
    Lib_IntVector_Intrinsics_vec256
    v1__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_0, v2_0);
    Lib_IntVector_Intrinsics_vec256
    v2__0 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_0, v3_0);
    Lib_IntVector_Intrinsics_vec256
    v3__0 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_0, v3_0);
    Lib_IntVector_Intrinsics_vec256 ws4 = v0__0;
    Lib_IntVector_Intrinsics_vec256 ws5 = v2__0;
    Lib_IntVector_Intrinsics_vec256 ws6 = v1__0;
    Lib_IntVector_Intrinsics_vec256 ws7 = v3__0;
    Lib_IntVector_Intrinsics_vec256 v02 = ws[8U];
    Lib_IntVector_Intrinsics_vec256 v12 = ws[9U];
    Lib_IntVector_Intrinsics_vec256 v22 = ws[10U];
    Lib_IntVector_Intrinsics_vec256 v32 = ws[11U];
    Lib_IntVector_Intrinsics_vec256
    v0_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v02, v12);
    Lib_IntVector_Intrinsics_vec256
    v1_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v02, v12);
    Lib_IntVector_Intrinsics_vec256
    v2_1 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v22, v32);
    Lib_IntVector_Intrinsics_vec256
    v3_1 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v22, v32);
    Lib_IntVector_Intrinsics_vec256
    v0__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_1, v2_1);
    Lib_IntVector_Intrinsics_vec256
    v1__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_1, v2_1);
    Lib_IntVector_Intrinsics_vec256
    v2__1 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_1, v3_1);
    Lib_IntVector_Intrinsics_vec256
    v3__1 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_1, v3_1);
    Lib_IntVector_Intrinsics_vec256 ws8 = v0__1;
    Lib_IntVector_Intrinsics_vec256 ws9 = v2__1;
    Lib_IntVector_Intrinsics_vec256 ws10 = v1__1;
    Lib_IntVector_Intrinsics_vec256 ws11 = v3__1;
    Lib_IntVector_Intrinsics_vec256 v03 = ws[12U];
    Lib_IntVector_Intrinsics_vec256 v13 = ws[13U];
    Lib_IntVector_Intrinsics_vec256 v23 = ws[14U];
    Lib_IntVector_Intrinsics_vec256 v33 = ws[15U];
    Lib_IntVector_Intrinsics_vec256
    v0_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v03, v13);
    Lib_IntVector_Intrinsics_vec256
    v1_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v03, v13);
    Lib_IntVector_Intrinsics_vec256
    v2_2 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v23, v33);
    Lib_IntVector_Intrinsics_vec256
    v3_2 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v23, v33);
    Lib_IntVector_Intrinsics_vec256
    v0__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_2, v2_2);
    Lib_IntVector_Intrinsics_vec256
    v1__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_2, v2_2);
    Lib_IntVector_Intrinsics_vec256
    v2__2 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_2, v3_2);
    Lib_IntVector_Intrinsics_vec256
    v3__2 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_2, v3_2);
    Lib_IntVector_Intrinsics_vec256 ws12 = v0__2;
    Lib_IntVector_Intrinsics_vec256 ws13 = v2__2;
    Lib_IntVector_Intrinsics_vec256 ws14 = v1__2;
    Lib_IntVector_Intrinsics_vec256 ws15 = v3__2;
    Lib_IntVector_Intrinsics_vec256 v04 = ws[16U];
    Lib_IntVector_Intrinsics_vec256 v14 = ws[17U];
    Lib_IntVector_Intrinsics_vec256 v24 = ws[18U];
    Lib_IntVector_Intrinsics_vec256 v34 = ws[19U];
    Lib_IntVector_Intrinsics_vec256
    v0_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v04, v14);
    Lib_IntVector_Intrinsics_vec256
    v1_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v04, v14);
    Lib_IntVector_Intrinsics_vec256
    v2_3 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v24, v34);
    Lib_IntVector_Intrinsics_vec256
    v3_3 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v24, v34);
    Lib_IntVector_Intrinsics_vec256
    v0__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_3, v2_3);
    Lib_IntVector_Intrinsics_vec256
    v1__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_3, v2_3);
    Lib_IntVector_Intrinsics_vec256
    v2__3 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_3, v3_3);
    Lib_IntVector_Intrinsics_vec256
    v3__3 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_3, v3_3);
    Lib_IntVector_Intrinsics_vec256 ws16 = v0__3;
    Lib_IntVector_Intrinsics_vec256 ws17 = v2__3;
    Lib_IntVector_Intrinsics_vec256 ws18 = v1__3;
    Lib_IntVector_Intrinsics_vec256 ws19 = v3__3;
    Lib_IntVector_Intrinsics_vec256 v05 = ws[20U];
    Lib_IntVector_Intrinsics_vec256 v15 = ws[21U];
    Lib_IntVector_Intrinsics_vec256 v25 = ws[22U];
    Lib_IntVector_Intrinsics_vec256 v35 = ws[23U];
    Lib_IntVector_Intrinsics_vec256
    v0_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v05, v15);
    Lib_IntVector_Intrinsics_vec256
    v1_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v05, v15);
    Lib_IntVector_Intrinsics_vec256
    v2_4 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v25, v35);
    Lib_IntVector_Intrinsics_vec256
    v3_4 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v25, v35);
    Lib_IntVector_Intrinsics_vec256
    v0__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_4, v2_4);
    Lib_IntVector_Intrinsics_vec256
    v1__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_4, v2_4);
    Lib_IntVector_Intrinsics_vec256
    v2__4 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_4, v3_4);
    Lib_IntVector_Intrinsics_vec256
    v3__4 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_4, v3_4);
    Lib_IntVector_Intrinsics_vec256 ws20 = v0__4;
    Lib_IntVector_Intrinsics_vec256 ws21 = v2__4;
    Lib_IntVector_Intrinsics_vec256 ws22 = v1__4;
    Lib_IntVector_Intrinsics_vec256 ws23 = v3__4;
    Lib_IntVector_Intrinsics_vec256 v06 = ws[24U];
    Lib_IntVector_Intrinsics_vec256 v16 = ws[25U];
    Lib_IntVector_Intrinsics_vec256 v26 = ws[26U];
    Lib_IntVector_Intrinsics_vec256 v36 = ws[27U];
    Lib_IntVector_Intrinsics_vec256
    v0_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v06, v16);
    Lib_IntVector_Intrinsics_vec256
    v1_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v06, v16);
    Lib_IntVector_Intrinsics_vec256
    v2_5 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v26, v36);
    Lib_IntVector_Intrinsics_vec256
    v3_5 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v26, v36);
    Lib_IntVector_Intrinsics_vec256
    v0__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_5, v2_5);
    Lib_IntVector_Intrinsics_vec256
    v1__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_5, v2_5);
    Lib_IntVector_Intrinsics_vec256
    v2__5 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_5, v3_5);
    Lib_IntVector_Intrinsics_vec256
    v3__5 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_5, v3_5);
    Lib_IntVector_Intrinsics_vec256 ws24 = v0__5;
    Lib_IntVector_Intrinsics_vec256 ws25 = v2__5;
    Lib_IntVector_Intrinsics_vec256 ws26 = v1__5;
    Lib_IntVector_Intrinsics_vec256 ws27 = v3__5;
    Lib_IntVector_Intrinsics_vec256 v0 = ws[28U];
    Lib_IntVector_Intrinsics_vec256 v1 = ws[29U];
    Lib_IntVector_Intrinsics_vec256 v2 = ws[30U];
    Lib_IntVector_Intrinsics_vec256 v3 = ws[31U];
    Lib_IntVector_Intrinsics_vec256
    v0_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v1_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v0, v1);
    Lib_IntVector_Intrinsics_vec256
    v2_6 = Lib_IntVector_Intrinsics_vec256_interleave_low64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v3_6 = Lib_IntVector_Intrinsics_vec256_interleave_high64(v2, v3);
    Lib_IntVector_Intrinsics_vec256
    v0__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v0_6, v2_6);
    Lib_IntVector_Intrinsics_vec256
    v1__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v0_6, v2_6);
    Lib_IntVector_Intrinsics_vec256
    v2__6 = Lib_IntVector_Intrinsics_vec256_interleave_low128(v1_6, v3_6);
    Lib_IntVector_Intrinsics_vec256
    v3__6 = Lib_IntVector_Intrinsics_vec256_interleave_high128(v1_6, v3_6);
    Lib_IntVector_Intrinsics_vec256 ws28 = v0__6;
    Lib_IntVector_Intrinsics_vec256 ws29 = v2__6;
    Lib_IntVector_Intrinsics_vec256 ws30 = v1__6;
    Lib_IntVector_Intrinsics_vec256 ws31 = v3__6;
    ws[0U] = ws0;
    ws[1U] = ws4;
    ws[2U] = ws8;
    ws[3U] = ws12;
    ws[4U] = ws16;
    ws[5U] = ws20;
    ws[6U] = ws24;
    ws[7U] = ws28;
    ws[8U] = ws1;
    ws[9U] = ws5;
    ws[10U] = ws9;
    ws[11U] = ws13;
    ws[12U] = ws17;
    ws[13U] = ws21;
    ws[14U] = ws25;
    ws[15U] = ws29;
    ws[16U] = ws2;
    ws[17U] = ws6;
    ws[18U] = ws10;
    ws[19U] = ws14;
    ws[20U] = ws18;
    ws[21U] = ws22;
    ws[22U] = ws26;
    ws[23U] = ws30;
    ws[24U] = ws3;
    ws[25U] = ws7;
    ws[26U] = ws11;
    ws[27U] = ws15;
    ws[28U] = ws19;
    ws[29U] = ws23;
    ws[30U] = ws27;
    ws[31U] = ws31;
    for (uint32_t i = 0U; i < 32U; i++)
    {
      Lib_IntVector_Intrinsics_vec256_store64_le(hbuf + i * 32U, ws[i]);
    }
    uint8_t *b0 = output0;
    uint8_t *b1 = output1;
    uint8_t *b2 = output2;
    uint8_t *b3 = output3;
    memcpy(b0 + i0 * 168U, hbuf, 168U * sizeof (uint8_t));
    memcpy(b1 + i0 * 168U, hbuf + 256U, 168U * sizeof (uint8_t));
    memcpy(b2 + i0 * 168U, hbuf + 512U, 168U * sizeof (uint8_t));
    memcpy(b3 + i0 * 168U, hbuf + 768U, 168U * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      KRML_PRE_ALIGN(32) Lib_IntVector_Intrinsics_vec256 _C[5U] KRML_POST_ALIGN(32) = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____0 = state[i + 0U];
        Lib_IntVector_Intrinsics_vec256 uu____1 = state[i + 5U];
        Lib_IntVector_Intrinsics_vec256 uu____2 = state[i + 10U];
        _C[i] =
          Lib_IntVector_Intrinsics_vec256_xor(uu____0,
            Lib_IntVector_Intrinsics_vec256_xor(uu____1,
              Lib_IntVector_Intrinsics_vec256_xor(uu____2,
                Lib_IntVector_Intrinsics_vec256_xor(state[i + 15U], state[i + 20U])))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____3 = _C[(i2 + 4U) % 5U];
        Lib_IntVector_Intrinsics_vec256 uu____4 = _C[(i2 + 1U) % 5U];
        Lib_IntVector_Intrinsics_vec256
        _D =
          Lib_IntVector_Intrinsics_vec256_xor(uu____3,
            Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____4,
                1U),
              Lib_IntVector_Intrinsics_vec256_shift_right64(uu____4, 63U)));
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          state[i2 + 5U * i] = Lib_IntVector_Intrinsics_vec256_xor(state[i2 + 5U * i], _D);););
      Lib_IntVector_Intrinsics_vec256 x = state[1U];
      Lib_IntVector_Intrinsics_vec256 current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        Lib_IntVector_Intrinsics_vec256 temp = state[_Y];
        Lib_IntVector_Intrinsics_vec256 uu____5 = current;
        state[_Y] =
          Lib_IntVector_Intrinsics_vec256_or(Lib_IntVector_Intrinsics_vec256_shift_left64(uu____5,
              r),
            Lib_IntVector_Intrinsics_vec256_shift_right64(uu____5, 64U - r));
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        Lib_IntVector_Intrinsics_vec256 uu____6 = state[0U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____7 = Lib_IntVector_Intrinsics_vec256_lognot(state[1U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v07 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____6,
            Lib_IntVector_Intrinsics_vec256_and(uu____7, state[2U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____8 = state[1U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____9 = Lib_IntVector_Intrinsics_vec256_lognot(state[2U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v17 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____8,
            Lib_IntVector_Intrinsics_vec256_and(uu____9, state[3U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____10 = state[2U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____11 = Lib_IntVector_Intrinsics_vec256_lognot(state[3U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v27 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____10,
            Lib_IntVector_Intrinsics_vec256_and(uu____11, state[4U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____12 = state[3U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____13 = Lib_IntVector_Intrinsics_vec256_lognot(state[4U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v37 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____12,
            Lib_IntVector_Intrinsics_vec256_and(uu____13, state[0U + 5U * i]));
        Lib_IntVector_Intrinsics_vec256 uu____14 = state[4U + 5U * i];
        Lib_IntVector_Intrinsics_vec256
        uu____15 = Lib_IntVector_Intrinsics_vec256_lognot(state[0U + 5U * i]);
        Lib_IntVector_Intrinsics_vec256
        v4 =
          Lib_IntVector_Intrinsics_vec256_xor(uu____14,
            Lib_IntVector_Intrinsics_vec256_and(uu____15, state[1U + 5U * i]));
        state[0U + 5U * i] = v07;
        state[1U + 5U * i] = v17;
        state[2U + 5U * i] = v27;
        state[3U + 5U * i] = v37;
        state[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      Lib_IntVector_Intrinsics_vec256 uu____16 = state[0U];
      state[0U] =
        Lib_IntVector_Intrinsics_vec256_xor(uu____16,
          Lib_IntVector_Intrinsics_vec256_load64(c));
    }
  }
}

