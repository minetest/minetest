/*
 ---------------------------------------------------------------------------
 Copyright (c) 2003, Dr Brian Gladman <                 >, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 26/08/2003

 This file contains the code for implementing the key schedule for AES
 (Rijndael) for block and key sizes of 16, 24, and 32 bytes. See aesopt.h
 for further details including optimisation.
*/

#include "aesopt.h"

/* Initialise the key schedule from the user supplied key. The key
   length can be specified in bytes, with legal values of 16, 24
   and 32, or in bits, with legal values of 128, 192 and 256. These
   values correspond with Nk values of 4, 6 and 8 respectively.

   The following macros implement a single cycle in the key
   schedule generation process. The number of cycles needed
   for each cx->n_col and nk value is:

    nk =             4  5  6  7  8
    ------------------------------
    cx->n_col = 4   10  9  8  7  7
    cx->n_col = 5   14 11 10  9  9
    cx->n_col = 6   19 15 12 11 11
    cx->n_col = 7   21 19 16 13 14
    cx->n_col = 8   29 23 19 17 14
*/

#define ke4(k,i) \
{   k[4*(i)+4] = ss[0] ^= ls_box(ss[3],3) ^ t_use(r,c)[i]; k[4*(i)+5] = ss[1] ^= ss[0]; \
    k[4*(i)+6] = ss[2] ^= ss[1]; k[4*(i)+7] = ss[3] ^= ss[2]; \
}
#define kel4(k,i) \
{   k[4*(i)+4] = ss[0] ^= ls_box(ss[3],3) ^ t_use(r,c)[i]; k[4*(i)+5] = ss[1] ^= ss[0]; \
    k[4*(i)+6] = ss[2] ^= ss[1]; k[4*(i)+7] = ss[3] ^= ss[2]; \
}

#define ke6(k,i) \
{   k[6*(i)+ 6] = ss[0] ^= ls_box(ss[5],3) ^ t_use(r,c)[i]; k[6*(i)+ 7] = ss[1] ^= ss[0]; \
    k[6*(i)+ 8] = ss[2] ^= ss[1]; k[6*(i)+ 9] = ss[3] ^= ss[2]; \
    k[6*(i)+10] = ss[4] ^= ss[3]; k[6*(i)+11] = ss[5] ^= ss[4]; \
}
#define kel6(k,i) \
{   k[6*(i)+ 6] = ss[0] ^= ls_box(ss[5],3) ^ t_use(r,c)[i]; k[6*(i)+ 7] = ss[1] ^= ss[0]; \
    k[6*(i)+ 8] = ss[2] ^= ss[1]; k[6*(i)+ 9] = ss[3] ^= ss[2]; \
}

#define ke8(k,i) \
{   k[8*(i)+ 8] = ss[0] ^= ls_box(ss[7],3) ^ t_use(r,c)[i]; k[8*(i)+ 9] = ss[1] ^= ss[0]; \
    k[8*(i)+10] = ss[2] ^= ss[1]; k[8*(i)+11] = ss[3] ^= ss[2]; \
    k[8*(i)+12] = ss[4] ^= ls_box(ss[3],0); k[8*(i)+13] = ss[5] ^= ss[4]; \
    k[8*(i)+14] = ss[6] ^= ss[5]; k[8*(i)+15] = ss[7] ^= ss[6]; \
}
#define kel8(k,i) \
{   k[8*(i)+ 8] = ss[0] ^= ls_box(ss[7],3) ^ t_use(r,c)[i]; k[8*(i)+ 9] = ss[1] ^= ss[0]; \
    k[8*(i)+10] = ss[2] ^= ss[1]; k[8*(i)+11] = ss[3] ^= ss[2]; \
}

#if defined(ENCRYPTION_KEY_SCHEDULE)

#if defined(AES_128) || defined(AES_VAR)

aes_rval aes_encrypt_key128(const void *in_key, aes_encrypt_ctx cx[1])
{   aes_32t    ss[4];

    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);

#if ENC_UNROLL == NONE
    {   aes_32t i;

        for(i = 0; i < ((11 * N_COLS - 1) / 4); ++i)
            ke4(cx->ks, i);
    }
#else
    ke4(cx->ks, 0);  ke4(cx->ks, 1);
    ke4(cx->ks, 2);  ke4(cx->ks, 3);
    ke4(cx->ks, 4);  ke4(cx->ks, 5);
    ke4(cx->ks, 6);  ke4(cx->ks, 7);
    ke4(cx->ks, 8); kel4(cx->ks, 9);
#endif

    /* cx->ks[45] ^ cx->ks[52] ^ cx->ks[53] is zero for a 256 bit       */
    /* key and must be non-zero for 128 and 192 bits keys   */
    cx->ks[53] = cx->ks[45] = 0;
    cx->ks[52] = 10;
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_192) || defined(AES_VAR)

aes_rval aes_encrypt_key192(const void *in_key, aes_encrypt_ctx cx[1])
{   aes_32t    ss[6];

    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);
    cx->ks[4] = ss[4] = word_in(in_key, 4);
    cx->ks[5] = ss[5] = word_in(in_key, 5);

#if ENC_UNROLL == NONE
    {   aes_32t i;

        for(i = 0; i < (13 * N_COLS - 1) / 6; ++i)
            ke6(cx->ks, i);
    }
#else
    ke6(cx->ks, 0);  ke6(cx->ks, 1);
    ke6(cx->ks, 2);  ke6(cx->ks, 3);
    ke6(cx->ks, 4);  ke6(cx->ks, 5);
    ke6(cx->ks, 6); kel6(cx->ks, 7);
#endif

    /* cx->ks[45] ^ cx->ks[52] ^ cx->ks[53] is zero for a 256 bit       */
    /* key and must be non-zero for 128 and 192 bits keys   */
    cx->ks[53] = cx->ks[45];
    cx->ks[52] = 12;
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_256) || defined(AES_VAR)

aes_rval aes_encrypt_key256(const void *in_key, aes_encrypt_ctx cx[1])
{   aes_32t    ss[8];

    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);
    cx->ks[4] = ss[4] = word_in(in_key, 4);
    cx->ks[5] = ss[5] = word_in(in_key, 5);
    cx->ks[6] = ss[6] = word_in(in_key, 6);
    cx->ks[7] = ss[7] = word_in(in_key, 7);

#if ENC_UNROLL == NONE
    {   aes_32t i;

        for(i = 0; i < (15 * N_COLS - 1) / 8; ++i)
            ke8(cx->ks,  i);
    }
#else
    ke8(cx->ks, 0); ke8(cx->ks, 1);
    ke8(cx->ks, 2); ke8(cx->ks, 3);
    ke8(cx->ks, 4); ke8(cx->ks, 5);
    kel8(cx->ks, 6);
#endif
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_VAR)

aes_rval aes_encrypt_key(const void *in_key, int key_len, aes_encrypt_ctx cx[1])
{
    switch(key_len)
    {
#ifdef AES_ERR_CHK
    case 16: case 128: return aes_encrypt_key128(in_key, cx);
    case 24: case 192: return aes_encrypt_key192(in_key, cx);
    case 32: case 256: return aes_encrypt_key256(in_key, cx);
    default: return aes_error;
#else
    case 16: case 128: aes_encrypt_key128(in_key, cx); return;
    case 24: case 192: aes_encrypt_key192(in_key, cx); return;
    case 32: case 256: aes_encrypt_key256(in_key, cx); return;
#endif
    }
}

#endif

#endif

#if defined(DECRYPTION_KEY_SCHEDULE)

#if DEC_ROUND == NO_TABLES
#define ff(x)   (x)
#else
#define ff(x)   inv_mcol(x)
#ifdef  dec_imvars
#define d_vars  dec_imvars
#endif
#endif

#if 1
#define kdf4(k,i) \
{   ss[0] = ss[0] ^ ss[2] ^ ss[1] ^ ss[3]; ss[1] = ss[1] ^ ss[3]; ss[2] = ss[2] ^ ss[3]; ss[3] = ss[3]; \
    ss[4] = ls_box(ss[(i+3) % 4], 3) ^ t_use(r,c)[i]; ss[i % 4] ^= ss[4]; \
    ss[4] ^= k[4*(i)];   k[4*(i)+4] = ff(ss[4]); ss[4] ^= k[4*(i)+1]; k[4*(i)+5] = ff(ss[4]); \
    ss[4] ^= k[4*(i)+2]; k[4*(i)+6] = ff(ss[4]); ss[4] ^= k[4*(i)+3]; k[4*(i)+7] = ff(ss[4]); \
}
#define kd4(k,i) \
{   ss[4] = ls_box(ss[(i+3) % 4], 3) ^ t_use(r,c)[i]; ss[i % 4] ^= ss[4]; ss[4] = ff(ss[4]); \
    k[4*(i)+4] = ss[4] ^= k[4*(i)]; k[4*(i)+5] = ss[4] ^= k[4*(i)+1]; \
    k[4*(i)+6] = ss[4] ^= k[4*(i)+2]; k[4*(i)+7] = ss[4] ^= k[4*(i)+3]; \
}
#define kdl4(k,i) \
{   ss[4] = ls_box(ss[(i+3) % 4], 3) ^ t_use(r,c)[i]; ss[i % 4] ^= ss[4]; \
    k[4*(i)+4] = (ss[0] ^= ss[1]) ^ ss[2] ^ ss[3]; k[4*(i)+5] = ss[1] ^ ss[3]; \
    k[4*(i)+6] = ss[0]; k[4*(i)+7] = ss[1]; \
}
#else
#define kdf4(k,i) \
{   ss[0] ^= ls_box(ss[3],3) ^ t_use(r,c)[i]; k[4*(i)+ 4] = ff(ss[0]); ss[1] ^= ss[0]; k[4*(i)+ 5] = ff(ss[1]); \
    ss[2] ^= ss[1]; k[4*(i)+ 6] = ff(ss[2]); ss[3] ^= ss[2]; k[4*(i)+ 7] = ff(ss[3]); \
}
#define kd4(k,i) \
{   ss[4] = ls_box(ss[3],3) ^ t_use(r,c)[i]; \
    ss[0] ^= ss[4]; ss[4] = ff(ss[4]); k[4*(i)+ 4] = ss[4] ^= k[4*(i)]; \
    ss[1] ^= ss[0]; k[4*(i)+ 5] = ss[4] ^= k[4*(i)+ 1]; \
    ss[2] ^= ss[1]; k[4*(i)+ 6] = ss[4] ^= k[4*(i)+ 2]; \
    ss[3] ^= ss[2]; k[4*(i)+ 7] = ss[4] ^= k[4*(i)+ 3]; \
}
#define kdl4(k,i) \
{   ss[0] ^= ls_box(ss[3],3) ^ t_use(r,c)[i]; k[4*(i)+ 4] = ss[0]; ss[1] ^= ss[0]; k[4*(i)+ 5] = ss[1]; \
    ss[2] ^= ss[1]; k[4*(i)+ 6] = ss[2]; ss[3] ^= ss[2]; k[4*(i)+ 7] = ss[3]; \
}
#endif

#define kdf6(k,i) \
{   ss[0] ^= ls_box(ss[5],3) ^ t_use(r,c)[i]; k[6*(i)+ 6] = ff(ss[0]); ss[1] ^= ss[0]; k[6*(i)+ 7] = ff(ss[1]); \
    ss[2] ^= ss[1]; k[6*(i)+ 8] = ff(ss[2]); ss[3] ^= ss[2]; k[6*(i)+ 9] = ff(ss[3]); \
    ss[4] ^= ss[3]; k[6*(i)+10] = ff(ss[4]); ss[5] ^= ss[4]; k[6*(i)+11] = ff(ss[5]); \
}
#define kd6(k,i) \
{   ss[6] = ls_box(ss[5],3) ^ t_use(r,c)[i]; \
    ss[0] ^= ss[6]; ss[6] = ff(ss[6]); k[6*(i)+ 6] = ss[6] ^= k[6*(i)]; \
    ss[1] ^= ss[0]; k[6*(i)+ 7] = ss[6] ^= k[6*(i)+ 1]; \
    ss[2] ^= ss[1]; k[6*(i)+ 8] = ss[6] ^= k[6*(i)+ 2]; \
    ss[3] ^= ss[2]; k[6*(i)+ 9] = ss[6] ^= k[6*(i)+ 3]; \
    ss[4] ^= ss[3]; k[6*(i)+10] = ss[6] ^= k[6*(i)+ 4]; \
    ss[5] ^= ss[4]; k[6*(i)+11] = ss[6] ^= k[6*(i)+ 5]; \
}
#define kdl6(k,i) \
{   ss[0] ^= ls_box(ss[5],3) ^ t_use(r,c)[i]; k[6*(i)+ 6] = ss[0]; ss[1] ^= ss[0]; k[6*(i)+ 7] = ss[1]; \
    ss[2] ^= ss[1]; k[6*(i)+ 8] = ss[2]; ss[3] ^= ss[2]; k[6*(i)+ 9] = ss[3]; \
}

#define kdf8(k,i) \
{   ss[0] ^= ls_box(ss[7],3) ^ t_use(r,c)[i]; k[8*(i)+ 8] = ff(ss[0]); ss[1] ^= ss[0]; k[8*(i)+ 9] = ff(ss[1]); \
    ss[2] ^= ss[1]; k[8*(i)+10] = ff(ss[2]); ss[3] ^= ss[2]; k[8*(i)+11] = ff(ss[3]); \
    ss[4] ^= ls_box(ss[3],0); k[8*(i)+12] = ff(ss[4]); ss[5] ^= ss[4]; k[8*(i)+13] = ff(ss[5]); \
    ss[6] ^= ss[5]; k[8*(i)+14] = ff(ss[6]); ss[7] ^= ss[6]; k[8*(i)+15] = ff(ss[7]); \
}
#define kd8(k,i) \
{   aes_32t g = ls_box(ss[7],3) ^ t_use(r,c)[i]; \
    ss[0] ^= g; g = ff(g); k[8*(i)+ 8] = g ^= k[8*(i)]; \
    ss[1] ^= ss[0]; k[8*(i)+ 9] = g ^= k[8*(i)+ 1]; \
    ss[2] ^= ss[1]; k[8*(i)+10] = g ^= k[8*(i)+ 2]; \
    ss[3] ^= ss[2]; k[8*(i)+11] = g ^= k[8*(i)+ 3]; \
    g = ls_box(ss[3],0); \
    ss[4] ^= g; g = ff(g); k[8*(i)+12] = g ^= k[8*(i)+ 4]; \
    ss[5] ^= ss[4]; k[8*(i)+13] = g ^= k[8*(i)+ 5]; \
    ss[6] ^= ss[5]; k[8*(i)+14] = g ^= k[8*(i)+ 6]; \
    ss[7] ^= ss[6]; k[8*(i)+15] = g ^= k[8*(i)+ 7]; \
}
#define kdl8(k,i) \
{   ss[0] ^= ls_box(ss[7],3) ^ t_use(r,c)[i]; k[8*(i)+ 8] = ss[0]; ss[1] ^= ss[0]; k[8*(i)+ 9] = ss[1]; \
    ss[2] ^= ss[1]; k[8*(i)+10] = ss[2]; ss[3] ^= ss[2]; k[8*(i)+11] = ss[3]; \
}

#if defined(AES_128) || defined(AES_VAR)

aes_rval aes_decrypt_key128(const void *in_key, aes_decrypt_ctx cx[1])
{   aes_32t    ss[5];
#ifdef  d_vars
        d_vars;
#endif
    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);

#if DEC_UNROLL == NONE
    {   aes_32t i;

        for(i = 0; i < (11 * N_COLS - 1) / 4; ++i)
            ke4(cx->ks, i);
#if !(DEC_ROUND == NO_TABLES)
        for(i = N_COLS; i < 10 * N_COLS; ++i)
            cx->ks[i] = inv_mcol(cx->ks[i]);
#endif
    }
#else
    kdf4(cx->ks, 0);  kd4(cx->ks, 1);
     kd4(cx->ks, 2);  kd4(cx->ks, 3);
     kd4(cx->ks, 4);  kd4(cx->ks, 5);
     kd4(cx->ks, 6);  kd4(cx->ks, 7);
     kd4(cx->ks, 8); kdl4(cx->ks, 9);
#endif

    /* cx->ks[45] ^ cx->ks[52] ^ cx->ks[53] is zero for a 256 bit       */
    /* key and must be non-zero for 128 and 192 bits keys   */
    cx->ks[53] = cx->ks[45] = 0;
    cx->ks[52] = 10;
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_192) || defined(AES_VAR)

aes_rval aes_decrypt_key192(const void *in_key, aes_decrypt_ctx cx[1])
{   aes_32t    ss[7];
#ifdef  d_vars
        d_vars;
#endif
    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);

#if DEC_UNROLL == NONE
    cx->ks[4] = ss[4] = word_in(in_key, 4);
    cx->ks[5] = ss[5] = word_in(in_key, 5);
    {   aes_32t i;

        for(i = 0; i < (13 * N_COLS - 1) / 6; ++i)
            ke6(cx->ks, i);
#if !(DEC_ROUND == NO_TABLES)
        for(i = N_COLS; i < 12 * N_COLS; ++i)
            cx->ks[i] = inv_mcol(cx->ks[i]);
#endif
    }
#else
	ss[4] = word_in(in_key, 4);
    cx->ks[4] = ff(ss[4]);
    ss[5] = word_in(in_key, 5);
    cx->ks[5] = ff(ss[5]);
    kdf6(cx->ks, 0); kd6(cx->ks, 1);
    kd6(cx->ks, 2);  kd6(cx->ks, 3);
    kd6(cx->ks, 4);  kd6(cx->ks, 5);
    kd6(cx->ks, 6); kdl6(cx->ks, 7);
#endif

    /* cx->ks[45] ^ cx->ks[52] ^ cx->ks[53] is zero for a 256 bit       */
    /* key and must be non-zero for 128 and 192 bits keys   */
    cx->ks[53] = cx->ks[45];
    cx->ks[52] = 12;
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_256) || defined(AES_VAR)

aes_rval aes_decrypt_key256(const void *in_key, aes_decrypt_ctx cx[1])
{   aes_32t    ss[8];
#ifdef  d_vars
        d_vars;
#endif
    cx->ks[0] = ss[0] = word_in(in_key, 0);
    cx->ks[1] = ss[1] = word_in(in_key, 1);
    cx->ks[2] = ss[2] = word_in(in_key, 2);
    cx->ks[3] = ss[3] = word_in(in_key, 3);

#if DEC_UNROLL == NONE
    cx->ks[4] = ss[4] = word_in(in_key, 4);
    cx->ks[5] = ss[5] = word_in(in_key, 5);
    cx->ks[6] = ss[6] = word_in(in_key, 6);
    cx->ks[7] = ss[7] = word_in(in_key, 7);
    {   aes_32t i;

        for(i = 0; i < (15 * N_COLS - 1) / 8; ++i)
            ke8(cx->ks,  i);
#if !(DEC_ROUND == NO_TABLES)
        for(i = N_COLS; i < 14 * N_COLS; ++i)
            cx->ks[i] = inv_mcol(cx->ks[i]);
#endif
    }
#else
	ss[4] = word_in(in_key, 4);
    cx->ks[4] = ff(ss[4]);
    ss[5] = word_in(in_key, 5);
    cx->ks[5] = ff(ss[5]);
    ss[6] = word_in(in_key, 6);
    cx->ks[6] = ff(ss[6]);
    ss[7] = word_in(in_key, 7);
    cx->ks[7] = ff(ss[7]);
    kdf8(cx->ks, 0); kd8(cx->ks, 1);
    kd8(cx->ks, 2);  kd8(cx->ks, 3);
    kd8(cx->ks, 4);  kd8(cx->ks, 5);
    kdl8(cx->ks, 6);
#endif
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(AES_VAR)

aes_rval aes_decrypt_key(const void *in_key, int key_len, aes_decrypt_ctx cx[1])
{
    switch(key_len)
    {
#ifdef AES_ERR_CHK
    case 16: case 128: return aes_decrypt_key128(in_key, cx);
    case 24: case 192: return aes_decrypt_key192(in_key, cx);
    case 32: case 256: return aes_decrypt_key256(in_key, cx);
    default: return aes_error;
#else
    case 16: case 128: aes_decrypt_key128(in_key, cx); return;
    case 24: case 192: aes_decrypt_key192(in_key, cx); return;
    case 32: case 256: aes_decrypt_key256(in_key, cx); return;
#endif
    }
}

#endif

#endif

