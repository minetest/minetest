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

 This file contains the definitions required to use AES in C. See aesopt.h
 for optimisation details.
*/

#ifndef _AES_H
#define _AES_H

#include "irrMath.h"

#define AES_128     /* define if AES with 128 bit keys is needed    */
#define AES_192     /* define if AES with 192 bit keys is needed    */
#define AES_256     /* define if AES with 256 bit keys is needed    */
#define AES_VAR     /* define if a variable key size is needed      */

/* The following must also be set in assembler files if being used  */

#define AES_ENCRYPT /* if support for encryption is needed          */
#define AES_DECRYPT /* if support for decryption is needed          */
#define AES_ERR_CHK /* for parameter checks & error return codes    */

typedef irr::u8 aes_08t;
typedef irr::u32 aes_32t;

#define AES_BLOCK_SIZE  16  /* the AES block size in bytes          */
#define N_COLS           4  /* the number of columns in the state   */

/* a maximum of 60 32-bit words are needed for the key schedule		*/
#define KS_LENGTH       64

#ifdef  AES_ERR_CHK
#define aes_ret     int
#define aes_good    0
#define aes_error  -1
#else
#define aes_ret     void
#endif

#ifndef AES_DLL                 /* implement normal/DLL functions   */
#define aes_rval    aes_ret
#else
#define aes_rval    aes_ret __declspec(dllexport) _stdcall
#endif

/* This routine must be called before first use if non-static       */
/* tables are being used                                            */

void gen_tabs(void);

/* The key length (klen) is input in bytes when it is in the range  */
/* 16 <= klen <= 32 or in bits when in the range 128 <= klen <= 256 */

#ifdef  AES_ENCRYPT

typedef struct  
{
	aes_32t ks[KS_LENGTH];
} aes_encrypt_ctx;

#if defined(AES_128) || defined(AES_VAR)
aes_rval aes_encrypt_key128(const void *in_key, aes_encrypt_ctx cx[1]);
#endif

#if defined(AES_192) || defined(AES_VAR)
aes_rval aes_encrypt_key192(const void *in_key, aes_encrypt_ctx cx[1]);
#endif

#if defined(AES_256) || defined(AES_VAR)
aes_rval aes_encrypt_key256(const void *in_key, aes_encrypt_ctx cx[1]);
#endif

#if defined(AES_VAR)
aes_rval aes_encrypt_key(const void *in_key, int key_len, aes_encrypt_ctx cx[1]);
#endif

aes_rval aes_encrypt(const void *in_blk, void *out_blk, const aes_encrypt_ctx cx[1]);
#endif

#ifdef AES_DECRYPT

typedef struct  
{
	aes_32t ks[KS_LENGTH];
} aes_decrypt_ctx;

#if defined(AES_128) || defined(AES_VAR)
aes_rval aes_decrypt_key128(const void *in_key, aes_decrypt_ctx cx[1]);
#endif

#if defined(AES_192) || defined(AES_VAR)
aes_rval aes_decrypt_key192(const void *in_key, aes_decrypt_ctx cx[1]);
#endif

#if defined(AES_256) || defined(AES_VAR)
aes_rval aes_decrypt_key256(const void *in_key, aes_decrypt_ctx cx[1]);
#endif

#if defined(AES_VAR)
aes_rval aes_decrypt_key(const void *in_key, int key_len, aes_decrypt_ctx cx[1]);
#endif

aes_rval aes_decrypt(const void *in_blk, void *out_blk, const aes_decrypt_ctx cx[1]);
#endif

#endif

