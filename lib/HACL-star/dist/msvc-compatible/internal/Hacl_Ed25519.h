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


#ifndef __internal_Hacl_Ed25519_H
#define __internal_Hacl_Ed25519_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "internal/Hacl_Krmllib.h"
#include "internal/Hacl_Hash_SHA2.h"
#include "internal/Hacl_Ed25519_PrecompTable.h"
#include "internal/Hacl_Curve25519_51.h"
#include "internal/Hacl_Bignum_Base.h"
#include "internal/Hacl_Bignum25519_51.h"
#include "../Hacl_Ed25519.h"

void Hacl_Bignum25519_reduce_513(uint64_t *a);

void Hacl_Bignum25519_inverse(uint64_t *out, uint64_t *a);

void Hacl_Bignum25519_load_51(uint64_t *output, uint8_t *input);

void Hacl_Bignum25519_store_51(uint8_t *output, uint64_t *input);

void Hacl_Impl_Ed25519_PointDouble_point_double(uint64_t *out, uint64_t *p);

void Hacl_Impl_Ed25519_PointAdd_point_add(uint64_t *out, uint64_t *p, uint64_t *q);

void Hacl_Impl_Ed25519_PointConstants_make_point_inf(uint64_t *b);

bool Hacl_Impl_Ed25519_PointDecompress_point_decompress(uint64_t *out, uint8_t *s);

void Hacl_Impl_Ed25519_PointCompress_point_compress(uint8_t *z, uint64_t *p);

bool Hacl_Impl_Ed25519_PointEqual_point_equal(uint64_t *p, uint64_t *q);

void Hacl_Impl_Ed25519_PointNegate_point_negate(uint64_t *p, uint64_t *out);

void Hacl_Impl_Ed25519_Ladder_point_mul(uint64_t *out, uint8_t *scalar, uint64_t *q);

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Ed25519_H_DEFINED
#endif
