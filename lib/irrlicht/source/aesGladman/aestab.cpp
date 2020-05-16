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

*/

#define DO_TABLES

#include "aesopt.h"

#if defined(FIXED_TABLES)

/* implemented in case of wrong call for fixed tables */

void gen_tabs(void)
{
}

#else   /* dynamic table generation */

#if !defined(FF_TABLES)

/*  Generate the tables for the dynamic table option

    It will generally be sensible to use tables to compute finite
    field multiplies and inverses but where memory is scarse this
    code might sometimes be better. But it only has effect during
    initialisation so its pretty unimportant in overall terms.
*/

/*  return 2 ^ (n - 1) where n is the bit number of the highest bit
    set in x with x in the range 1 < x < 0x00000200.   This form is
    used so that locals within fi can be bytes rather than words
*/

static aes_08t hibit(const aes_32t x)
{   aes_08t r = (aes_08t)((x >> 1) | (x >> 2));

    r |= (r >> 2);
    r |= (r >> 4);
    return (r + 1) >> 1;
}

/* return the inverse of the finite field element x */

static aes_08t fi(const aes_08t x)
{   aes_08t p1 = x, p2 = BPOLY, n1 = hibit(x), n2 = 0x80, v1 = 1, v2 = 0;

    if(x < 2) return x;

    for(;;)
    {
        if(!n1) return v1;

        while(n2 >= n1)
        {
            n2 /= n1; p2 ^= p1 * n2; v2 ^= v1 * n2; n2 = hibit(p2);
        }

        if(!n2) return v2;

        while(n1 >= n2)
        {
            n1 /= n2; p1 ^= p2 * n1; v1 ^= v2 * n1; n1 = hibit(p1);
        }
    }
}

#endif

/* The forward and inverse affine transformations used in the S-box */

#define fwd_affine(x) \
    (w = (aes_32t)x, w ^= (w<<1)^(w<<2)^(w<<3)^(w<<4), 0x63^(aes_08t)(w^(w>>8)))

#define inv_affine(x) \
    (w = (aes_32t)x, w = (w<<1)^(w<<3)^(w<<6), 0x05^(aes_08t)(w^(w>>8)))

static int init = 0;

void gen_tabs(void)
{   aes_32t  i, w;

#if defined(FF_TABLES)

    aes_08t  pow[512], log[256];

    if(init) return;
    /*  log and power tables for GF(2^8) finite field with
        WPOLY as modular polynomial - the simplest primitive
        root is 0x03, used here to generate the tables
    */

    i = 0; w = 1;
    do
    {
        pow[i] = (aes_08t)w;
        pow[i + 255] = (aes_08t)w;
        log[w] = (aes_08t)i++;
        w ^=  (w << 1) ^ (w & 0x80 ? WPOLY : 0);
    }
    while (w != 1);

#else
    if(init) return;
#endif

    for(i = 0, w = 1; i < RC_LENGTH; ++i)
    {
        t_set(r,c)[i] = bytes2word(w, 0, 0, 0);
        w = f2(w);
    }

    for(i = 0; i < 256; ++i)
    {   aes_08t    b;

        b = fwd_affine(fi((aes_08t)i));
        w = bytes2word(f2(b), b, b, f3(b));

#ifdef  SBX_SET
        t_set(s,box)[i] = b;
#endif

#ifdef  FT1_SET                 /* tables for a normal encryption round */
        t_set(f,n)[i] = w;
#endif
#ifdef  FT4_SET
        t_set(f,n)[0][i] = w;
        t_set(f,n)[1][i] = upr(w,1);
        t_set(f,n)[2][i] = upr(w,2);
        t_set(f,n)[3][i] = upr(w,3);
#endif
        w = bytes2word(b, 0, 0, 0);

#ifdef  FL1_SET                 /* tables for last encryption round (may also   */
        t_set(f,l)[i] = w;        /* be used in the key schedule)                 */
#endif
#ifdef  FL4_SET
        t_set(f,l)[0][i] = w;
        t_set(f,l)[1][i] = upr(w,1);
        t_set(f,l)[2][i] = upr(w,2);
        t_set(f,l)[3][i] = upr(w,3);
#endif

#ifdef  LS1_SET                 /* table for key schedule if t_set(f,l) above is    */
        t_set(l,s)[i] = w;      /* not of the required form                     */
#endif
#ifdef  LS4_SET
        t_set(l,s)[0][i] = w;
        t_set(l,s)[1][i] = upr(w,1);
        t_set(l,s)[2][i] = upr(w,2);
        t_set(l,s)[3][i] = upr(w,3);
#endif

        b = fi(inv_affine((aes_08t)i));
        w = bytes2word(fe(b), f9(b), fd(b), fb(b));

#ifdef  IM1_SET                 /* tables for the inverse mix column operation  */
        t_set(i,m)[b] = w;
#endif
#ifdef  IM4_SET
        t_set(i,m)[0][b] = w;
        t_set(i,m)[1][b] = upr(w,1);
        t_set(i,m)[2][b] = upr(w,2);
        t_set(i,m)[3][b] = upr(w,3);
#endif

#ifdef  ISB_SET
        t_set(i,box)[i] = b;
#endif
#ifdef  IT1_SET                 /* tables for a normal decryption round */
        t_set(i,n)[i] = w;
#endif
#ifdef  IT4_SET
        t_set(i,n)[0][i] = w;
        t_set(i,n)[1][i] = upr(w,1);
        t_set(i,n)[2][i] = upr(w,2);
        t_set(i,n)[3][i] = upr(w,3);
#endif
        w = bytes2word(b, 0, 0, 0);
#ifdef  IL1_SET                 /* tables for last decryption round */
        t_set(i,l)[i] = w;
#endif
#ifdef  IL4_SET
        t_set(i,l)[0][i] = w;
        t_set(i,l)[1][i] = upr(w,1);
        t_set(i,l)[2][i] = upr(w,2);
        t_set(i,l)[3][i] = upr(w,3);
#endif
    }
    init = 1;
}

#endif

