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


#ifndef __EverCrypt_AutoConfig2_H
#define __EverCrypt_AutoConfig2_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

bool EverCrypt_AutoConfig2_has_shaext(void);

bool EverCrypt_AutoConfig2_has_aesni(void);

bool EverCrypt_AutoConfig2_has_pclmulqdq(void);

bool EverCrypt_AutoConfig2_has_avx2(void);

bool EverCrypt_AutoConfig2_has_avx(void);

bool EverCrypt_AutoConfig2_has_bmi2(void);

bool EverCrypt_AutoConfig2_has_adx(void);

bool EverCrypt_AutoConfig2_has_sse(void);

bool EverCrypt_AutoConfig2_has_movbe(void);

bool EverCrypt_AutoConfig2_has_rdrand(void);

bool EverCrypt_AutoConfig2_has_avx512(void);

void EverCrypt_AutoConfig2_recall(void);

void EverCrypt_AutoConfig2_init(void);

typedef void (*EverCrypt_AutoConfig2_disabler)(void);

void EverCrypt_AutoConfig2_disable_avx2(void);

void EverCrypt_AutoConfig2_disable_avx(void);

void EverCrypt_AutoConfig2_disable_bmi2(void);

void EverCrypt_AutoConfig2_disable_adx(void);

void EverCrypt_AutoConfig2_disable_shaext(void);

void EverCrypt_AutoConfig2_disable_aesni(void);

void EverCrypt_AutoConfig2_disable_pclmulqdq(void);

void EverCrypt_AutoConfig2_disable_sse(void);

void EverCrypt_AutoConfig2_disable_movbe(void);

void EverCrypt_AutoConfig2_disable_rdrand(void);

void EverCrypt_AutoConfig2_disable_avx512(void);

bool EverCrypt_AutoConfig2_has_vec128(void);

bool EverCrypt_AutoConfig2_has_vec256(void);

#if defined(__cplusplus)
}
#endif

#define __EverCrypt_AutoConfig2_H_DEFINED
#endif
