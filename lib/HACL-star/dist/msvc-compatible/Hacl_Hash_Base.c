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


#include "Hacl_Hash_Base.h"

uint32_t Hacl_Hash_Definitions_word_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 4U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 4U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 4U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 4U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 8U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 8U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

uint32_t Hacl_Hash_Definitions_block_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 128U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 128U;
      }
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 144U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 136U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 104U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 72U;
      }
    case Spec_Hash_Definitions_Shake128:
      {
        return 168U;
      }
    case Spec_Hash_Definitions_Shake256:
      {
        return 136U;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        return 128U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

uint32_t Hacl_Hash_Definitions_hash_word_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 4U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 5U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 7U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 8U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 6U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 8U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

uint32_t Hacl_Hash_Definitions_hash_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 16U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 20U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 28U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 48U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 28U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 48U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 64U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

