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
 Issue Date: 26/08/2003
 Includes a bugfix from Dr Brian Gladman made on 16/04/2012 for compiling on 64-bit

 This is an implementation of HMAC, the FIPS standard keyed hash function
*/

#ifndef _HMAC_H
#define _HMAC_H

#include <memory.h>

#define USE_SHA1	// Irrlicht only cares about SHA1 for now
#if !defined(USE_SHA1) && !defined(USE_SHA256)
#error define USE_SHA1 or USE_SHA256 to set the HMAC hash algorithm
#endif

#ifdef USE_SHA1

#include "sha1.h"

#define HMAC_HASH_INPUT_SIZE    SHA1_BLOCK_SIZE
#define HMAC_HASH_OUTPUT_SIZE   SHA1_DIGEST_SIZE
#define sha_ctx             	sha1_ctx
#define sha_begin           	sha1_begin
#define sha_hash            	sha1_hash
#define sha_end             	sha1_end

#endif

#ifdef USE_SHA256

#include "sha2.h"

#define HMAC_HASH_INPUT_SIZE     SHA256_BLOCK_SIZE
#define HMAC_HASH_OUTPUT_SIZE    SHA256_DIGEST_SIZE
#define sha_ctx             sha256_ctx
#define sha_begin           sha256_begin
#define sha_hash            sha256_hash
#define sha_end             sha256_end

#endif

#define HMAC_OK                0
#define HMAC_BAD_MODE         -1
#define HMAC_IN_DATA  0xffffffff

typedef struct
{   unsigned char   key[HMAC_HASH_INPUT_SIZE];
    sha_ctx         ctx[1];
    unsigned long   klen;
} hmac_ctx;

void hmac_sha_begin(hmac_ctx cx[1]);

int  hmac_sha_key(const unsigned char key[], unsigned long key_len, hmac_ctx cx[1]);

void hmac_sha_data(const unsigned char data[], unsigned long data_len, hmac_ctx cx[1]);

void hmac_sha_end(unsigned char mac[], unsigned long mac_len, hmac_ctx cx[1]);

void hmac_sha(const unsigned char key[], unsigned long key_len,
          const unsigned char data[], unsigned long data_len,
          unsigned char mac[], unsigned long mac_len);

#endif
