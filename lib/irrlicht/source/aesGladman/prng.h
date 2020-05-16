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

 This is the header file for an implementation of a random data pool based on
 the use of an external entropy function (inspired by Peter Gutmann's work).
*/

#ifndef _PRNG_H
#define _PRNG_H

#include "sha1.h"

#define PRNG_POOL_LEN    256    /* minimum random pool size             */
#define PRNG_MIN_MIX      20    /* min initial pool mixing iterations   */

/* ensure that pool length is a multiple of the SHA1 digest size        */

#define PRNG_POOL_SIZE  (SHA1_DIGEST_SIZE * (1 + (PRNG_POOL_LEN - 1) / SHA1_DIGEST_SIZE))

/* A function for providing entropy is a parameter in the prng_init()   */
/* call.  This function has the following form and returns a maximum    */
/* of 'len' bytes of pseudo random data in the buffer 'buf'.  It can    */
/* return less than 'len' bytes but will be repeatedly called for more  */
/* data in this case.                                                   */

typedef int (*prng_entropy_fn)(unsigned char buf[], unsigned int len);

typedef struct
{   unsigned char   rbuf[PRNG_POOL_SIZE];   /* the random pool          */
    unsigned char   obuf[PRNG_POOL_SIZE];   /* pool output buffer       */
    unsigned int    pos;                    /* output buffer position   */
    prng_entropy_fn entropy;                /* entropy function pointer */
} prng_ctx;

/* initialise the random stream generator   */
void prng_init(prng_entropy_fn fun, prng_ctx ctx[1]);

/* obtain random bytes from the generator   */
void prng_rand(unsigned char data[], unsigned int data_len, prng_ctx ctx[1]);

/* close the random stream generator        */
void prng_end(prng_ctx ctx[1]);

#endif

