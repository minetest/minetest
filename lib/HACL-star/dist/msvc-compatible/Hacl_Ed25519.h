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


#ifndef __Hacl_Ed25519_H
#define __Hacl_Ed25519_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"
#include "Hacl_Krmllib.h"
#include "Hacl_Hash_SHA2.h"

/********************************************************************************
  Verified C library for EdDSA signing and verification on the edwards25519 curve.
********************************************************************************/


/**
Compute the public key from the private key.

  @param[out] public_key Points to 32 bytes of valid memory, i.e., `uint8_t[32]`. Must not overlap the memory location of `private_key`.
  @param[in] private_key Points to 32 bytes of valid memory containing the private key, i.e., `uint8_t[32]`.
*/
void Hacl_Ed25519_secret_to_public(uint8_t *public_key, uint8_t *private_key);

/**
Compute the expanded keys for an Ed25519 signature.

  @param[out] expanded_keys Points to 96 bytes of valid memory, i.e., `uint8_t[96]`. Must not overlap the memory location of `private_key`.
  @param[in] private_key Points to 32 bytes of valid memory containing the private key, i.e., `uint8_t[32]`.

  If one needs to sign several messages under the same private key, it is more efficient
  to call `expand_keys` only once and `sign_expanded` multiple times, for each message.
*/
void Hacl_Ed25519_expand_keys(uint8_t *expanded_keys, uint8_t *private_key);

/**
Create an Ed25519 signature with the (precomputed) expanded keys.

  @param[out] signature Points to 64 bytes of valid memory, i.e., `uint8_t[64]`. Must not overlap the memory locations of `expanded_keys` nor `msg`.
  @param[in] expanded_keys Points to 96 bytes of valid memory, i.e., `uint8_t[96]`, containing the expanded keys obtained by invoking `expand_keys`.
  @param[in] msg_len Length of `msg`.
  @param[in] msg Points to `msg_len` bytes of valid memory containing the message, i.e., `uint8_t[msg_len]`.

  If one needs to sign several messages under the same private key, it is more efficient
  to call `expand_keys` only once and `sign_expanded` multiple times, for each message.
*/
void
Hacl_Ed25519_sign_expanded(
  uint8_t *signature,
  uint8_t *expanded_keys,
  uint32_t msg_len,
  uint8_t *msg
);

/**
Create an Ed25519 signature.

  @param[out] signature Points to 64 bytes of valid memory, i.e., `uint8_t[64]`. Must not overlap the memory locations of `private_key` nor `msg`.
  @param[in] private_key Points to 32 bytes of valid memory containing the private key, i.e., `uint8_t[32]`.
  @param[in] msg_len Length of `msg`.
  @param[in] msg Points to `msg_len` bytes of valid memory containing the message, i.e., `uint8_t[msg_len]`.

  The function first calls `expand_keys` and then invokes `sign_expanded`.

  If one needs to sign several messages under the same private key, it is more efficient
  to call `expand_keys` only once and `sign_expanded` multiple times, for each message.
*/
void
Hacl_Ed25519_sign(uint8_t *signature, uint8_t *private_key, uint32_t msg_len, uint8_t *msg);

/**
Verify an Ed25519 signature.

  @param public_key Points to 32 bytes of valid memory containing the public key, i.e., `uint8_t[32]`.
  @param msg_len Length of `msg`.
  @param msg Points to `msg_len` bytes of valid memory containing the message, i.e., `uint8_t[msg_len]`.
  @param signature Points to 64 bytes of valid memory containing the signature, i.e., `uint8_t[64]`.

  @return Returns `true` if the signature is valid and `false` otherwise.
*/
bool
Hacl_Ed25519_verify(uint8_t *public_key, uint32_t msg_len, uint8_t *msg, uint8_t *signature);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Ed25519_H_DEFINED
#endif
