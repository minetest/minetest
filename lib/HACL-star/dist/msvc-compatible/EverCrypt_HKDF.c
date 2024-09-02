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


#include "EverCrypt_HKDF.h"

#include "internal/EverCrypt_HMAC.h"

static void
expand_sha1(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 20U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_sha1(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha1(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_sha1(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha1(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_sha1(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_sha1(prk, salt, saltlen, ikm, ikmlen);
}

static void
expand_sha2_256(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 32U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_sha2_256(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_256(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_sha2_256(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_256(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_sha2_256(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_sha2_256(prk, salt, saltlen, ikm, ikmlen);
}

static void
expand_sha2_384(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 48U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_sha2_384(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_384(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_sha2_384(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_384(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_sha2_384(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_sha2_384(prk, salt, saltlen, ikm, ikmlen);
}

static void
expand_sha2_512(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 64U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_sha2_512(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_512(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_sha2_512(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_sha2_512(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_sha2_512(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_sha2_512(prk, salt, saltlen, ikm, ikmlen);
}

static void
expand_blake2s(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 32U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_blake2s(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_blake2s(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_blake2s(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_blake2s(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_blake2s(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_blake2s(prk, salt, saltlen, ikm, ikmlen);
}

static void
expand_blake2b(
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  uint32_t tlen = 64U;
  uint32_t n = len / tlen;
  uint8_t *output = okm;
  KRML_CHECK_SIZE(sizeof (uint8_t), tlen + infolen + 1U);
  uint8_t *text = (uint8_t *)alloca((tlen + infolen + 1U) * sizeof (uint8_t));
  memset(text, 0U, (tlen + infolen + 1U) * sizeof (uint8_t));
  uint8_t *text0 = text + tlen;
  uint8_t *tag = text;
  uint8_t *ctr = text + tlen + infolen;
  memcpy(text + tlen, info, infolen * sizeof (uint8_t));
  for (uint32_t i = 0U; i < n; i++)
  {
    ctr[0U] = (uint8_t)(i + 1U);
    if (i == 0U)
    {
      EverCrypt_HMAC_compute_blake2b(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_blake2b(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    memcpy(output + i * tlen, tag, tlen * sizeof (uint8_t));
  }
  if (n * tlen < len)
  {
    ctr[0U] = (uint8_t)(n + 1U);
    if (n == 0U)
    {
      EverCrypt_HMAC_compute_blake2b(tag, prk, prklen, text0, infolen + 1U);
    }
    else
    {
      EverCrypt_HMAC_compute_blake2b(tag, prk, prklen, text, tlen + infolen + 1U);
    }
    uint8_t *block = okm + n * tlen;
    memcpy(block, tag, (len - n * tlen) * sizeof (uint8_t));
  }
}

static void
extract_blake2b(uint8_t *prk, uint8_t *salt, uint32_t saltlen, uint8_t *ikm, uint32_t ikmlen)
{
  EverCrypt_HMAC_compute_blake2b(prk, salt, saltlen, ikm, ikmlen);
}

/**
Expand pseudorandom key to desired length.

@param a Hash function to use. Usually, the same as used in `EverCrypt_HKDF_extract`.
@param okm Pointer to `len` bytes of memory where output keying material is written to.
@param prk Pointer to at least `HashLen` bytes of memory where pseudorandom key is read from. Usually, this points to the output from the extract step.
@param prklen Length of pseudorandom key.
@param info Pointer to `infolen` bytes of memory where context and application specific information is read from.
@param infolen Length of context and application specific information. Can be 0.
@param len Length of output keying material.
*/
void
EverCrypt_HKDF_expand(
  Spec_Hash_Definitions_hash_alg a,
  uint8_t *okm,
  uint8_t *prk,
  uint32_t prklen,
  uint8_t *info,
  uint32_t infolen,
  uint32_t len
)
{
  switch (a)
  {
    case Spec_Hash_Definitions_SHA1:
      {
        expand_sha1(okm, prk, prklen, info, infolen, len);
        break;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        expand_sha2_256(okm, prk, prklen, info, infolen, len);
        break;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        expand_sha2_384(okm, prk, prklen, info, infolen, len);
        break;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        expand_sha2_512(okm, prk, prklen, info, infolen, len);
        break;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        expand_blake2s(okm, prk, prklen, info, infolen, len);
        break;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        expand_blake2b(okm, prk, prklen, info, infolen, len);
        break;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

/**
Extract a fixed-length pseudorandom key from input keying material.

@param a Hash function to use. The allowed values are:
  * `Spec_Hash_Definitions_Blake2B` (`HashLen` = 64), 
  * `Spec_Hash_Definitions_Blake2S` (`HashLen` = 32), 
  * `Spec_Hash_Definitions_SHA2_256` (`HashLen` = 32), 
  * `Spec_Hash_Definitions_SHA2_384` (`HashLen` = 48), 
  * `Spec_Hash_Definitions_SHA2_512` (`HashLen` = 64), and
  * `Spec_Hash_Definitions_SHA1` (`HashLen` = 20).
@param prk Pointer to `HashLen` bytes of memory where pseudorandom key is written to.
  `HashLen` depends on the used algorithm `a`. See above.
@param salt Pointer to `saltlen` bytes of memory where salt value is read from.
@param saltlen Length of salt value.
@param ikm Pointer to `ikmlen` bytes of memory where input keying material is read from.
@param ikmlen Length of input keying material.
*/
void
EverCrypt_HKDF_extract(
  Spec_Hash_Definitions_hash_alg a,
  uint8_t *prk,
  uint8_t *salt,
  uint32_t saltlen,
  uint8_t *ikm,
  uint32_t ikmlen
)
{
  switch (a)
  {
    case Spec_Hash_Definitions_SHA1:
      {
        extract_sha1(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        extract_sha2_256(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        extract_sha2_384(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        extract_sha2_512(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        extract_blake2s(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        extract_blake2b(prk, salt, saltlen, ikm, ikmlen);
        break;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

