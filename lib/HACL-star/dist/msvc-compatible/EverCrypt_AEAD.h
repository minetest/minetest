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


#ifndef __EverCrypt_AEAD_H
#define __EverCrypt_AEAD_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Spec.h"
#include "EverCrypt_Error.h"
#include "EverCrypt_Chacha20Poly1305.h"
#include "EverCrypt_AutoConfig2.h"

typedef struct EverCrypt_AEAD_state_s_s EverCrypt_AEAD_state_s;

/**
Both encryption and decryption require a state that holds the key.
The state may be reused as many times as desired.
*/
bool EverCrypt_AEAD_uu___is_Ek(Spec_Agile_AEAD_alg a, EverCrypt_AEAD_state_s projectee);

/**
Return the algorithm used in the AEAD state.

@param s State of the AEAD algorithm.

@return Algorithm used in the AEAD state.
*/
Spec_Agile_AEAD_alg EverCrypt_AEAD_alg_of_state(EverCrypt_AEAD_state_s *s);

/**
Create the required AEAD state for the algorithm.

Note: The caller must free the AEAD state by calling `EverCrypt_AEAD_free`.

@param a The argument `a` must be either of:
  * `Spec_Agile_AEAD_AES128_GCM` (KEY_LEN=16),
  * `Spec_Agile_AEAD_AES256_GCM` (KEY_LEN=32), or
  * `Spec_Agile_AEAD_CHACHA20_POLY1305` (KEY_LEN=32).
@param dst Pointer to a pointer where the address of the allocated AEAD state will be written to.
@param k Pointer to `KEY_LEN` bytes of memory where the key is read from. The size depends on the used algorithm, see above.

@return The function returns `EverCrypt_Error_Success` on success or
  `EverCrypt_Error_UnsupportedAlgorithm` in case of a bad algorithm identifier.
  (See `EverCrypt_Error.h`.)
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_create_in(Spec_Agile_AEAD_alg a, EverCrypt_AEAD_state_s **dst, uint8_t *k);

/**
Encrypt and authenticate a message (`plain`) with associated data (`ad`).

@param s Pointer to the The AEAD state created by `EverCrypt_AEAD_create_in`. It already contains the encryption key.
@param iv Pointer to `iv_len` bytes of memory where the nonce is read from.
@param iv_len Length of the nonce. Note: ChaCha20Poly1305 requires a 12 byte nonce.
@param ad Pointer to `ad_len` bytes of memory where the associated data is read from.
@param ad_len Length of the associated data.
@param plain Pointer to `plain_len` bytes of memory where the to-be-encrypted plaintext is read from.
@param plain_len Length of the to-be-encrypted plaintext.
@param cipher Pointer to `plain_len` bytes of memory where the ciphertext is written to.
@param tag Pointer to `TAG_LEN` bytes of memory where the tag is written to.
  The length of the `tag` must be of a suitable length for the chosen algorithm:
  * `Spec_Agile_AEAD_AES128_GCM` (TAG_LEN=16)
  * `Spec_Agile_AEAD_AES256_GCM` (TAG_LEN=16)
  * `Spec_Agile_AEAD_CHACHA20_POLY1305` (TAG_LEN=16)

@return `EverCrypt_AEAD_encrypt` may return either `EverCrypt_Error_Success` or `EverCrypt_Error_InvalidKey` (`EverCrypt_error.h`). The latter is returned if and only if the `s` parameter is `NULL`.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

/**
WARNING: this function doesn't perform any dynamic
  hardware check. You MUST make sure your hardware supports the
  implementation of AESGCM. Besides, this function was not designed
  for cross-compilation: if you compile it on a system which doesn't
  support Vale, it will compile it to a function which makes the
  program exit.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand_aes128_gcm_no_check(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

/**
WARNING: this function doesn't perform any dynamic
  hardware check. You MUST make sure your hardware supports the
  implementation of AESGCM. Besides, this function was not designed
  for cross-compilation: if you compile it on a system which doesn't
  support Vale, it will compile it to a function which makes the
  program exit.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand_aes256_gcm_no_check(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand_aes128_gcm(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand_aes256_gcm(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand_chacha20_poly1305(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

EverCrypt_Error_error_code
EverCrypt_AEAD_encrypt_expand(
  Spec_Agile_AEAD_alg a,
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
);

/**
Verify the authenticity of `ad` || `cipher` and decrypt `cipher` into `dst`.

@param s Pointer to the The AEAD state created by `EverCrypt_AEAD_create_in`. It already contains the encryption key.
@param iv Pointer to `iv_len` bytes of memory where the nonce is read from.
@param iv_len Length of the nonce. Note: ChaCha20Poly1305 requires a 12 byte nonce.
@param ad Pointer to `ad_len` bytes of memory where the associated data is read from.
@param ad_len Length of the associated data.
@param cipher Pointer to `cipher_len` bytes of memory where the ciphertext is read from.
@param cipher_len Length of the ciphertext.
@param tag Pointer to `TAG_LEN` bytes of memory where the tag is read from.
  The length of the `tag` must be of a suitable length for the chosen algorithm:
  * `Spec_Agile_AEAD_AES128_GCM` (TAG_LEN=16)
  * `Spec_Agile_AEAD_AES256_GCM` (TAG_LEN=16)
  * `Spec_Agile_AEAD_CHACHA20_POLY1305` (TAG_LEN=16)
@param dst Pointer to `cipher_len` bytes of memory where the decrypted plaintext will be written to.

@return `EverCrypt_AEAD_decrypt` returns ...

  * `EverCrypt_Error_Success`

  ... on success and either of ...

  * `EverCrypt_Error_InvalidKey` (returned if and only if the `s` parameter is `NULL`),
  * `EverCrypt_Error_InvalidIVLength` (see note about requirements on IV size above), or
  * `EverCrypt_Error_AuthenticationFailure` (in case the ciphertext could not be authenticated, e.g., due to modifications)

  ... on failure (`EverCrypt_error.h`).

  Upon success, the plaintext will be written into `dst`.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

/**
WARNING: this function doesn't perform any dynamic
  hardware check. You MUST make sure your hardware supports the
  implementation of AESGCM. Besides, this function was not designed
  for cross-compilation: if you compile it on a system which doesn't
  support Vale, it will compile it to a function which makes the
  program exit.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand_aes128_gcm_no_check(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

/**
WARNING: this function doesn't perform any dynamic
  hardware check. You MUST make sure your hardware supports the
  implementation of AESGCM. Besides, this function was not designed
  for cross-compilation: if you compile it on a system which doesn't
  support Vale, it will compile it to a function which makes the
  program exit.
*/
EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand_aes256_gcm_no_check(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand_aes128_gcm(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand_aes256_gcm(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand_chacha20_poly1305(
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

EverCrypt_Error_error_code
EverCrypt_AEAD_decrypt_expand(
  Spec_Agile_AEAD_alg a,
  uint8_t *k,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
);

/**
Cleanup and free the AEAD state.

@param s State of the AEAD algorithm.
*/
void EverCrypt_AEAD_free(EverCrypt_AEAD_state_s *s);

#if defined(__cplusplus)
}
#endif

#define __EverCrypt_AEAD_H_DEFINED
#endif
