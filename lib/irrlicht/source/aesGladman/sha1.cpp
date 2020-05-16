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

 This is a byte oriented version of SHA1 that operates on arrays of bytes
 stored in memory. It runs at 22 cycles per byte on a Pentium P4 processor
*/

#include <string.h>     /* for memcpy() etc.        */
#include <stdlib.h>     /* for _lrotl with VC++     */

#include "sha1.h"
#include "../os.h"

/*
    To obtain the highest speed on processors with 32-bit words, this code 
    needs to determine the order in which bytes are packed into such words.
    The following block of code is an attempt to capture the most obvious 
    ways in which various environemnts specify their endian definitions. 
    It may well fail, in which case the definitions will need to be set by 
    editing at the points marked **** EDIT HERE IF NECESSARY **** below.
*/

/*  BYTE ORDER IN 32-BIT WORDS

    To obtain the highest speed on processors with 32-bit words, this code
    needs to determine the byte order of the target machine. The following 
    block of code is an attempt to capture the most obvious ways in which 
    various environemnts define byte order. It may well fail, in which case 
    the definitions will need to be set by editing at the points marked 
    **** EDIT HERE IF NECESSARY **** below.  My thanks to Peter Gutmann for 
    some of these defines (from cryptlib).
*/

#define BRG_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */
#define BRG_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */

#ifdef __BIG_ENDIAN__
#define PLATFORM_BYTE_ORDER BRG_BIG_ENDIAN
#else
#define PLATFORM_BYTE_ORDER BRG_LITTLE_ENDIAN
#endif

#define rotl32(x,n) (((x) << n) | ((x) >> (32 - n)))

#if (PLATFORM_BYTE_ORDER == BRG_BIG_ENDIAN)
#define swap_b32(x) (x)
#else
#define swap_b32(x) irr::os::Byteswap::byteswap(x)
#endif

#define SHA1_MASK   (SHA1_BLOCK_SIZE - 1)

#if 1

#define ch(x,y,z)       (((x) & (y)) ^ (~(x) & (z)))
#define parity(x,y,z)   ((x) ^ (y) ^ (z))
#define maj(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#else   /* Discovered Rich Schroeppel and Colin Plumb   */

#define ch(x,y,z)       ((z) ^ ((x) & ((y) ^ (z))))
#define parity(x,y,z)   ((x) ^ (y) ^ (z))
#define maj(x,y,z)      (((x) & (y)) | ((z) & ((x) ^ (y))))

#endif

/* A normal version as set out in the FIPS  */

#define rnd(f,k)    \
    t = a; a = rotl32(a,5) + f(b,c,d) + e + k + w[i]; \
    e = d; d = c; c = rotl32(b, 30); b = t

void sha1_compile(sha1_ctx ctx[1])
{   sha1_32t    w[80], i, a, b, c, d, e, t;

    /* note that words are compiled from the buffer into 32-bit */
    /* words in big-endian order so an order reversal is needed */
    /* here on little endian machines                           */
    for(i = 0; i < SHA1_BLOCK_SIZE / 4; ++i)
        w[i] = swap_b32(ctx->wbuf[i]);

    for(i = SHA1_BLOCK_SIZE / 4; i < 80; ++i)
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    a = ctx->hash[0];
    b = ctx->hash[1];
    c = ctx->hash[2];
    d = ctx->hash[3];
    e = ctx->hash[4];

    for(i = 0; i < 20; ++i)
    {
        rnd(ch, 0x5a827999);    
    }

    for(i = 20; i < 40; ++i)
    {
        rnd(parity, 0x6ed9eba1);
    }

    for(i = 40; i < 60; ++i)
    {
        rnd(maj, 0x8f1bbcdc);
    }

    for(i = 60; i < 80; ++i)
    {
        rnd(parity, 0xca62c1d6);
    }

    ctx->hash[0] += a; 
    ctx->hash[1] += b; 
    ctx->hash[2] += c; 
    ctx->hash[3] += d; 
    ctx->hash[4] += e;
}

void sha1_begin(sha1_ctx ctx[1])
{
    ctx->count[0] = ctx->count[1] = 0;
    ctx->hash[0] = 0x67452301;
    ctx->hash[1] = 0xefcdab89;
    ctx->hash[2] = 0x98badcfe;
    ctx->hash[3] = 0x10325476;
    ctx->hash[4] = 0xc3d2e1f0;
}

/* SHA1 hash data in an array of bytes into hash buffer and */
/* call the hash_compile function as required.              */

void sha1_hash(const unsigned char data[], unsigned long len, sha1_ctx ctx[1])
{   sha1_32t pos = (sha1_32t)(ctx->count[0] & SHA1_MASK), 
             space = SHA1_BLOCK_SIZE - pos;
    const unsigned char *sp = data;

    if((ctx->count[0] += len) < len)
        ++(ctx->count[1]);

    while(len >= space)     /* tranfer whole blocks if possible  */
    {
        memcpy(((unsigned char*)ctx->wbuf) + pos, sp, space);
        sp += space; len -= space; space = SHA1_BLOCK_SIZE; pos = 0; 
        sha1_compile(ctx);
    }

    /*lint -e{803} conceivable data overrun */
    memcpy(((unsigned char*)ctx->wbuf) + pos, sp, len);
}

/* SHA1 final padding and digest calculation  */

#if (PLATFORM_BYTE_ORDER == BRG_LITTLE_ENDIAN)
static sha1_32t  mask[4] = 
    {   0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
static sha1_32t  bits[4] = 
    {   0x00000080, 0x00008000, 0x00800000, 0x80000000 };
#else
static sha1_32t  mask[4] = 
    {   0x00000000, 0xff000000, 0xffff0000, 0xffffff00 };
static sha1_32t  bits[4] = 
    {   0x80000000, 0x00800000, 0x00008000, 0x00000080 };
#endif

void sha1_end(unsigned char hval[], sha1_ctx ctx[1])
{   sha1_32t    i = (sha1_32t)(ctx->count[0] & SHA1_MASK);

    /* mask out the rest of any partial 32-bit word and then set    */
    /* the next byte to 0x80. On big-endian machines any bytes in   */
    /* the buffer will be at the top end of 32 bit words, on little */
    /* endian machines they will be at the bottom. Hence the AND    */
    /* and OR masks above are reversed for little endian systems    */
    /* Note that we can always add the first padding byte at this   */
    /* point because the buffer always has at least one empty slot  */ 
    ctx->wbuf[i >> 2] = (ctx->wbuf[i >> 2] & mask[i & 3]) | bits[i & 3];

    /* we need 9 or more empty positions, one for the padding byte  */
    /* (above) and eight for the length count.  If there is not     */
    /* enough space pad and empty the buffer                        */
    if(i > SHA1_BLOCK_SIZE - 9)
    {
        if(i < 60) ctx->wbuf[15] = 0;
        sha1_compile(ctx);
        i = 0;
    }
    else    /* compute a word index for the empty buffer positions  */
        i = (i >> 2) + 1;

    while(i < 14) /* and zero pad all but last two positions        */ 
        ctx->wbuf[i++] = 0;
    
    /* assemble the eight byte counter in in big-endian format      */
    ctx->wbuf[14] = swap_b32((ctx->count[1] << 3) | (ctx->count[0] >> 29));
    ctx->wbuf[15] = swap_b32(ctx->count[0] << 3);

    sha1_compile(ctx);

    /* extract the hash value as bytes in case the hash buffer is   */
    /* misaligned for 32-bit words                                  */
    for(i = 0; i < SHA1_DIGEST_SIZE; ++i)
        hval[i] = (unsigned char)(ctx->hash[i >> 2] >> (8 * (~i & 3)));
}

void sha1(unsigned char hval[], const unsigned char data[], unsigned long len)
{   sha1_ctx    cx[1];

    sha1_begin(cx); sha1_hash(data, len, cx); sha1_end(hval, cx);
}

