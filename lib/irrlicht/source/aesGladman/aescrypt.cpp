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

 This file contains the code for implementing encryption and decryption
 for AES (Rijndael) for block and key sizes of 16, 24 and 32 bytes. It
 can optionally be replaced by code written in assembler using NASM. For
 further details see the file aesopt.h
*/

#include "aesopt.h"

#define si(y,x,k,c) (s(y,c) = word_in(x, c) ^ (k)[c])
#define so(y,x,c)   word_out(y, c, s(x,c))

#if defined(ARRAYS)
#define locals(y,x)     x[4],y[4]
#else
#define locals(y,x)     x##0,x##1,x##2,x##3,y##0,y##1,y##2,y##3
#endif

#define l_copy(y, x)    s(y,0) = s(x,0); s(y,1) = s(x,1); \
                        s(y,2) = s(x,2); s(y,3) = s(x,3);
#define state_in(y,x,k) si(y,x,k,0); si(y,x,k,1); si(y,x,k,2); si(y,x,k,3)
#define state_out(y,x)  so(y,x,0); so(y,x,1); so(y,x,2); so(y,x,3)
#define round(rm,y,x,k) rm(y,x,k,0); rm(y,x,k,1); rm(y,x,k,2); rm(y,x,k,3)

#if defined(ENCRYPTION) && !defined(AES_ASM)

/* Visual C++ .Net v7.1 provides the fastest encryption code when using
   Pentium optimization with small code but this is poor for decryption
   so we need to control this with the following VC++ pragmas
*/

#if defined(_MSC_VER)
#pragma optimize( "s", on )
#endif

/* Given the column (c) of the output state variable, the following
   macros give the input state variables which are needed in its
   computation for each row (r) of the state. All the alternative
   macros give the same end values but expand into different ways
   of calculating these values.  In particular the complex macro
   used for dynamically variable block sizes is designed to expand
   to a compile time constant whenever possible but will expand to
   conditional clauses on some branches (I am grateful to Frank
   Yellin for this construction)
*/

#define fwd_var(x,r,c)\
 ( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
 : r == 1 ? ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0))\
 : r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
 :          ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2)))

#if defined(FT4_SET)
#undef  dec_fmvars
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,n),fwd_var,rf1,c))
#elif defined(FT1_SET)
#undef  dec_fmvars
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ one_table(x,upr,t_use(f,n),fwd_var,rf1,c))
#else
#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ fwd_mcol(no_table(x,t_use(s,box),fwd_var,rf1,c)))
#endif

#if defined(FL4_SET)
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,l),fwd_var,rf1,c))
#elif defined(FL1_SET)
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ one_table(x,ups,t_use(f,l),fwd_var,rf1,c))
#else
#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ no_table(x,t_use(s,box),fwd_var,rf1,c))
#endif

aes_rval aes_encrypt(const void *in_blk, void *out_blk, const aes_encrypt_ctx cx[1])
{   aes_32t         locals(b0, b1);
    const aes_32t   *kp = cx->ks;
#ifdef dec_fmvars
    dec_fmvars; /* declare variables for fwd_mcol() if needed */
#endif

    aes_32t nr = (kp[45] ^ kp[52] ^ kp[53] ? kp[52] : 14);

#ifdef AES_ERR_CHK
    if(   (nr != 10 || !(kp[0] | kp[3] | kp[4])) 
       && (nr != 12 || !(kp[0] | kp[5] | kp[6]))
       && (nr != 14 || !(kp[0] | kp[7] | kp[8])) )
        return aes_error;
#endif

    state_in(b0, in_blk, kp);

#if (ENC_UNROLL == FULL)

    switch(nr)
    {
    case 14:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        kp += 2 * N_COLS;
    case 12:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        kp += 2 * N_COLS;
    case 10:
        round(fwd_rnd,  b1, b0, kp + 1 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 2 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 3 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 4 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 5 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 6 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 7 * N_COLS);
        round(fwd_rnd,  b0, b1, kp + 8 * N_COLS);
        round(fwd_rnd,  b1, b0, kp + 9 * N_COLS);
        round(fwd_lrnd, b0, b1, kp +10 * N_COLS);
    }

#else

#if (ENC_UNROLL == PARTIAL)
    {   aes_32t    rnd;
        for(rnd = 0; rnd < (nr >> 1) - 1; ++rnd)
        {
            kp += N_COLS;
            round(fwd_rnd, b1, b0, kp);
            kp += N_COLS;
            round(fwd_rnd, b0, b1, kp);
        }
        kp += N_COLS;
        round(fwd_rnd,  b1, b0, kp);
#else
    {   aes_32t    rnd;
        for(rnd = 0; rnd < nr - 1; ++rnd)
        {
            kp += N_COLS;
            round(fwd_rnd, b1, b0, kp);
            l_copy(b0, b1);
        }
#endif
        kp += N_COLS;
        round(fwd_lrnd, b0, b1, kp);
    }
#endif

    state_out(out_blk, b0);
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

#if defined(DECRYPTION) && !defined(AES_ASM)

/* Visual C++ .Net v7.1 provides the fastest encryption code when using
   Pentium optimization with small code but this is poor for decryption
   so we need to control this with the following VC++ pragmas
*/

#if defined(_MSC_VER)
#pragma optimize( "t", on )
#endif

/* Given the column (c) of the output state variable, the following
   macros give the input state variables which are needed in its
   computation for each row (r) of the state. All the alternative
   macros give the same end values but expand into different ways
   of calculating these values.  In particular the complex macro
   used for dynamically variable block sizes is designed to expand
   to a compile time constant whenever possible but will expand to
   conditional clauses on some branches (I am grateful to Frank
   Yellin for this construction)
*/

#define inv_var(x,r,c)\
 ( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
 : r == 1 ? ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2))\
 : r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
 :          ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0)))

#if defined(IT4_SET)
#undef  dec_imvars
#define inv_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,n),inv_var,rf1,c))
#elif defined(IT1_SET)
#undef  dec_imvars
#define inv_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ one_table(x,upr,t_use(i,n),inv_var,rf1,c))
#else
#define inv_rnd(y,x,k,c)    (s(y,c) = inv_mcol((k)[c] ^ no_table(x,t_use(i,box),inv_var,rf1,c)))
#endif

#if defined(IL4_SET)
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,l),inv_var,rf1,c))
#elif defined(IL1_SET)
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ one_table(x,ups,t_use(i,l),inv_var,rf1,c))
#else
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ no_table(x,t_use(i,box),inv_var,rf1,c))
#endif

aes_rval aes_decrypt(const void *in_blk, void *out_blk, const aes_decrypt_ctx cx[1])
{   aes_32t        locals(b0, b1);
#ifdef dec_imvars
    dec_imvars; /* declare variables for inv_mcol() if needed */
#endif

    aes_32t nr = (cx->ks[45] ^ cx->ks[52] ^ cx->ks[53] ? cx->ks[52] : 14);
    const aes_32t *kp = cx->ks + nr * N_COLS;

#ifdef AES_ERR_CHK
    if(   (nr != 10 || !(cx->ks[0] | cx->ks[3] | cx->ks[4])) 
       && (nr != 12 || !(cx->ks[0] | cx->ks[5] | cx->ks[6]))
       && (nr != 14 || !(cx->ks[0] | cx->ks[7] | cx->ks[8])) )
        return aes_error;
#endif

    state_in(b0, in_blk, kp);

#if (DEC_UNROLL == FULL)

    switch(nr)
    {
    case 14:
        round(inv_rnd,  b1, b0, kp -  1 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  2 * N_COLS);
        kp -= 2 * N_COLS;
    case 12:
        round(inv_rnd,  b1, b0, kp -  1 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  2 * N_COLS);
        kp -= 2 * N_COLS;
    case 10:
        round(inv_rnd,  b1, b0, kp -  1 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  2 * N_COLS);
        round(inv_rnd,  b1, b0, kp -  3 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  4 * N_COLS);
        round(inv_rnd,  b1, b0, kp -  5 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  6 * N_COLS);
        round(inv_rnd,  b1, b0, kp -  7 * N_COLS);
        round(inv_rnd,  b0, b1, kp -  8 * N_COLS);
        round(inv_rnd,  b1, b0, kp -  9 * N_COLS);
        round(inv_lrnd, b0, b1, kp - 10 * N_COLS);
    }

#else

#if (DEC_UNROLL == PARTIAL)
    {   aes_32t    rnd;
        for(rnd = 0; rnd < (nr >> 1) - 1; ++rnd)
        {
            kp -= N_COLS;
            round(inv_rnd, b1, b0, kp);
            kp -= N_COLS;
            round(inv_rnd, b0, b1, kp);
        }
        kp -= N_COLS;
        round(inv_rnd, b1, b0, kp);
#else
    {   aes_32t    rnd;
        for(rnd = 0; rnd < nr - 1; ++rnd)
        {
            kp -= N_COLS;
            round(inv_rnd, b1, b0, kp);
            l_copy(b0, b1);
        }
#endif
        kp -= N_COLS;
        round(inv_lrnd, b0, b1, kp);
    }
#endif

    state_out(out_blk, b0);
#ifdef AES_ERR_CHK
    return aes_good;
#endif
}

#endif

