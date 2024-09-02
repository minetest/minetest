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


#ifndef __EverCrypt_Curve25519_H
#define __EverCrypt_Curve25519_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Curve25519_64.h"
#include "Hacl_Curve25519_51.h"
#include "EverCrypt_AutoConfig2.h"

/**
Calculate a public point from a secret/private key.

This computes a scalar multiplication of the secret/private key with the curve's basepoint.

@param pub Pointer to 32 bytes of memory where the resulting point is written to.
@param priv Pointer to 32 bytes of memory where the secret/private key is read from.
*/
void EverCrypt_Curve25519_secret_to_public(uint8_t *pub, uint8_t *priv);

/**
Compute the scalar multiple of a point.

@param shared Pointer to 32 bytes of memory where the resulting point is written to.
@param my_priv Pointer to 32 bytes of memory where the secret/private key is read from.
@param their_pub Pointer to 32 bytes of memory where the public point is read from.
*/
void EverCrypt_Curve25519_scalarmult(uint8_t *shared, uint8_t *my_priv, uint8_t *their_pub);

/**
Execute the diffie-hellmann key exchange.

@param shared Pointer to 32 bytes of memory where the resulting point is written to.
@param my_priv Pointer to 32 bytes of memory where **our** secret/private key is read from.
@param their_pub Pointer to 32 bytes of memory where **their** public point is read from.
*/
bool EverCrypt_Curve25519_ecdh(uint8_t *shared, uint8_t *my_priv, uint8_t *their_pub);

#if defined(__cplusplus)
}
#endif

#define __EverCrypt_Curve25519_H_DEFINED
#endif
