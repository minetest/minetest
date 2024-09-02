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


#ifndef __Hacl_Hash_SHA3_Simd256_H
#define __Hacl_Hash_SHA3_Simd256_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_SHA2_Types.h"
#include "libintvector.h"

void
Hacl_Hash_SHA3_Simd256_absorb_inner_256(
  uint32_t rateInBytes,
  Hacl_Hash_SHA2_uint8_4p b,
  Lib_IntVector_Intrinsics_vec256 *s
);

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
);

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
);

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
);

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
);

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
);

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
);

/**
Allocate quadruple state buffer (200-bytes for each)
*/
Lib_IntVector_Intrinsics_vec256 *Hacl_Hash_SHA3_Simd256_state_malloc(void);

/**
Free quadruple state buffer
*/
void Hacl_Hash_SHA3_Simd256_state_free(Lib_IntVector_Intrinsics_vec256 *s);

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
);

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
);

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
);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_SHA3_Simd256_H_DEFINED
#endif
