/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman <                 >, Worcester, UK.
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
 Issue Date: 24/01/2003

 This file contains the header file for fileenc.c, which implements password
 based file encryption and authentication using AES in CTR mode, HMAC-SHA1 
 authentication and RFC2898 password based key derivation.
*/

#ifndef _FENC_H
#define _FENC_H

#include "aes.h"
#include "hmac.h"
#include "pwd2key.h"

#define	BLOCK_SIZE AES_BLOCK_SIZE
#define PASSWORD_VERIFIER

#define MAX_KEY_LENGTH        32
#define MAX_PWD_LENGTH       128
#define MAX_SALT_LENGTH       16
#define KEYING_ITERATIONS   1000

#ifdef  PASSWORD_VERIFIER
#define PWD_VER_LENGTH         2
#else
#define PWD_VER_LENGTH         0
#endif

#define GOOD_RETURN            0
#define PASSWORD_TOO_LONG   -100
#define BAD_MODE            -101

/*
    Field lengths (in bytes) versus File Encryption Mode (0 < mode < 4)

    Mode Key Salt  MAC Overhead
       1  16    8   10       18
       2  24   12   10       22
       3  32   16   10       26

   The following macros assume that the mode value is correct.
*/

#define KEY_LENGTH(mode)        (8 * (mode & 3) + 8)
#define SALT_LENGTH(mode)       (4 * (mode & 3) + 4)
#define MAC_LENGTH(mode)        (10)

/* the context for file encryption   */

typedef struct
{   unsigned char   nonce[BLOCK_SIZE];          /* the CTR nonce          */
    unsigned char   encr_bfr[BLOCK_SIZE];       /* encrypt buffer         */
    aes_encrypt_ctx encr_ctx[1];                /* encryption context     */
    hmac_ctx        auth_ctx[1];                /* authentication context */
    unsigned int    encr_pos;                   /* block position (enc)   */
    unsigned int    pwd_len;                    /* password length        */
    unsigned int    mode;                       /* File encryption mode   */
} fcrypt_ctx;

/* initialise file encryption or decryption */

int fcrypt_init(
    int mode,                               /* the mode to be used (input)          */
    const unsigned char pwd[],              /* the user specified password (input)  */
    unsigned int pwd_len,                   /* the length of the password (input)   */
    const unsigned char salt[],             /* the salt (input)                     */
#ifdef PASSWORD_VERIFIER
    unsigned char pwd_ver[PWD_VER_LENGTH],  /* 2 byte password verifier (output)    */
#endif
    fcrypt_ctx      cx[1]);                 /* the file encryption context (output) */

/* perform 'in place' encryption or decryption and authentication               */

void fcrypt_encrypt(unsigned char data[], unsigned int data_len, fcrypt_ctx cx[1]);
void fcrypt_decrypt(unsigned char data[], unsigned int data_len, fcrypt_ctx cx[1]);

/* close encryption/decryption and return the MAC value */
/* the return value is the length of the MAC            */

int fcrypt_end(unsigned char mac[],     /* the MAC value (output)   */
               fcrypt_ctx cx[1]);       /* the context (input)      */

#endif

