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
 -------------------------------------------------------------------------
 Issue Date: 26/08/2003

 This file implements password based file encryption and authentication 
 using AES in CTR mode, HMAC-SHA1 authentication and RFC2898 password 
 based key derivation.

*/

#include <memory.h>

#include "fileenc.h"

/* subroutine for data encryption/decryption    */
/* this could be speeded up a lot by aligning   */
/* buffers and using 32 bit operations          */

static void encr_data(unsigned char data[], unsigned long d_len, fcrypt_ctx cx[1])
{
    unsigned long i = 0, pos = cx->encr_pos;

    while(i < d_len)
    {
        if(pos == BLOCK_SIZE)
        {   unsigned int j = 0;
            /* increment encryption nonce   */
            while(j < 8 && !++cx->nonce[j])
                ++j;
            /* encrypt the nonce to form next xor buffer    */
            aes_encrypt(cx->nonce, cx->encr_bfr, cx->encr_ctx);
            pos = 0;
        }

        data[i++] ^= cx->encr_bfr[pos++];
    }

    cx->encr_pos = pos;
}

int fcrypt_init(
    int mode,                               /* the mode to be used (input)          */
    const unsigned char pwd[],              /* the user specified password (input)  */
    unsigned int pwd_len,                   /* the length of the password (input)   */
    const unsigned char salt[],             /* the salt (input)                     */
#ifdef PASSWORD_VERIFIER
    unsigned char pwd_ver[PWD_VER_LENGTH],  /* 2 byte password verifier (output)    */
#endif
    fcrypt_ctx      cx[1])                  /* the file encryption context (output) */
{
    unsigned char kbuf[2 * MAX_KEY_LENGTH + PWD_VER_LENGTH];

    if(pwd_len > MAX_PWD_LENGTH)
        return PASSWORD_TOO_LONG;

    if(mode < 1 || mode > 3)
        return BAD_MODE;

    cx->mode = mode;
    cx->pwd_len = pwd_len;
    /* initialise the encryption nonce and buffer pos   */
    cx->encr_pos = BLOCK_SIZE;

    /* if we need a random component in the encryption  */
    /* nonce, this is where it would have to be set     */
    memset(cx->nonce, 0, BLOCK_SIZE * sizeof(unsigned char));
    /* initialise for authentication			        */
    hmac_sha_begin(cx->auth_ctx);

    /* derive the encryption and authetication keys and the password verifier   */
    derive_key(pwd, pwd_len, salt, SALT_LENGTH(mode), KEYING_ITERATIONS,
                        kbuf, 2 * KEY_LENGTH(mode) + PWD_VER_LENGTH);
    /* set the encryption key							*/
    aes_encrypt_key(kbuf, KEY_LENGTH(mode), cx->encr_ctx);
    /* set the authentication key						*/
    hmac_sha_key(kbuf + KEY_LENGTH(mode), KEY_LENGTH(mode), cx->auth_ctx);
#ifdef PASSWORD_VERIFIER
    memcpy(pwd_ver, kbuf + 2 * KEY_LENGTH(mode), PWD_VER_LENGTH);
#endif
    /* clear the buffer holding the derived key values	*/
    memset(kbuf, 0, 2 * KEY_LENGTH(mode) + PWD_VER_LENGTH);

    return GOOD_RETURN;
}

/* perform 'in place' encryption and authentication */

void fcrypt_encrypt(unsigned char data[], unsigned int data_len, fcrypt_ctx cx[1])
{
    encr_data(data, data_len, cx);
    hmac_sha_data(data, data_len, cx->auth_ctx);
}

/* perform 'in place' authentication and decryption */

void fcrypt_decrypt(unsigned char data[], unsigned int data_len, fcrypt_ctx cx[1])
{
    hmac_sha_data(data, data_len, cx->auth_ctx);
    encr_data(data, data_len, cx);
}

/* close encryption/decryption and return the MAC value */

int fcrypt_end(unsigned char mac[], fcrypt_ctx cx[1])
{
    unsigned int res = cx->mode;

    hmac_sha_end(mac, MAC_LENGTH(cx->mode), cx->auth_ctx);
    memset(cx, 0, sizeof(fcrypt_ctx));	/* clear the encryption context	*/
    return MAC_LENGTH(res);		/* return MAC length in bytes   */
}

