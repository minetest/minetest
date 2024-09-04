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


#ifndef __internal_Hacl_Krmllib_H
#define __internal_Hacl_Krmllib_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "../Hacl_Krmllib.h"

static KRML_NOINLINE uint32_t FStar_UInt32_eq_mask(uint32_t a, uint32_t b);

static KRML_NOINLINE uint32_t FStar_UInt32_gte_mask(uint32_t a, uint32_t b);

static KRML_NOINLINE uint8_t FStar_UInt8_eq_mask(uint8_t a, uint8_t b);

static KRML_NOINLINE uint16_t FStar_UInt16_eq_mask(uint16_t a, uint16_t b);

static inline FStar_UInt128_uint128
FStar_UInt128_add(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b);

static inline FStar_UInt128_uint128
FStar_UInt128_logor(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b);

static inline FStar_UInt128_uint128
FStar_UInt128_shift_left(FStar_UInt128_uint128 a, uint32_t s);

static inline FStar_UInt128_uint128 FStar_UInt128_mul_wide(uint64_t x, uint64_t y);

static inline void store128_be(uint8_t *x0, FStar_UInt128_uint128 x1);

static inline FStar_UInt128_uint128 load128_be(uint8_t *x0);

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Krmllib_H_DEFINED
#endif
