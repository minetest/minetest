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


#include "EverCrypt_AEAD.h"

#include "internal/Vale.h"
#include "internal/Hacl_Spec.h"
#include "config.h"

/**
Both encryption and decryption require a state that holds the key.
The state may be reused as many times as desired.
*/
typedef struct EverCrypt_AEAD_state_s_s
{
  Spec_Cipher_Expansion_impl impl;
  uint8_t *ek;
}
EverCrypt_AEAD_state_s;

/**
Both encryption and decryption require a state that holds the key.
The state may be reused as many times as desired.
*/
bool EverCrypt_AEAD_uu___is_Ek(Spec_Agile_AEAD_alg a, EverCrypt_AEAD_state_s projectee)
{
  KRML_MAYBE_UNUSED_VAR(a);
  KRML_MAYBE_UNUSED_VAR(projectee);
  return true;
}

/**
Return the algorithm used in the AEAD state.

@param s State of the AEAD algorithm.

@return Algorithm used in the AEAD state.
*/
Spec_Agile_AEAD_alg EverCrypt_AEAD_alg_of_state(EverCrypt_AEAD_state_s *s)
{
  Spec_Cipher_Expansion_impl impl = (*s).impl;
  switch (impl)
  {
    case Spec_Cipher_Expansion_Hacl_CHACHA20:
      {
        return Spec_Agile_AEAD_CHACHA20_POLY1305;
      }
    case Spec_Cipher_Expansion_Vale_AES128:
      {
        return Spec_Agile_AEAD_AES128_GCM;
      }
    case Spec_Cipher_Expansion_Vale_AES256:
      {
        return Spec_Agile_AEAD_AES256_GCM;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static EverCrypt_Error_error_code
create_in_chacha20_poly1305(EverCrypt_AEAD_state_s **dst, uint8_t *k)
{
  uint8_t *ek = (uint8_t *)KRML_HOST_CALLOC(32U, sizeof (uint8_t));
  EverCrypt_AEAD_state_s
  *p = (EverCrypt_AEAD_state_s *)KRML_HOST_MALLOC(sizeof (EverCrypt_AEAD_state_s));
  p[0U] = ((EverCrypt_AEAD_state_s){ .impl = Spec_Cipher_Expansion_Hacl_CHACHA20, .ek = ek });
  memcpy(ek, k, 32U * sizeof (uint8_t));
  dst[0U] = p;
  return EverCrypt_Error_Success;
}

static EverCrypt_Error_error_code
create_in_aes128_gcm(EverCrypt_AEAD_state_s **dst, uint8_t *k)
{
  KRML_MAYBE_UNUSED_VAR(dst);
  KRML_MAYBE_UNUSED_VAR(k);
  #if HACL_CAN_COMPILE_VALE
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t *ek = (uint8_t *)KRML_HOST_CALLOC(480U, sizeof (uint8_t));
    uint8_t *keys_b = ek;
    uint8_t *hkeys_b = ek + 176U;
    aes128_key_expansion(k, keys_b);
    aes128_keyhash_init(keys_b, hkeys_b);
    EverCrypt_AEAD_state_s
    *p = (EverCrypt_AEAD_state_s *)KRML_HOST_MALLOC(sizeof (EverCrypt_AEAD_state_s));
    p[0U] = ((EverCrypt_AEAD_state_s){ .impl = Spec_Cipher_Expansion_Vale_AES128, .ek = ek });
    *dst = p;
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

static EverCrypt_Error_error_code
create_in_aes256_gcm(EverCrypt_AEAD_state_s **dst, uint8_t *k)
{
  KRML_MAYBE_UNUSED_VAR(dst);
  KRML_MAYBE_UNUSED_VAR(k);
  #if HACL_CAN_COMPILE_VALE
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t *ek = (uint8_t *)KRML_HOST_CALLOC(544U, sizeof (uint8_t));
    uint8_t *keys_b = ek;
    uint8_t *hkeys_b = ek + 240U;
    aes256_key_expansion(k, keys_b);
    aes256_keyhash_init(keys_b, hkeys_b);
    EverCrypt_AEAD_state_s
    *p = (EverCrypt_AEAD_state_s *)KRML_HOST_MALLOC(sizeof (EverCrypt_AEAD_state_s));
    p[0U] = ((EverCrypt_AEAD_state_s){ .impl = Spec_Cipher_Expansion_Vale_AES256, .ek = ek });
    *dst = p;
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

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
EverCrypt_AEAD_create_in(Spec_Agile_AEAD_alg a, EverCrypt_AEAD_state_s **dst, uint8_t *k)
{
  switch (a)
  {
    case Spec_Agile_AEAD_AES128_GCM:
      {
        return create_in_aes128_gcm(dst, k);
      }
    case Spec_Agile_AEAD_AES256_GCM:
      {
        return create_in_aes256_gcm(dst, k);
      }
    case Spec_Agile_AEAD_CHACHA20_POLY1305:
      {
        return create_in_chacha20_poly1305(dst, k);
      }
    default:
      {
        return EverCrypt_Error_UnsupportedAlgorithm;
      }
  }
}

static EverCrypt_Error_error_code
encrypt_aes128_gcm(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
)
{
  KRML_MAYBE_UNUSED_VAR(s);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek = (*s).ek;
  uint8_t *scratch_b = ek + 304U;
  uint8_t *ek1 = ek;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 176U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *plain_b_ = plain;
  uint8_t *out_b_ = cipher;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
  if (len128x6 / 16ULL >= 18ULL)
  {
    uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = plain_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    gcm128_encrypt_opt(auth_b_,
      (uint64_t)ad_len,
      auth_num,
      keys_b,
      tmp_iv,
      hkeys_b,
      abytes_b,
      in128x6_b,
      out128x6_b,
      len128x6_,
      in128_b,
      out128_b,
      len128_num_,
      inout_b,
      (uint64_t)plain_len,
      scratch_b1,
      tag);
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = plain_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = plain_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    gcm128_encrypt_opt(auth_b_,
      (uint64_t)ad_len,
      auth_num,
      keys_b,
      tmp_iv,
      hkeys_b,
      abytes_b,
      in128x6_b,
      out128x6_b,
      len128x6_,
      in128_b,
      out128_b,
      len128_num_,
      inout_b,
      (uint64_t)plain_len,
      scratch_b1,
      tag);
  }
  memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
  return EverCrypt_Error_Success;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "statically unreachable");
  KRML_HOST_EXIT(255U);
  #endif
}

static EverCrypt_Error_error_code
encrypt_aes256_gcm(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *plain,
  uint32_t plain_len,
  uint8_t *cipher,
  uint8_t *tag
)
{
  KRML_MAYBE_UNUSED_VAR(s);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek = (*s).ek;
  uint8_t *scratch_b = ek + 368U;
  uint8_t *ek1 = ek;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 240U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *plain_b_ = plain;
  uint8_t *out_b_ = cipher;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
  if (len128x6 / 16ULL >= 18ULL)
  {
    uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = plain_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    gcm256_encrypt_opt(auth_b_,
      (uint64_t)ad_len,
      auth_num,
      keys_b,
      tmp_iv,
      hkeys_b,
      abytes_b,
      in128x6_b,
      out128x6_b,
      len128x6_,
      in128_b,
      out128_b,
      len128_num_,
      inout_b,
      (uint64_t)plain_len,
      scratch_b1,
      tag);
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = plain_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = plain_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    gcm256_encrypt_opt(auth_b_,
      (uint64_t)ad_len,
      auth_num,
      keys_b,
      tmp_iv,
      hkeys_b,
      abytes_b,
      in128x6_b,
      out128x6_b,
      len128x6_,
      in128_b,
      out128_b,
      len128_num_,
      inout_b,
      (uint64_t)plain_len,
      scratch_b1,
      tag);
  }
  memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
  return EverCrypt_Error_Success;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "statically unreachable");
  KRML_HOST_EXIT(255U);
  #endif
}

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
)
{
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  EverCrypt_AEAD_state_s scrut = *s;
  Spec_Cipher_Expansion_impl i = scrut.impl;
  uint8_t *ek = scrut.ek;
  switch (i)
  {
    case Spec_Cipher_Expansion_Vale_AES128:
      {
        return encrypt_aes128_gcm(s, iv, iv_len, ad, ad_len, plain, plain_len, cipher, tag);
      }
    case Spec_Cipher_Expansion_Vale_AES256:
      {
        return encrypt_aes256_gcm(s, iv, iv_len, ad, ad_len, plain, plain_len, cipher, tag);
      }
    case Spec_Cipher_Expansion_Hacl_CHACHA20:
      {
        if (iv_len != 12U)
        {
          return EverCrypt_Error_InvalidIVLength;
        }
        EverCrypt_Chacha20Poly1305_aead_encrypt(ek, iv, ad_len, ad, plain_len, plain, cipher, tag);
        return EverCrypt_Error_Success;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  uint8_t ek[480U] = { 0U };
  uint8_t *keys_b0 = ek;
  uint8_t *hkeys_b0 = ek + 176U;
  aes128_key_expansion(k, keys_b0);
  aes128_keyhash_init(keys_b0, hkeys_b0);
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES128, .ek = ek };
  EverCrypt_AEAD_state_s *s = &p;
  if (s == NULL)
  {
    KRML_HOST_IGNORE(EverCrypt_Error_InvalidKey);
  }
  else if (iv_len == 0U)
  {
    KRML_HOST_IGNORE(EverCrypt_Error_InvalidIVLength);
  }
  else
  {
    uint8_t *ek0 = (*s).ek;
    uint8_t *scratch_b = ek0 + 304U;
    uint8_t *ek1 = ek0;
    uint8_t *keys_b = ek1;
    uint8_t *hkeys_b = ek1 + 176U;
    uint8_t tmp_iv[16U] = { 0U };
    uint32_t len = iv_len / 16U;
    uint32_t bytes_len = len * 16U;
    uint8_t *iv_b = iv;
    memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
    compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
    uint8_t *inout_b = scratch_b;
    uint8_t *abytes_b = scratch_b + 16U;
    uint8_t *scratch_b1 = scratch_b + 32U;
    uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
    uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
    uint8_t *plain_b_ = plain;
    uint8_t *out_b_ = cipher;
    uint8_t *auth_b_ = ad;
    memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
    memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
    uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
    if (len128x6 / 16ULL >= 18ULL)
    {
      uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
      uint8_t *in128x6_b = plain_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
      uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128x6_ = len128x6 / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      gcm128_encrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)plain_len,
        scratch_b1,
        tag);
    }
    else
    {
      uint32_t len128x61 = 0U;
      uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
      uint8_t *in128x6_b = plain_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = plain_b_ + len128x61;
      uint8_t *out128_b = out_b_ + len128x61;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t len128x6_ = 0ULL;
      gcm128_encrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)plain_len,
        scratch_b1,
        tag);
    }
    memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
      inout_b,
      (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
    KRML_HOST_IGNORE(EverCrypt_Error_Success);
  }
  return EverCrypt_Error_Success;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "EverCrypt was compiled on a system which doesn\'t support Vale");
  KRML_HOST_EXIT(255U);
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  uint8_t ek[544U] = { 0U };
  uint8_t *keys_b0 = ek;
  uint8_t *hkeys_b0 = ek + 240U;
  aes256_key_expansion(k, keys_b0);
  aes256_keyhash_init(keys_b0, hkeys_b0);
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES256, .ek = ek };
  EverCrypt_AEAD_state_s *s = &p;
  if (s == NULL)
  {
    KRML_HOST_IGNORE(EverCrypt_Error_InvalidKey);
  }
  else if (iv_len == 0U)
  {
    KRML_HOST_IGNORE(EverCrypt_Error_InvalidIVLength);
  }
  else
  {
    uint8_t *ek0 = (*s).ek;
    uint8_t *scratch_b = ek0 + 368U;
    uint8_t *ek1 = ek0;
    uint8_t *keys_b = ek1;
    uint8_t *hkeys_b = ek1 + 240U;
    uint8_t tmp_iv[16U] = { 0U };
    uint32_t len = iv_len / 16U;
    uint32_t bytes_len = len * 16U;
    uint8_t *iv_b = iv;
    memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
    compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
    uint8_t *inout_b = scratch_b;
    uint8_t *abytes_b = scratch_b + 16U;
    uint8_t *scratch_b1 = scratch_b + 32U;
    uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
    uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
    uint8_t *plain_b_ = plain;
    uint8_t *out_b_ = cipher;
    uint8_t *auth_b_ = ad;
    memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
    memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
    uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
    if (len128x6 / 16ULL >= 18ULL)
    {
      uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
      uint8_t *in128x6_b = plain_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
      uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128x6_ = len128x6 / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      gcm256_encrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)plain_len,
        scratch_b1,
        tag);
    }
    else
    {
      uint32_t len128x61 = 0U;
      uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
      uint8_t *in128x6_b = plain_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = plain_b_ + len128x61;
      uint8_t *out128_b = out_b_ + len128x61;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t len128x6_ = 0ULL;
      gcm256_encrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)plain_len,
        scratch_b1,
        tag);
    }
    memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
      inout_b,
      (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
    KRML_HOST_IGNORE(EverCrypt_Error_Success);
  }
  return EverCrypt_Error_Success;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "EverCrypt was compiled on a system which doesn\'t support Vale");
  KRML_HOST_EXIT(255U);
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t ek[480U] = { 0U };
    uint8_t *keys_b0 = ek;
    uint8_t *hkeys_b0 = ek + 176U;
    aes128_key_expansion(k, keys_b0);
    aes128_keyhash_init(keys_b0, hkeys_b0);
    EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES128, .ek = ek };
    EverCrypt_AEAD_state_s *s = &p;
    if (s == NULL)
    {
      KRML_HOST_IGNORE(EverCrypt_Error_InvalidKey);
    }
    else if (iv_len == 0U)
    {
      KRML_HOST_IGNORE(EverCrypt_Error_InvalidIVLength);
    }
    else
    {
      uint8_t *ek0 = (*s).ek;
      uint8_t *scratch_b = ek0 + 304U;
      uint8_t *ek1 = ek0;
      uint8_t *keys_b = ek1;
      uint8_t *hkeys_b = ek1 + 176U;
      uint8_t tmp_iv[16U] = { 0U };
      uint32_t len = iv_len / 16U;
      uint32_t bytes_len = len * 16U;
      uint8_t *iv_b = iv;
      memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
      compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
      uint8_t *inout_b = scratch_b;
      uint8_t *abytes_b = scratch_b + 16U;
      uint8_t *scratch_b1 = scratch_b + 32U;
      uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
      uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
      uint8_t *plain_b_ = plain;
      uint8_t *out_b_ = cipher;
      uint8_t *auth_b_ = ad;
      memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
      memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
      uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
      if (len128x6 / 16ULL >= 18ULL)
      {
        uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
        uint8_t *in128x6_b = plain_b_;
        uint8_t *out128x6_b = out_b_;
        uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
        uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
        uint64_t auth_num = (uint64_t)ad_len / 16ULL;
        uint64_t len128x6_ = len128x6 / 16ULL;
        uint64_t len128_num_ = len128_num / 16ULL;
        gcm128_encrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)plain_len,
          scratch_b1,
          tag);
      }
      else
      {
        uint32_t len128x61 = 0U;
        uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
        uint8_t *in128x6_b = plain_b_;
        uint8_t *out128x6_b = out_b_;
        uint8_t *in128_b = plain_b_ + len128x61;
        uint8_t *out128_b = out_b_ + len128x61;
        uint64_t auth_num = (uint64_t)ad_len / 16ULL;
        uint64_t len128_num_ = len128_num / 16ULL;
        uint64_t len128x6_ = 0ULL;
        gcm128_encrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)plain_len,
          scratch_b1,
          tag);
      }
      memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
        inout_b,
        (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
      KRML_HOST_IGNORE(EverCrypt_Error_Success);
    }
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(plain);
  KRML_MAYBE_UNUSED_VAR(plain_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(tag);
  #if HACL_CAN_COMPILE_VALE
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t ek[544U] = { 0U };
    uint8_t *keys_b0 = ek;
    uint8_t *hkeys_b0 = ek + 240U;
    aes256_key_expansion(k, keys_b0);
    aes256_keyhash_init(keys_b0, hkeys_b0);
    EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES256, .ek = ek };
    EverCrypt_AEAD_state_s *s = &p;
    if (s == NULL)
    {
      KRML_HOST_IGNORE(EverCrypt_Error_InvalidKey);
    }
    else if (iv_len == 0U)
    {
      KRML_HOST_IGNORE(EverCrypt_Error_InvalidIVLength);
    }
    else
    {
      uint8_t *ek0 = (*s).ek;
      uint8_t *scratch_b = ek0 + 368U;
      uint8_t *ek1 = ek0;
      uint8_t *keys_b = ek1;
      uint8_t *hkeys_b = ek1 + 240U;
      uint8_t tmp_iv[16U] = { 0U };
      uint32_t len = iv_len / 16U;
      uint32_t bytes_len = len * 16U;
      uint8_t *iv_b = iv;
      memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
      compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
      uint8_t *inout_b = scratch_b;
      uint8_t *abytes_b = scratch_b + 16U;
      uint8_t *scratch_b1 = scratch_b + 32U;
      uint32_t plain_len_ = (uint32_t)(uint64_t)plain_len / 16U * 16U;
      uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
      uint8_t *plain_b_ = plain;
      uint8_t *out_b_ = cipher;
      uint8_t *auth_b_ = ad;
      memcpy(inout_b, plain + plain_len_, (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
      memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
      uint64_t len128x6 = (uint64_t)plain_len / 96ULL * 96ULL;
      if (len128x6 / 16ULL >= 18ULL)
      {
        uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL - len128x6;
        uint8_t *in128x6_b = plain_b_;
        uint8_t *out128x6_b = out_b_;
        uint8_t *in128_b = plain_b_ + (uint32_t)len128x6;
        uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
        uint64_t auth_num = (uint64_t)ad_len / 16ULL;
        uint64_t len128x6_ = len128x6 / 16ULL;
        uint64_t len128_num_ = len128_num / 16ULL;
        gcm256_encrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)plain_len,
          scratch_b1,
          tag);
      }
      else
      {
        uint32_t len128x61 = 0U;
        uint64_t len128_num = (uint64_t)plain_len / 16ULL * 16ULL;
        uint8_t *in128x6_b = plain_b_;
        uint8_t *out128x6_b = out_b_;
        uint8_t *in128_b = plain_b_ + len128x61;
        uint8_t *out128_b = out_b_ + len128x61;
        uint64_t auth_num = (uint64_t)ad_len / 16ULL;
        uint64_t len128_num_ = len128_num / 16ULL;
        uint64_t len128x6_ = 0ULL;
        gcm256_encrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)plain_len,
          scratch_b1,
          tag);
      }
      memcpy(cipher + (uint32_t)(uint64_t)plain_len / 16U * 16U,
        inout_b,
        (uint32_t)(uint64_t)plain_len % 16U * sizeof (uint8_t));
      KRML_HOST_IGNORE(EverCrypt_Error_Success);
    }
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(iv_len);
  uint8_t ek[32U] = { 0U };
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Hacl_CHACHA20, .ek = ek };
  memcpy(ek, k, 32U * sizeof (uint8_t));
  EverCrypt_AEAD_state_s *s = &p;
  uint8_t *ek0 = (*s).ek;
  EverCrypt_Chacha20Poly1305_aead_encrypt(ek0, iv, ad_len, ad, plain_len, plain, cipher, tag);
  return EverCrypt_Error_Success;
}

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
)
{
  switch (a)
  {
    case Spec_Agile_AEAD_AES128_GCM:
      {
        return
          EverCrypt_AEAD_encrypt_expand_aes128_gcm(k,
            iv,
            iv_len,
            ad,
            ad_len,
            plain,
            plain_len,
            cipher,
            tag);
      }
    case Spec_Agile_AEAD_AES256_GCM:
      {
        return
          EverCrypt_AEAD_encrypt_expand_aes256_gcm(k,
            iv,
            iv_len,
            ad,
            ad_len,
            plain,
            plain_len,
            cipher,
            tag);
      }
    case Spec_Agile_AEAD_CHACHA20_POLY1305:
      {
        return
          EverCrypt_AEAD_encrypt_expand_chacha20_poly1305(k,
            iv,
            iv_len,
            ad,
            ad_len,
            plain,
            plain_len,
            cipher,
            tag);
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static EverCrypt_Error_error_code
decrypt_aes128_gcm(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
)
{
  KRML_MAYBE_UNUSED_VAR(s);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek = (*s).ek;
  uint8_t *scratch_b = ek + 304U;
  uint8_t *ek1 = ek;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 176U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *cipher_b_ = cipher;
  uint8_t *out_b_ = dst;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
  uint64_t c;
  if (len128x6 / 16ULL >= 6ULL)
  {
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t
    c0 =
      gcm128_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    uint64_t
    c0 =
      gcm128_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  uint64_t r = c;
  if (r == 0ULL)
  {
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_AuthenticationFailure;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "statically unreachable");
  KRML_HOST_EXIT(255U);
  #endif
}

static EverCrypt_Error_error_code
decrypt_aes256_gcm(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
)
{
  KRML_MAYBE_UNUSED_VAR(s);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek = (*s).ek;
  uint8_t *scratch_b = ek + 368U;
  uint8_t *ek1 = ek;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 240U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *cipher_b_ = cipher;
  uint8_t *out_b_ = dst;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
  uint64_t c;
  if (len128x6 / 16ULL >= 6ULL)
  {
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t
    c0 =
      gcm256_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    uint64_t
    c0 =
      gcm256_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  uint64_t r = c;
  if (r == 0ULL)
  {
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_AuthenticationFailure;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "statically unreachable");
  KRML_HOST_EXIT(255U);
  #endif
}

static EverCrypt_Error_error_code
decrypt_chacha20_poly1305(
  EverCrypt_AEAD_state_s *s,
  uint8_t *iv,
  uint32_t iv_len,
  uint8_t *ad,
  uint32_t ad_len,
  uint8_t *cipher,
  uint32_t cipher_len,
  uint8_t *tag,
  uint8_t *dst
)
{
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len != 12U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek = (*s).ek;
  uint32_t
  r = EverCrypt_Chacha20Poly1305_aead_decrypt(ek, iv, ad_len, ad, cipher_len, dst, cipher, tag);
  if (r == 0U)
  {
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_AuthenticationFailure;
}

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
)
{
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  Spec_Cipher_Expansion_impl i = (*s).impl;
  switch (i)
  {
    case Spec_Cipher_Expansion_Vale_AES128:
      {
        return decrypt_aes128_gcm(s, iv, iv_len, ad, ad_len, cipher, cipher_len, tag, dst);
      }
    case Spec_Cipher_Expansion_Vale_AES256:
      {
        return decrypt_aes256_gcm(s, iv, iv_len, ad, ad_len, cipher, cipher_len, tag, dst);
      }
    case Spec_Cipher_Expansion_Hacl_CHACHA20:
      {
        return decrypt_chacha20_poly1305(s, iv, iv_len, ad, ad_len, cipher, cipher_len, tag, dst);
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  uint8_t ek[480U] = { 0U };
  uint8_t *keys_b0 = ek;
  uint8_t *hkeys_b0 = ek + 176U;
  aes128_key_expansion(k, keys_b0);
  aes128_keyhash_init(keys_b0, hkeys_b0);
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES128, .ek = ek };
  EverCrypt_AEAD_state_s *s = &p;
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek0 = (*s).ek;
  uint8_t *scratch_b = ek0 + 304U;
  uint8_t *ek1 = ek0;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 176U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *cipher_b_ = cipher;
  uint8_t *out_b_ = dst;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
  uint64_t c;
  if (len128x6 / 16ULL >= 6ULL)
  {
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t
    c0 =
      gcm128_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    uint64_t
    c0 =
      gcm128_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  uint64_t r = c;
  if (r == 0ULL)
  {
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_AuthenticationFailure;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "EverCrypt was compiled on a system which doesn\'t support Vale");
  KRML_HOST_EXIT(255U);
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  uint8_t ek[544U] = { 0U };
  uint8_t *keys_b0 = ek;
  uint8_t *hkeys_b0 = ek + 240U;
  aes256_key_expansion(k, keys_b0);
  aes256_keyhash_init(keys_b0, hkeys_b0);
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES256, .ek = ek };
  EverCrypt_AEAD_state_s *s = &p;
  if (s == NULL)
  {
    return EverCrypt_Error_InvalidKey;
  }
  if (iv_len == 0U)
  {
    return EverCrypt_Error_InvalidIVLength;
  }
  uint8_t *ek0 = (*s).ek;
  uint8_t *scratch_b = ek0 + 368U;
  uint8_t *ek1 = ek0;
  uint8_t *keys_b = ek1;
  uint8_t *hkeys_b = ek1 + 240U;
  uint8_t tmp_iv[16U] = { 0U };
  uint32_t len = iv_len / 16U;
  uint32_t bytes_len = len * 16U;
  uint8_t *iv_b = iv;
  memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
  compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
  uint8_t *inout_b = scratch_b;
  uint8_t *abytes_b = scratch_b + 16U;
  uint8_t *scratch_b1 = scratch_b + 32U;
  uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
  uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
  uint8_t *cipher_b_ = cipher;
  uint8_t *out_b_ = dst;
  uint8_t *auth_b_ = ad;
  memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
  uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
  uint64_t c;
  if (len128x6 / 16ULL >= 6ULL)
  {
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
    uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128x6_ = len128x6 / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t
    c0 =
      gcm256_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  else
  {
    uint32_t len128x61 = 0U;
    uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
    uint8_t *in128x6_b = cipher_b_;
    uint8_t *out128x6_b = out_b_;
    uint8_t *in128_b = cipher_b_ + len128x61;
    uint8_t *out128_b = out_b_ + len128x61;
    uint64_t auth_num = (uint64_t)ad_len / 16ULL;
    uint64_t len128_num_ = len128_num / 16ULL;
    uint64_t len128x6_ = 0ULL;
    uint64_t
    c0 =
      gcm256_decrypt_opt(auth_b_,
        (uint64_t)ad_len,
        auth_num,
        keys_b,
        tmp_iv,
        hkeys_b,
        abytes_b,
        in128x6_b,
        out128x6_b,
        len128x6_,
        in128_b,
        out128_b,
        len128_num_,
        inout_b,
        (uint64_t)cipher_len,
        scratch_b1,
        tag);
    c = c0;
  }
  memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
    inout_b,
    (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
  uint64_t r = c;
  if (r == 0ULL)
  {
    return EverCrypt_Error_Success;
  }
  return EverCrypt_Error_AuthenticationFailure;
  #else
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "EverCrypt was compiled on a system which doesn\'t support Vale");
  KRML_HOST_EXIT(255U);
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t ek[480U] = { 0U };
    uint8_t *keys_b0 = ek;
    uint8_t *hkeys_b0 = ek + 176U;
    aes128_key_expansion(k, keys_b0);
    aes128_keyhash_init(keys_b0, hkeys_b0);
    EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES128, .ek = ek };
    EverCrypt_AEAD_state_s *s = &p;
    if (s == NULL)
    {
      return EverCrypt_Error_InvalidKey;
    }
    if (iv_len == 0U)
    {
      return EverCrypt_Error_InvalidIVLength;
    }
    uint8_t *ek0 = (*s).ek;
    uint8_t *scratch_b = ek0 + 304U;
    uint8_t *ek1 = ek0;
    uint8_t *keys_b = ek1;
    uint8_t *hkeys_b = ek1 + 176U;
    uint8_t tmp_iv[16U] = { 0U };
    uint32_t len = iv_len / 16U;
    uint32_t bytes_len = len * 16U;
    uint8_t *iv_b = iv;
    memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
    compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
    uint8_t *inout_b = scratch_b;
    uint8_t *abytes_b = scratch_b + 16U;
    uint8_t *scratch_b1 = scratch_b + 32U;
    uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
    uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
    uint8_t *cipher_b_ = cipher;
    uint8_t *out_b_ = dst;
    uint8_t *auth_b_ = ad;
    memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
    memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
    uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
    uint64_t c;
    if (len128x6 / 16ULL >= 6ULL)
    {
      uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
      uint8_t *in128x6_b = cipher_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
      uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128x6_ = len128x6 / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t
      c0 =
        gcm128_decrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)cipher_len,
          scratch_b1,
          tag);
      c = c0;
    }
    else
    {
      uint32_t len128x61 = 0U;
      uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
      uint8_t *in128x6_b = cipher_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = cipher_b_ + len128x61;
      uint8_t *out128_b = out_b_ + len128x61;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t len128x6_ = 0ULL;
      uint64_t
      c0 =
        gcm128_decrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)cipher_len,
          scratch_b1,
          tag);
      c = c0;
    }
    memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
      inout_b,
      (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
    uint64_t r = c;
    if (r == 0ULL)
    {
      return EverCrypt_Error_Success;
    }
    return EverCrypt_Error_AuthenticationFailure;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

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
)
{
  KRML_MAYBE_UNUSED_VAR(k);
  KRML_MAYBE_UNUSED_VAR(iv);
  KRML_MAYBE_UNUSED_VAR(iv_len);
  KRML_MAYBE_UNUSED_VAR(ad);
  KRML_MAYBE_UNUSED_VAR(ad_len);
  KRML_MAYBE_UNUSED_VAR(cipher);
  KRML_MAYBE_UNUSED_VAR(cipher_len);
  KRML_MAYBE_UNUSED_VAR(tag);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if HACL_CAN_COMPILE_VALE
  bool has_pclmulqdq = EverCrypt_AutoConfig2_has_pclmulqdq();
  bool has_avx = EverCrypt_AutoConfig2_has_avx();
  bool has_sse = EverCrypt_AutoConfig2_has_sse();
  bool has_movbe = EverCrypt_AutoConfig2_has_movbe();
  bool has_aesni = EverCrypt_AutoConfig2_has_aesni();
  if (has_aesni && has_pclmulqdq && has_avx && has_sse && has_movbe)
  {
    uint8_t ek[544U] = { 0U };
    uint8_t *keys_b0 = ek;
    uint8_t *hkeys_b0 = ek + 240U;
    aes256_key_expansion(k, keys_b0);
    aes256_keyhash_init(keys_b0, hkeys_b0);
    EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Vale_AES256, .ek = ek };
    EverCrypt_AEAD_state_s *s = &p;
    if (s == NULL)
    {
      return EverCrypt_Error_InvalidKey;
    }
    if (iv_len == 0U)
    {
      return EverCrypt_Error_InvalidIVLength;
    }
    uint8_t *ek0 = (*s).ek;
    uint8_t *scratch_b = ek0 + 368U;
    uint8_t *ek1 = ek0;
    uint8_t *keys_b = ek1;
    uint8_t *hkeys_b = ek1 + 240U;
    uint8_t tmp_iv[16U] = { 0U };
    uint32_t len = iv_len / 16U;
    uint32_t bytes_len = len * 16U;
    uint8_t *iv_b = iv;
    memcpy(tmp_iv, iv + bytes_len, iv_len % 16U * sizeof (uint8_t));
    compute_iv_stdcall(iv_b, (uint64_t)iv_len, (uint64_t)len, tmp_iv, tmp_iv, hkeys_b);
    uint8_t *inout_b = scratch_b;
    uint8_t *abytes_b = scratch_b + 16U;
    uint8_t *scratch_b1 = scratch_b + 32U;
    uint32_t cipher_len_ = (uint32_t)(uint64_t)cipher_len / 16U * 16U;
    uint32_t auth_len_ = (uint32_t)(uint64_t)ad_len / 16U * 16U;
    uint8_t *cipher_b_ = cipher;
    uint8_t *out_b_ = dst;
    uint8_t *auth_b_ = ad;
    memcpy(inout_b, cipher + cipher_len_, (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
    memcpy(abytes_b, ad + auth_len_, (uint32_t)(uint64_t)ad_len % 16U * sizeof (uint8_t));
    uint64_t len128x6 = (uint64_t)cipher_len / 96ULL * 96ULL;
    uint64_t c;
    if (len128x6 / 16ULL >= 6ULL)
    {
      uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL - len128x6;
      uint8_t *in128x6_b = cipher_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = cipher_b_ + (uint32_t)len128x6;
      uint8_t *out128_b = out_b_ + (uint32_t)len128x6;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128x6_ = len128x6 / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t
      c0 =
        gcm256_decrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)cipher_len,
          scratch_b1,
          tag);
      c = c0;
    }
    else
    {
      uint32_t len128x61 = 0U;
      uint64_t len128_num = (uint64_t)cipher_len / 16ULL * 16ULL;
      uint8_t *in128x6_b = cipher_b_;
      uint8_t *out128x6_b = out_b_;
      uint8_t *in128_b = cipher_b_ + len128x61;
      uint8_t *out128_b = out_b_ + len128x61;
      uint64_t auth_num = (uint64_t)ad_len / 16ULL;
      uint64_t len128_num_ = len128_num / 16ULL;
      uint64_t len128x6_ = 0ULL;
      uint64_t
      c0 =
        gcm256_decrypt_opt(auth_b_,
          (uint64_t)ad_len,
          auth_num,
          keys_b,
          tmp_iv,
          hkeys_b,
          abytes_b,
          in128x6_b,
          out128x6_b,
          len128x6_,
          in128_b,
          out128_b,
          len128_num_,
          inout_b,
          (uint64_t)cipher_len,
          scratch_b1,
          tag);
      c = c0;
    }
    memcpy(dst + (uint32_t)(uint64_t)cipher_len / 16U * 16U,
      inout_b,
      (uint32_t)(uint64_t)cipher_len % 16U * sizeof (uint8_t));
    uint64_t r = c;
    if (r == 0ULL)
    {
      return EverCrypt_Error_Success;
    }
    return EverCrypt_Error_AuthenticationFailure;
  }
  return EverCrypt_Error_UnsupportedAlgorithm;
  #else
  return EverCrypt_Error_UnsupportedAlgorithm;
  #endif
}

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
)
{
  uint8_t ek[32U] = { 0U };
  EverCrypt_AEAD_state_s p = { .impl = Spec_Cipher_Expansion_Hacl_CHACHA20, .ek = ek };
  memcpy(ek, k, 32U * sizeof (uint8_t));
  EverCrypt_AEAD_state_s *s = &p;
  EverCrypt_Error_error_code
  r = decrypt_chacha20_poly1305(s, iv, iv_len, ad, ad_len, cipher, cipher_len, tag, dst);
  return r;
}

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
)
{
  switch (a)
  {
    case Spec_Agile_AEAD_AES128_GCM:
      {
        return
          EverCrypt_AEAD_decrypt_expand_aes128_gcm(k,
            iv,
            iv_len,
            ad,
            ad_len,
            cipher,
            cipher_len,
            tag,
            dst);
      }
    case Spec_Agile_AEAD_AES256_GCM:
      {
        return
          EverCrypt_AEAD_decrypt_expand_aes256_gcm(k,
            iv,
            iv_len,
            ad,
            ad_len,
            cipher,
            cipher_len,
            tag,
            dst);
      }
    case Spec_Agile_AEAD_CHACHA20_POLY1305:
      {
        return
          EverCrypt_AEAD_decrypt_expand_chacha20_poly1305(k,
            iv,
            iv_len,
            ad,
            ad_len,
            cipher,
            cipher_len,
            tag,
            dst);
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

/**
Cleanup and free the AEAD state.

@param s State of the AEAD algorithm.
*/
void EverCrypt_AEAD_free(EverCrypt_AEAD_state_s *s)
{
  uint8_t *ek = (*s).ek;
  KRML_HOST_FREE(ek);
  KRML_HOST_FREE(s);
}

