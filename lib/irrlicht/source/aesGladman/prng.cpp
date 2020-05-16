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

 This file implements a random data pool based on the use of an external
 entropy function.  It is based on the ideas advocated by Peter Gutmann in
 his work on pseudo random sequence generators.  It is not a 'paranoid'
 random sequence generator and no attempt is made to protect the pool
 from prying eyes either by memory locking or by techniques to obscure
 its location in memory.
*/

#include <memory.h>
#include "prng.h"

/* mix a random data pool using the SHA1 compression function (as   */
/* suggested by Peter Gutmann in his paper on random pools)         */

static void prng_mix(unsigned char buf[])
{   unsigned int    i, len;
    sha1_ctx        ctx[1];

    /*lint -e{663}  unusual array to pointer conversion */
    for(i = 0; i < PRNG_POOL_SIZE; i += SHA1_DIGEST_SIZE)
    {
        /* copy digest size pool block into SHA1 hash block */
        memcpy(ctx->hash, buf + (i ? i : PRNG_POOL_SIZE)
                            - SHA1_DIGEST_SIZE, SHA1_DIGEST_SIZE);

        /* copy data from pool into the SHA1 data buffer    */
        len = PRNG_POOL_SIZE - i;
        memcpy(ctx->wbuf, buf + i, (len > SHA1_BLOCK_SIZE ? SHA1_BLOCK_SIZE : len));

        if(len < SHA1_BLOCK_SIZE)
            memcpy(((char*)ctx->wbuf) + len, buf, SHA1_BLOCK_SIZE - len);

        /* compress using the SHA1 compression function     */
        sha1_compile(ctx);

        /* put digest size block back into the random pool  */
        memcpy(buf + i, ctx->hash, SHA1_DIGEST_SIZE);
    }
}

/* refresh the output buffer and update the random pool by adding   */
/* entropy and remixing                                             */

static void update_pool(prng_ctx ctx[1])
{   unsigned int    i = 0;

    /* transfer random pool data to the output buffer   */
    memcpy(ctx->obuf, ctx->rbuf, PRNG_POOL_SIZE);

    /* enter entropy data into the pool */
    while(i < PRNG_POOL_SIZE)
        i += ctx->entropy(ctx->rbuf + i, PRNG_POOL_SIZE - i);

    /* invert and xor the original pool data into the pool  */
    for(i = 0; i < PRNG_POOL_SIZE; ++i)
        ctx->rbuf[i] ^= ~ctx->obuf[i];

    /* mix the pool and the output buffer   */
    prng_mix(ctx->rbuf);
    prng_mix(ctx->obuf);
}

void prng_init(prng_entropy_fn fun, prng_ctx ctx[1])
{   int i;

    /* clear the buffers and the counter in the context     */
    memset(ctx, 0, sizeof(prng_ctx));

    /* set the pointer to the entropy collection function   */
    ctx->entropy = fun;

    /* initialise the random data pool                      */
    update_pool(ctx);

    /* mix the pool a minimum number of times               */
    for(i = 0; i < PRNG_MIN_MIX; ++i)
        prng_mix(ctx->rbuf);

    /* update the pool to prime the pool output buffer      */
    update_pool(ctx);
}

/* provide random bytes from the random data pool   */

void prng_rand(unsigned char data[], unsigned int data_len, prng_ctx ctx[1])
{   unsigned char   *rp = data;
    unsigned int    len, pos = ctx->pos;

    while(data_len)
    {
        /* transfer 'data_len' bytes (or the number of bytes remaining  */
        /* the pool output buffer if less) into the output              */
        len = (data_len < PRNG_POOL_SIZE - pos ? data_len : PRNG_POOL_SIZE - pos);
        memcpy(rp, ctx->obuf + pos, len);
        rp += len;          /* update ouput buffer position pointer     */
        pos += len;         /* update pool output buffer pointer        */
        data_len -= len;    /* update the remaining data count          */

        /* refresh the random pool if necessary */
        if(pos == PRNG_POOL_SIZE)
        {
            update_pool(ctx); pos = 0;
        }
    }

    ctx->pos = pos;
}

void prng_end(prng_ctx ctx[1])
{
    /* ensure the data in the context is destroyed  */
    memset(ctx, 0, sizeof(prng_ctx));
}

