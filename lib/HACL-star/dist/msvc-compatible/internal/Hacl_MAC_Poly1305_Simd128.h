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


#ifndef __internal_Hacl_MAC_Poly1305_Simd128_H
#define __internal_Hacl_MAC_Poly1305_Simd128_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "../Hacl_MAC_Poly1305_Simd128.h"
#include "libintvector.h"

void Hacl_MAC_Poly1305_Simd128_load_acc2(Lib_IntVector_Intrinsics_vec128 *acc, uint8_t *b);

void
Hacl_MAC_Poly1305_Simd128_fmul_r2_normalize(
  Lib_IntVector_Intrinsics_vec128 *out,
  Lib_IntVector_Intrinsics_vec128 *p
);

void
Hacl_MAC_Poly1305_Simd128_poly1305_init(Lib_IntVector_Intrinsics_vec128 *ctx, uint8_t *key);

void
Hacl_MAC_Poly1305_Simd128_poly1305_finish(
  uint8_t *tag,
  uint8_t *key,
  Lib_IntVector_Intrinsics_vec128 *ctx
);

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_MAC_Poly1305_Simd128_H_DEFINED
#endif
