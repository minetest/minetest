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
*/

#ifndef _SHA1_H
#define _SHA1_H

#include <limits.h>

#define SHA1_BLOCK_SIZE  64
#define SHA1_DIGEST_SIZE 20

/* define an unsigned 32-bit type */

#if UINT_MAX == 0xffffffff
  typedef   unsigned int     sha1_32t;
#elif ULONG_MAX == 0xffffffff
  typedef   unsigned long    sha1_32t;
#else
#error Please define sha1_32t as an unsigned 32 bit type in sha2.h
#endif

/* type to hold the SHA256 context  */

typedef struct
{   sha1_32t count[2];
    sha1_32t hash[5];
    sha1_32t wbuf[16];
} sha1_ctx;

void sha1_compile(sha1_ctx ctx[1]);

void sha1_begin(sha1_ctx ctx[1]);
void sha1_hash(const unsigned char data[], unsigned long len, sha1_ctx ctx[1]);
void sha1_end(unsigned char hval[], sha1_ctx ctx[1]);
void sha1(unsigned char hval[], const unsigned char data[], unsigned long len);

#endif

