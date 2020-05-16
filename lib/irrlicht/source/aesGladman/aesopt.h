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

 My thanks go to Dag Arne Osvik for devising the schemes used here for key
 length derivation from the form of the key schedule

 This file contains the compilation options for AES (Rijndael) and code
 that is common across encryption, key scheduling and table generation.

    OPERATION

    These source code files implement the AES algorithm Rijndael designed by
    Joan Daemen and Vincent Rijmen. This version is designed for the standard
    block size of 16 bytes and for key sizes of 128, 192 and 256 bits (16, 24
    and 32 bytes).

    This version is designed for flexibility and speed using operations on
    32-bit words rather than operations on bytes.  It can be compiled with
    either big or little endian internal byte order but is faster when the
    native byte order for the processor is used.

    THE CIPHER INTERFACE

    The cipher interface is implemented as an array of bytes in which lower
    AES bit sequence indexes map to higher numeric significance within bytes.

    aes_08t                 (an unsigned  8-bit type)
    aes_32t                 (an unsigned 32-bit type)
    struct aes_encrypt_ctx  (structure for the cipher encryption context)
    struct aes_decrypt_ctx  (structure for the cipher decryption context)
    aes_rval                the function return type

    C subroutine calls:

      aes_rval aes_encrypt_key128(const void *in_key, aes_encrypt_ctx cx[1]);
      aes_rval aes_encrypt_key192(const void *in_key, aes_encrypt_ctx cx[1]);
      aes_rval aes_encrypt_key256(const void *in_key, aes_encrypt_ctx cx[1]);
      aes_rval aes_encrypt(const void *in_blk,
                                 void *out_blk, const aes_encrypt_ctx cx[1]);

      aes_rval aes_decrypt_key128(const void *in_key, aes_decrypt_ctx cx[1]);
      aes_rval aes_decrypt_key192(const void *in_key, aes_decrypt_ctx cx[1]);
      aes_rval aes_decrypt_key256(const void *in_key, aes_decrypt_ctx cx[1]);
      aes_rval aes_decrypt(const void *in_blk,
                                 void *out_blk, const aes_decrypt_ctx cx[1]);

    IMPORTANT NOTE: If you are using this C interface with dynamic tables make sure that
    you call genTabs() before AES is used so that the tables are initialised.

    C++ aes class subroutines:

        Class AESencrypt  for encryption

        Construtors:
            AESencrypt(void)
            AESencrypt(const void *in_key) - 128 bit key
        Members:
            void key128(const void *in_key)
            void key192(const void *in_key)
            void key256(const void *in_key)
            void encrypt(const void *in_blk, void *out_blk) const

        Class AESdecrypt  for encryption
        Construtors:
            AESdecrypt(void)
            AESdecrypt(const void *in_key) - 128 bit key
        Members:
            void key128(const void *in_key)
            void key192(const void *in_key)
            void key256(const void *in_key)
            void decrypt(const void *in_blk, void *out_blk) const

    COMPILATION

    The files used to provide AES (Rijndael) are

    a. aes.h for the definitions needed for use in C.
    b. aescpp.h for the definitions needed for use in C++.
    c. aesopt.h for setting compilation options (also includes common code).
    d. aescrypt.c for encryption and decrytpion, or
    e. aeskey.c for key scheduling.
    f. aestab.c for table loading or generation.
    g. aescrypt.asm for encryption and decryption using assembler code.
    h. aescrypt.mmx.asm for encryption and decryption using MMX assembler.

    To compile AES (Rijndael) for use in C code use aes.h and set the
    defines here for the facilities you need (key lengths, encryption
    and/or decryption). Do not define AES_DLL or AES_CPP.  Set the options
    for optimisations and table sizes here.

    To compile AES (Rijndael) for use in in C++ code use aescpp.h but do
    not define AES_DLL

    To compile AES (Rijndael) in C as a Dynamic Link Library DLL) use
    aes.h and include the AES_DLL define.

    CONFIGURATION OPTIONS (here and in aes.h)

    a. set AES_DLL in aes.h if AES (Rijndael) is to be compiled as a DLL
    b. You may need to set PLATFORM_BYTE_ORDER to define the byte order.
    c. If you want the code to run in a specific internal byte order, then
       ALGORITHM_BYTE_ORDER must be set accordingly.
    d. set other configuration options decribed below.
*/

#ifndef _AESOPT_H
#define _AESOPT_H

#include "aes.h"

/*  CONFIGURATION - USE OF DEFINES

    Later in this section there are a number of defines that control the
    operation of the code.  In each section, the purpose of each define is
    explained so that the relevant form can be included or excluded by
    setting either 1's or 0's respectively on the branches of the related
    #if clauses.
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

/*  SOME LOCAL DEFINITIONS  */

#define NO_TABLES              0
#define ONE_TABLE              1
#define FOUR_TABLES            4
#define NONE                   0
#define PARTIAL                1
#define FULL                   2

#define aes_sw32 Byteswap::byteswap

/*  1. FUNCTIONS REQUIRED

    This implementation provides subroutines for encryption, decryption
    and for setting the three key lengths (separately) for encryption
    and decryption. When the assembler code is not being used the following
    definition blocks allow the selection of the routines that are to be
    included in the compilation.
*/
#ifdef AES_ENCRYPT
#define ENCRYPTION
#define ENCRYPTION_KEY_SCHEDULE
#endif

#ifdef AES_DECRYPT
#define DECRYPTION
#define DECRYPTION_KEY_SCHEDULE
#endif

/*  2. ASSEMBLER SUPPORT

    This define (which can be on the command line) enables the use of the
    assembler code routines for encryption and decryption with the C code
    only providing key scheduling
*/
#if 0
#define AES_ASM
#endif

/*  3. BYTE ORDER WITHIN 32 BIT WORDS

    The fundamental data processing units in Rijndael are 8-bit bytes. The
    input, output and key input are all enumerated arrays of bytes in which
    bytes are numbered starting at zero and increasing to one less than the
    number of bytes in the array in question. This enumeration is only used
    for naming bytes and does not imply any adjacency or order relationship
    from one byte to another. When these inputs and outputs are considered
    as bit sequences, bits 8*n to 8*n+7 of the bit sequence are mapped to
    byte[n] with bit 8n+i in the sequence mapped to bit 7-i within the byte.
    In this implementation bits are numbered from 0 to 7 starting at the
    numerically least significant end of each byte (bit n represents 2^n).

    However, Rijndael can be implemented more efficiently using 32-bit
    words by packing bytes into words so that bytes 4*n to 4*n+3 are placed
    into word[n]. While in principle these bytes can be assembled into words
    in any positions, this implementation only supports the two formats in
    which bytes in adjacent positions within words also have adjacent byte
    numbers. This order is called big-endian if the lowest numbered bytes
    in words have the highest numeric significance and little-endian if the
    opposite applies.

    This code can work in either order irrespective of the order used by the
    machine on which it runs. Normally the internal byte order will be set
    to the order of the processor on which the code is to be run but this
    define can be used to reverse this in special situations

    NOTE: Assembler code versions rely on PLATFORM_BYTE_ORDER being set
*/
#if 1 || defined(AES_ASM)
#define ALGORITHM_BYTE_ORDER PLATFORM_BYTE_ORDER
#elif 0
#define ALGORITHM_BYTE_ORDER BRG_LITTLE_ENDIAN
#elif 0
#define ALGORITHM_BYTE_ORDER BRG_BIG_ENDIAN
#else
#error The algorithm byte order is not defined
#endif

/*  4. FAST INPUT/OUTPUT OPERATIONS.

    On some machines it is possible to improve speed by transferring the
    bytes in the input and output arrays to and from the internal 32-bit
    variables by addressing these arrays as if they are arrays of 32-bit
    words.  On some machines this will always be possible but there may
    be a large performance penalty if the byte arrays are not aligned on
    the normal word boundaries. On other machines this technique will
    lead to memory access errors when such 32-bit word accesses are not
    properly aligned. The option SAFE_IO avoids such problems but will
    often be slower on those machines that support misaligned access
    (especially so if care is taken to align the input  and output byte
    arrays on 32-bit word boundaries). If SAFE_IO is not defined it is
    assumed that access to byte arrays as if they are arrays of 32-bit
    words will not cause problems when such accesses are misaligned.
*/
#if 1 && !defined(_MSC_VER)
#define SAFE_IO
#endif

/*  5. LOOP UNROLLING

    The code for encryption and decrytpion cycles through a number of rounds
    that can be implemented either in a loop or by expanding the code into a
    long sequence of instructions, the latter producing a larger program but
    one that will often be much faster. The latter is called loop unrolling.
    There are also potential speed advantages in expanding two iterations in
    a loop with half the number of iterations, which is called partial loop
    unrolling.  The following options allow partial or full loop unrolling
    to be set independently for encryption and decryption
*/
#if 1
#define ENC_UNROLL  FULL
#elif 0
#define ENC_UNROLL  PARTIAL
#else
#define ENC_UNROLL  NONE
#endif

#if 1
#define DEC_UNROLL  FULL
#elif 0
#define DEC_UNROLL  PARTIAL
#else
#define DEC_UNROLL  NONE
#endif

/*  6. FAST FINITE FIELD OPERATIONS

    If this section is included, tables are used to provide faster finite
    field arithmetic (this has no effect if FIXED_TABLES is defined).
*/
#if 0
#define FF_TABLES
#endif

/*  7. INTERNAL STATE VARIABLE FORMAT

    The internal state of Rijndael is stored in a number of local 32-bit
    word varaibles which can be defined either as an array or as individual
    names variables. Include this section if you want to store these local
    varaibles in arrays. Otherwise individual local variables will be used.
*/
#if 1
#define ARRAYS
#endif

/* In this implementation the columns of the state array are each held in
   32-bit words. The state array can be held in various ways: in an array
   of words, in a number of individual word variables or in a number of
   processor registers. The following define maps a variable name x and
   a column number c to the way the state array variable is to be held.
   The first define below maps the state into an array x[c] whereas the
   second form maps the state into a number of individual variables x0,
   x1, etc.  Another form could map individual state colums to machine
   register names.
*/

#if defined(ARRAYS)
#define s(x,c) x[c]
#else
#define s(x,c) x##c
#endif

/*  8. FIXED OR DYNAMIC TABLES

    When this section is included the tables used by the code are compiled
    statically into the binary file.  Otherwise the subroutine gen_tabs()
    must be called to compute them before the code is first used.
*/
#if 1
#define FIXED_TABLES
#define DO_TABLES
#endif

/*  9. TABLE ALIGNMENT

    On some systems speed will be improved by aligning the AES large lookup
    tables on particular boundaries. This define should be set to a power of
    two giving the desired alignment. It can be left undefined if alignment 
    is not needed.  This option is specific to the Microsft VC++ compiler -
    it seems to sometimes cause trouble for the VC++ version 6 compiler.
*/

#if 0 && defined(_MSC_VER) && (_MSC_VER >= 1300)
#define TABLE_ALIGN 64
#endif

/*  10. INTERNAL TABLE CONFIGURATION

    This cipher proceeds by repeating in a number of cycles known as 'rounds'
    which are implemented by a round function which can optionally be speeded
    up using tables.  The basic tables are each 256 32-bit words, with either
    one or four tables being required for each round function depending on
    how much speed is required. The encryption and decryption round functions
    are different and the last encryption and decrytpion round functions are
    different again making four different round functions in all.

    This means that:
      1. Normal encryption and decryption rounds can each use either 0, 1
         or 4 tables and table spaces of 0, 1024 or 4096 bytes each.
      2. The last encryption and decryption rounds can also use either 0, 1
         or 4 tables and table spaces of 0, 1024 or 4096 bytes each.

    Include or exclude the appropriate definitions below to set the number
    of tables used by this implementation.
*/

#if 1   /* set tables for the normal encryption round */
#define ENC_ROUND   FOUR_TABLES
#elif 0
#define ENC_ROUND   ONE_TABLE
#else
#define ENC_ROUND   NO_TABLES
#endif

#if 1   /* set tables for the last encryption round */
#define LAST_ENC_ROUND  FOUR_TABLES
#elif 0
#define LAST_ENC_ROUND  ONE_TABLE
#else
#define LAST_ENC_ROUND  NO_TABLES
#endif

#if 1   /* set tables for the normal decryption round */
#define DEC_ROUND   FOUR_TABLES
#elif 0
#define DEC_ROUND   ONE_TABLE
#else
#define DEC_ROUND   NO_TABLES
#endif

#if 1   /* set tables for the last decryption round */
#define LAST_DEC_ROUND  FOUR_TABLES
#elif 0
#define LAST_DEC_ROUND  ONE_TABLE
#else
#define LAST_DEC_ROUND  NO_TABLES
#endif

/*  The decryption key schedule can be speeded up with tables in the same
    way that the round functions can.  Include or exclude the following
    defines to set this requirement.
*/
#if 1
#define KEY_SCHED   FOUR_TABLES
#elif 0
#define KEY_SCHED   ONE_TABLE
#else
#define KEY_SCHED   NO_TABLES
#endif

/* END OF CONFIGURATION OPTIONS */

#define RC_LENGTH   (5 * (AES_BLOCK_SIZE / 4 - 2))

/* Disable or report errors on some combinations of options */

#if ENC_ROUND == NO_TABLES && LAST_ENC_ROUND != NO_TABLES
#undef  LAST_ENC_ROUND
#define LAST_ENC_ROUND  NO_TABLES
#elif ENC_ROUND == ONE_TABLE && LAST_ENC_ROUND == FOUR_TABLES
#undef  LAST_ENC_ROUND
#define LAST_ENC_ROUND  ONE_TABLE
#endif

#if ENC_ROUND == NO_TABLES && ENC_UNROLL != NONE
#undef  ENC_UNROLL
#define ENC_UNROLL  NONE
#endif

#if DEC_ROUND == NO_TABLES && LAST_DEC_ROUND != NO_TABLES
#undef  LAST_DEC_ROUND
#define LAST_DEC_ROUND  NO_TABLES
#elif DEC_ROUND == ONE_TABLE && LAST_DEC_ROUND == FOUR_TABLES
#undef  LAST_DEC_ROUND
#define LAST_DEC_ROUND  ONE_TABLE
#endif

#if DEC_ROUND == NO_TABLES && DEC_UNROLL != NONE
#undef  DEC_UNROLL
#define DEC_UNROLL  NONE
#endif

/*  upr(x,n):  rotates bytes within words by n positions, moving bytes to
               higher index positions with wrap around into low positions
    ups(x,n):  moves bytes by n positions to higher index positions in
               words but without wrap around
    bval(x,n): extracts a byte from a word

    NOTE:      The definitions given here are intended only for use with
               unsigned variables and with shift counts that are compile
               time constants
*/

#if (ALGORITHM_BYTE_ORDER == BRG_LITTLE_ENDIAN)
#define upr(x,n)        (((aes_32t)(x) << (8 * (n))) | ((aes_32t)(x) >> (32 - 8 * (n))))
#define ups(x,n)        ((aes_32t) (x) << (8 * (n)))
#define bval(x,n)       ((aes_08t)((x) >> (8 * (n))))
#define bytes2word(b0, b1, b2, b3)  \
        (((aes_32t)(b3) << 24) | ((aes_32t)(b2) << 16) | ((aes_32t)(b1) << 8) | (b0))
#endif

#if (ALGORITHM_BYTE_ORDER == BRG_BIG_ENDIAN)
#define upr(x,n)        (((aes_32t)(x) >> (8 * (n))) | ((aes_32t)(x) << (32 - 8 * (n))))
#define ups(x,n)        ((aes_32t) (x) >> (8 * (n))))
#define bval(x,n)       ((aes_08t)((x) >> (24 - 8 * (n))))
#define bytes2word(b0, b1, b2, b3)  \
        (((aes_32t)(b0) << 24) | ((aes_32t)(b1) << 16) | ((aes_32t)(b2) << 8) | (b3))
#endif

#if defined(SAFE_IO)

#define word_in(x,c)    bytes2word(((aes_08t*)(x)+4*c)[0], ((aes_08t*)(x)+4*c)[1], \
                                   ((aes_08t*)(x)+4*c)[2], ((aes_08t*)(x)+4*c)[3])
#define word_out(x,c,v) { ((aes_08t*)(x)+4*c)[0] = bval(v,0); ((aes_08t*)(x)+4*c)[1] = bval(v,1); \
                          ((aes_08t*)(x)+4*c)[2] = bval(v,2); ((aes_08t*)(x)+4*c)[3] = bval(v,3); }

#elif (ALGORITHM_BYTE_ORDER == PLATFORM_BYTE_ORDER)

#define word_in(x,c)    (*((aes_32t*)(x)+(c)))
#define word_out(x,c,v) (*((aes_32t*)(x)+(c)) = (v))

#else

#define word_in(x,c)    aes_sw32(*((aes_32t*)(x)+(c)))
#define word_out(x,c,v) (*((aes_32t*)(x)+(c)) = aes_sw32(v))

#endif

/* the finite field modular polynomial and elements */

#define WPOLY   0x011b
#define BPOLY     0x1b

/* multiply four bytes in GF(2^8) by 'x' {02} in parallel */

#define m1  0x80808080
#define m2  0x7f7f7f7f
#define gf_mulx(x)  ((((x) & m2) << 1) ^ ((((x) & m1) >> 7) * BPOLY))

/* The following defines provide alternative definitions of gf_mulx that might
   give improved performance if a fast 32-bit multiply is not available. Note
   that a temporary variable u needs to be defined where gf_mulx is used.

#define gf_mulx(x) (u = (x) & m1, u |= (u >> 1), ((x) & m2) << 1) ^ ((u >> 3) | (u >> 6))
#define m4  (0x01010101 * BPOLY)
#define gf_mulx(x) (u = (x) & m1, ((x) & m2) << 1) ^ ((u - (u >> 7)) & m4)
*/

/* Work out which tables are needed for the different options   */

#ifdef  AES_ASM
#ifdef  ENC_ROUND
#undef  ENC_ROUND
#endif
#define ENC_ROUND   FOUR_TABLES
#ifdef  LAST_ENC_ROUND
#undef  LAST_ENC_ROUND
#endif
#define LAST_ENC_ROUND  FOUR_TABLES
#ifdef  DEC_ROUND
#undef  DEC_ROUND
#endif
#define DEC_ROUND   FOUR_TABLES
#ifdef  LAST_DEC_ROUND
#undef  LAST_DEC_ROUND
#endif
#define LAST_DEC_ROUND  FOUR_TABLES
#ifdef  KEY_SCHED
#undef  KEY_SCHED
#define KEY_SCHED   FOUR_TABLES
#endif
#endif

#if defined(ENCRYPTION) || defined(AES_ASM)
#if ENC_ROUND == ONE_TABLE
#define FT1_SET
#elif ENC_ROUND == FOUR_TABLES
#define FT4_SET
#else
#define SBX_SET
#endif
#if LAST_ENC_ROUND == ONE_TABLE
#define FL1_SET
#elif LAST_ENC_ROUND == FOUR_TABLES
#define FL4_SET
#elif !defined(SBX_SET)
#define SBX_SET
#endif
#endif

#if defined(DECRYPTION) || defined(AES_ASM)
#if DEC_ROUND == ONE_TABLE
#define IT1_SET
#elif DEC_ROUND == FOUR_TABLES
#define IT4_SET
#else
#define ISB_SET
#endif
#if LAST_DEC_ROUND == ONE_TABLE
#define IL1_SET
#elif LAST_DEC_ROUND == FOUR_TABLES
#define IL4_SET
#elif !defined(ISB_SET)
#define ISB_SET
#endif
#endif

#if defined(ENCRYPTION_KEY_SCHEDULE) || defined(DECRYPTION_KEY_SCHEDULE)
#if KEY_SCHED == ONE_TABLE
#define LS1_SET
#define IM1_SET
#elif KEY_SCHED == FOUR_TABLES
#define LS4_SET
#define IM4_SET
#elif !defined(SBX_SET)
#define SBX_SET
#endif
#endif

/* generic definitions of Rijndael macros that use tables    */

#define no_table(x,box,vf,rf,c) bytes2word( \
    box[bval(vf(x,0,c),rf(0,c))], \
    box[bval(vf(x,1,c),rf(1,c))], \
    box[bval(vf(x,2,c),rf(2,c))], \
    box[bval(vf(x,3,c),rf(3,c))])

#define one_table(x,op,tab,vf,rf,c) \
 (     tab[bval(vf(x,0,c),rf(0,c))] \
  ^ op(tab[bval(vf(x,1,c),rf(1,c))],1) \
  ^ op(tab[bval(vf(x,2,c),rf(2,c))],2) \
  ^ op(tab[bval(vf(x,3,c),rf(3,c))],3))

#define four_tables(x,tab,vf,rf,c) \
 (  tab[0][bval(vf(x,0,c),rf(0,c))] \
  ^ tab[1][bval(vf(x,1,c),rf(1,c))] \
  ^ tab[2][bval(vf(x,2,c),rf(2,c))] \
  ^ tab[3][bval(vf(x,3,c),rf(3,c))])

#define vf1(x,r,c)  (x)
#define rf1(r,c)    (r)
#define rf2(r,c)    ((8+r-c)&3)

/* perform forward and inverse column mix operation on four bytes in long word x in */
/* parallel. NOTE: x must be a simple variable, NOT an expression in these macros.  */

#if defined(FM4_SET)    /* not currently used */
#define fwd_mcol(x)     four_tables(x,t_use(f,m),vf1,rf1,0)
#elif defined(FM1_SET)  /* not currently used */
#define fwd_mcol(x)     one_table(x,upr,t_use(f,m),vf1,rf1,0)
#else
#define dec_fmvars      aes_32t g2
#define fwd_mcol(x)     (g2 = gf_mulx(x), g2 ^ upr((x) ^ g2, 3) ^ upr((x), 2) ^ upr((x), 1))
#endif

#if defined(IM4_SET)
#define inv_mcol(x)     four_tables(x,t_use(i,m),vf1,rf1,0)
#elif defined(IM1_SET)
#define inv_mcol(x)     one_table(x,upr,t_use(i,m),vf1,rf1,0)
#else
#define dec_imvars      aes_32t g2, g4, g9
#define inv_mcol(x)     (g2 = gf_mulx(x), g4 = gf_mulx(g2), g9 = (x) ^ gf_mulx(g4), g4 ^= g9, \
                        (x) ^ g2 ^ g4 ^ upr(g2 ^ g9, 3) ^ upr(g4, 2) ^ upr(g9, 1))
#endif

#if defined(FL4_SET)
#define ls_box(x,c)     four_tables(x,t_use(f,l),vf1,rf2,c)
#elif   defined(LS4_SET)
#define ls_box(x,c)     four_tables(x,t_use(l,s),vf1,rf2,c)
#elif defined(FL1_SET)
#define ls_box(x,c)     one_table(x,upr,t_use(f,l),vf1,rf2,c)
#elif defined(LS1_SET)
#define ls_box(x,c)     one_table(x,upr,t_use(l,s),vf1,rf2,c)
#else
#define ls_box(x,c)     no_table(x,t_use(s,box),vf1,rf2,c)
#endif

/*  If there are no global variables, the definitions here can be
    used to put the AES tables in a structure so that a pointer 
    can then be added to the AES context to pass them to the AES
    routines that need them.  If this facility is used, the calling 
    program has to ensure that this pointer is managed appropriately. 
    In particular, the value of the t_dec(in,it) item in the table 
    structure must be set to zero in order to ensure that the tables 
    are initialised. In practice the three code sequences in aeskey.c 
    that control the calls to gen_tabs() and the gen_tabs() routine 
    itself will have to be changed for a specific implementation. If 
    global variables are available it will generally be preferable to 
    use them with the precomputed FIXED_TABLES option that uses static 
    global tables.

    The following defines can be used to control the way the tables
    are defined, initialised and used in embedded environments that
    require special features for these purposes

    the 't_dec' construction is used to declare fixed table arrays
    the 't_set' construction is used to set fixed table values
    the 't_use' construction is used to access fixed table values

    256 byte tables:

        t_xxx(s,box)    => forward S box
        t_xxx(i,box)    => inverse S box

    256 32-bit word OR 4 x 256 32-bit word tables:

        t_xxx(f,n)      => forward normal round
        t_xxx(f,l)      => forward last round
        t_xxx(i,n)      => inverse normal round
        t_xxx(i,l)      => inverse last round
        t_xxx(l,s)      => key schedule table
        t_xxx(i,m)      => key schedule table

    Other variables and tables:

        t_xxx(r,c)      => the rcon table
*/

#define t_dec(m,n) t_##m##n
#define t_set(m,n) t_##m##n
#define t_use(m,n) t_##m##n

#if defined(DO_TABLES)  /* declare and instantiate tables   */

/*  finite field arithmetic operations for table generation */

#if defined(FIXED_TABLES) || !defined(FF_TABLES)

#define f2(x)   ((x<<1) ^ (((x>>7) & 1) * WPOLY))
#define f4(x)   ((x<<2) ^ (((x>>6) & 1) * WPOLY) ^ (((x>>6) & 2) * WPOLY))
#define f8(x)   ((x<<3) ^ (((x>>5) & 1) * WPOLY) ^ (((x>>5) & 2) * WPOLY) \
                        ^ (((x>>5) & 4) * WPOLY))
#define f3(x)   (f2(x) ^ x)
#define f9(x)   (f8(x) ^ x)
#define fb(x)   (f8(x) ^ f2(x) ^ x)
#define fd(x)   (f8(x) ^ f4(x) ^ x)
#define fe(x)   (f8(x) ^ f4(x) ^ f2(x))

#else

#define f2(x) ((x) ? pow[log[x] + 0x19] : 0)
#define f3(x) ((x) ? pow[log[x] + 0x01] : 0)
#define f9(x) ((x) ? pow[log[x] + 0xc7] : 0)
#define fb(x) ((x) ? pow[log[x] + 0x68] : 0)
#define fd(x) ((x) ? pow[log[x] + 0xee] : 0)
#define fe(x) ((x) ? pow[log[x] + 0xdf] : 0)
#define fi(x) ((x) ? pow[ 255 - log[x]] : 0)

#endif

#if defined(FIXED_TABLES)   /* declare and set values for static tables */

#define sb_data(w) \
    w(0x63), w(0x7c), w(0x77), w(0x7b), w(0xf2), w(0x6b), w(0x6f), w(0xc5),\
    w(0x30), w(0x01), w(0x67), w(0x2b), w(0xfe), w(0xd7), w(0xab), w(0x76),\
    w(0xca), w(0x82), w(0xc9), w(0x7d), w(0xfa), w(0x59), w(0x47), w(0xf0),\
    w(0xad), w(0xd4), w(0xa2), w(0xaf), w(0x9c), w(0xa4), w(0x72), w(0xc0),\
    w(0xb7), w(0xfd), w(0x93), w(0x26), w(0x36), w(0x3f), w(0xf7), w(0xcc),\
    w(0x34), w(0xa5), w(0xe5), w(0xf1), w(0x71), w(0xd8), w(0x31), w(0x15),\
    w(0x04), w(0xc7), w(0x23), w(0xc3), w(0x18), w(0x96), w(0x05), w(0x9a),\
    w(0x07), w(0x12), w(0x80), w(0xe2), w(0xeb), w(0x27), w(0xb2), w(0x75),\
    w(0x09), w(0x83), w(0x2c), w(0x1a), w(0x1b), w(0x6e), w(0x5a), w(0xa0),\
    w(0x52), w(0x3b), w(0xd6), w(0xb3), w(0x29), w(0xe3), w(0x2f), w(0x84),\
    w(0x53), w(0xd1), w(0x00), w(0xed), w(0x20), w(0xfc), w(0xb1), w(0x5b),\
    w(0x6a), w(0xcb), w(0xbe), w(0x39), w(0x4a), w(0x4c), w(0x58), w(0xcf),\
    w(0xd0), w(0xef), w(0xaa), w(0xfb), w(0x43), w(0x4d), w(0x33), w(0x85),\
    w(0x45), w(0xf9), w(0x02), w(0x7f), w(0x50), w(0x3c), w(0x9f), w(0xa8),\
    w(0x51), w(0xa3), w(0x40), w(0x8f), w(0x92), w(0x9d), w(0x38), w(0xf5),\
    w(0xbc), w(0xb6), w(0xda), w(0x21), w(0x10), w(0xff), w(0xf3), w(0xd2),\
    w(0xcd), w(0x0c), w(0x13), w(0xec), w(0x5f), w(0x97), w(0x44), w(0x17),\
    w(0xc4), w(0xa7), w(0x7e), w(0x3d), w(0x64), w(0x5d), w(0x19), w(0x73),\
    w(0x60), w(0x81), w(0x4f), w(0xdc), w(0x22), w(0x2a), w(0x90), w(0x88),\
    w(0x46), w(0xee), w(0xb8), w(0x14), w(0xde), w(0x5e), w(0x0b), w(0xdb),\
    w(0xe0), w(0x32), w(0x3a), w(0x0a), w(0x49), w(0x06), w(0x24), w(0x5c),\
    w(0xc2), w(0xd3), w(0xac), w(0x62), w(0x91), w(0x95), w(0xe4), w(0x79),\
    w(0xe7), w(0xc8), w(0x37), w(0x6d), w(0x8d), w(0xd5), w(0x4e), w(0xa9),\
    w(0x6c), w(0x56), w(0xf4), w(0xea), w(0x65), w(0x7a), w(0xae), w(0x08),\
    w(0xba), w(0x78), w(0x25), w(0x2e), w(0x1c), w(0xa6), w(0xb4), w(0xc6),\
    w(0xe8), w(0xdd), w(0x74), w(0x1f), w(0x4b), w(0xbd), w(0x8b), w(0x8a),\
    w(0x70), w(0x3e), w(0xb5), w(0x66), w(0x48), w(0x03), w(0xf6), w(0x0e),\
    w(0x61), w(0x35), w(0x57), w(0xb9), w(0x86), w(0xc1), w(0x1d), w(0x9e),\
    w(0xe1), w(0xf8), w(0x98), w(0x11), w(0x69), w(0xd9), w(0x8e), w(0x94),\
    w(0x9b), w(0x1e), w(0x87), w(0xe9), w(0xce), w(0x55), w(0x28), w(0xdf),\
    w(0x8c), w(0xa1), w(0x89), w(0x0d), w(0xbf), w(0xe6), w(0x42), w(0x68),\
    w(0x41), w(0x99), w(0x2d), w(0x0f), w(0xb0), w(0x54), w(0xbb), w(0x16)

#define isb_data(w) \
    w(0x52), w(0x09), w(0x6a), w(0xd5), w(0x30), w(0x36), w(0xa5), w(0x38),\
    w(0xbf), w(0x40), w(0xa3), w(0x9e), w(0x81), w(0xf3), w(0xd7), w(0xfb),\
    w(0x7c), w(0xe3), w(0x39), w(0x82), w(0x9b), w(0x2f), w(0xff), w(0x87),\
    w(0x34), w(0x8e), w(0x43), w(0x44), w(0xc4), w(0xde), w(0xe9), w(0xcb),\
    w(0x54), w(0x7b), w(0x94), w(0x32), w(0xa6), w(0xc2), w(0x23), w(0x3d),\
    w(0xee), w(0x4c), w(0x95), w(0x0b), w(0x42), w(0xfa), w(0xc3), w(0x4e),\
    w(0x08), w(0x2e), w(0xa1), w(0x66), w(0x28), w(0xd9), w(0x24), w(0xb2),\
    w(0x76), w(0x5b), w(0xa2), w(0x49), w(0x6d), w(0x8b), w(0xd1), w(0x25),\
    w(0x72), w(0xf8), w(0xf6), w(0x64), w(0x86), w(0x68), w(0x98), w(0x16),\
    w(0xd4), w(0xa4), w(0x5c), w(0xcc), w(0x5d), w(0x65), w(0xb6), w(0x92),\
    w(0x6c), w(0x70), w(0x48), w(0x50), w(0xfd), w(0xed), w(0xb9), w(0xda),\
    w(0x5e), w(0x15), w(0x46), w(0x57), w(0xa7), w(0x8d), w(0x9d), w(0x84),\
    w(0x90), w(0xd8), w(0xab), w(0x00), w(0x8c), w(0xbc), w(0xd3), w(0x0a),\
    w(0xf7), w(0xe4), w(0x58), w(0x05), w(0xb8), w(0xb3), w(0x45), w(0x06),\
    w(0xd0), w(0x2c), w(0x1e), w(0x8f), w(0xca), w(0x3f), w(0x0f), w(0x02),\
    w(0xc1), w(0xaf), w(0xbd), w(0x03), w(0x01), w(0x13), w(0x8a), w(0x6b),\
    w(0x3a), w(0x91), w(0x11), w(0x41), w(0x4f), w(0x67), w(0xdc), w(0xea),\
    w(0x97), w(0xf2), w(0xcf), w(0xce), w(0xf0), w(0xb4), w(0xe6), w(0x73),\
    w(0x96), w(0xac), w(0x74), w(0x22), w(0xe7), w(0xad), w(0x35), w(0x85),\
    w(0xe2), w(0xf9), w(0x37), w(0xe8), w(0x1c), w(0x75), w(0xdf), w(0x6e),\
    w(0x47), w(0xf1), w(0x1a), w(0x71), w(0x1d), w(0x29), w(0xc5), w(0x89),\
    w(0x6f), w(0xb7), w(0x62), w(0x0e), w(0xaa), w(0x18), w(0xbe), w(0x1b),\
    w(0xfc), w(0x56), w(0x3e), w(0x4b), w(0xc6), w(0xd2), w(0x79), w(0x20),\
    w(0x9a), w(0xdb), w(0xc0), w(0xfe), w(0x78), w(0xcd), w(0x5a), w(0xf4),\
    w(0x1f), w(0xdd), w(0xa8), w(0x33), w(0x88), w(0x07), w(0xc7), w(0x31),\
    w(0xb1), w(0x12), w(0x10), w(0x59), w(0x27), w(0x80), w(0xec), w(0x5f),\
    w(0x60), w(0x51), w(0x7f), w(0xa9), w(0x19), w(0xb5), w(0x4a), w(0x0d),\
    w(0x2d), w(0xe5), w(0x7a), w(0x9f), w(0x93), w(0xc9), w(0x9c), w(0xef),\
    w(0xa0), w(0xe0), w(0x3b), w(0x4d), w(0xae), w(0x2a), w(0xf5), w(0xb0),\
    w(0xc8), w(0xeb), w(0xbb), w(0x3c), w(0x83), w(0x53), w(0x99), w(0x61),\
    w(0x17), w(0x2b), w(0x04), w(0x7e), w(0xba), w(0x77), w(0xd6), w(0x26),\
    w(0xe1), w(0x69), w(0x14), w(0x63), w(0x55), w(0x21), w(0x0c), w(0x7d),

#define mm_data(w) \
    w(0x00), w(0x01), w(0x02), w(0x03), w(0x04), w(0x05), w(0x06), w(0x07),\
    w(0x08), w(0x09), w(0x0a), w(0x0b), w(0x0c), w(0x0d), w(0x0e), w(0x0f),\
    w(0x10), w(0x11), w(0x12), w(0x13), w(0x14), w(0x15), w(0x16), w(0x17),\
    w(0x18), w(0x19), w(0x1a), w(0x1b), w(0x1c), w(0x1d), w(0x1e), w(0x1f),\
    w(0x20), w(0x21), w(0x22), w(0x23), w(0x24), w(0x25), w(0x26), w(0x27),\
    w(0x28), w(0x29), w(0x2a), w(0x2b), w(0x2c), w(0x2d), w(0x2e), w(0x2f),\
    w(0x30), w(0x31), w(0x32), w(0x33), w(0x34), w(0x35), w(0x36), w(0x37),\
    w(0x38), w(0x39), w(0x3a), w(0x3b), w(0x3c), w(0x3d), w(0x3e), w(0x3f),\
    w(0x40), w(0x41), w(0x42), w(0x43), w(0x44), w(0x45), w(0x46), w(0x47),\
    w(0x48), w(0x49), w(0x4a), w(0x4b), w(0x4c), w(0x4d), w(0x4e), w(0x4f),\
    w(0x50), w(0x51), w(0x52), w(0x53), w(0x54), w(0x55), w(0x56), w(0x57),\
    w(0x58), w(0x59), w(0x5a), w(0x5b), w(0x5c), w(0x5d), w(0x5e), w(0x5f),\
    w(0x60), w(0x61), w(0x62), w(0x63), w(0x64), w(0x65), w(0x66), w(0x67),\
    w(0x68), w(0x69), w(0x6a), w(0x6b), w(0x6c), w(0x6d), w(0x6e), w(0x6f),\
    w(0x70), w(0x71), w(0x72), w(0x73), w(0x74), w(0x75), w(0x76), w(0x77),\
    w(0x78), w(0x79), w(0x7a), w(0x7b), w(0x7c), w(0x7d), w(0x7e), w(0x7f),\
    w(0x80), w(0x81), w(0x82), w(0x83), w(0x84), w(0x85), w(0x86), w(0x87),\
    w(0x88), w(0x89), w(0x8a), w(0x8b), w(0x8c), w(0x8d), w(0x8e), w(0x8f),\
    w(0x90), w(0x91), w(0x92), w(0x93), w(0x94), w(0x95), w(0x96), w(0x97),\
    w(0x98), w(0x99), w(0x9a), w(0x9b), w(0x9c), w(0x9d), w(0x9e), w(0x9f),\
    w(0xa0), w(0xa1), w(0xa2), w(0xa3), w(0xa4), w(0xa5), w(0xa6), w(0xa7),\
    w(0xa8), w(0xa9), w(0xaa), w(0xab), w(0xac), w(0xad), w(0xae), w(0xaf),\
    w(0xb0), w(0xb1), w(0xb2), w(0xb3), w(0xb4), w(0xb5), w(0xb6), w(0xb7),\
    w(0xb8), w(0xb9), w(0xba), w(0xbb), w(0xbc), w(0xbd), w(0xbe), w(0xbf),\
    w(0xc0), w(0xc1), w(0xc2), w(0xc3), w(0xc4), w(0xc5), w(0xc6), w(0xc7),\
    w(0xc8), w(0xc9), w(0xca), w(0xcb), w(0xcc), w(0xcd), w(0xce), w(0xcf),\
    w(0xd0), w(0xd1), w(0xd2), w(0xd3), w(0xd4), w(0xd5), w(0xd6), w(0xd7),\
    w(0xd8), w(0xd9), w(0xda), w(0xdb), w(0xdc), w(0xdd), w(0xde), w(0xdf),\
    w(0xe0), w(0xe1), w(0xe2), w(0xe3), w(0xe4), w(0xe5), w(0xe6), w(0xe7),\
    w(0xe8), w(0xe9), w(0xea), w(0xeb), w(0xec), w(0xed), w(0xee), w(0xef),\
    w(0xf0), w(0xf1), w(0xf2), w(0xf3), w(0xf4), w(0xf5), w(0xf6), w(0xf7),\
    w(0xf8), w(0xf9), w(0xfa), w(0xfb), w(0xfc), w(0xfd), w(0xfe), w(0xff)

#define h0(x)   (x)

/*  These defines are used to ensure tables are generated in the
    right format depending on the internal byte order required
*/

#define w0(p)   bytes2word(p, 0, 0, 0)
#define w1(p)   bytes2word(0, p, 0, 0)
#define w2(p)   bytes2word(0, 0, p, 0)
#define w3(p)   bytes2word(0, 0, 0, p)

#define u0(p)   bytes2word(f2(p), p, p, f3(p))
#define u1(p)   bytes2word(f3(p), f2(p), p, p)
#define u2(p)   bytes2word(p, f3(p), f2(p), p)
#define u3(p)   bytes2word(p, p, f3(p), f2(p))

#define v0(p)   bytes2word(fe(p), f9(p), fd(p), fb(p))
#define v1(p)   bytes2word(fb(p), fe(p), f9(p), fd(p))
#define v2(p)   bytes2word(fd(p), fb(p), fe(p), f9(p))
#define v3(p)   bytes2word(f9(p), fd(p), fb(p), fe(p))

const aes_32t t_dec(r,c)[RC_LENGTH] =
{
    w0(0x01), w0(0x02), w0(0x04), w0(0x08), w0(0x10),
    w0(0x20), w0(0x40), w0(0x80), w0(0x1b), w0(0x36)
};

#if defined(__BORLANDC__)
    #define concat(s1, s2) s1##s2
    #define d_1(t,n,b,v) const t n[256]    =   { b(concat(v,0)) }
    #define d_4(t,n,b,v) const t n[4][256] = { { b(concat(v,0)) }, { b(concat(v,1)) }, { b(concat(v,2)) }, { b(concat(v,3)) } }
#else
	#define d_1(t,n,b,v) const t n[256]    =   { b(v##0) }
	#define d_4(t,n,b,v) const t n[4][256] = { { b(v##0) }, { b(v##1) }, { b(v##2) }, { b(v##3) } }
#endif

#else   /* declare and instantiate tables for dynamic value generation in in tab.c  */

aes_32t t_dec(r,c)[RC_LENGTH];

#define d_1(t,n,b,v) t  n[256]
#define d_4(t,n,b,v) t  n[4][256]

#endif

#else   /* declare tables without instantiation */

#if defined(FIXED_TABLES)

extern const aes_32t t_dec(r,c)[RC_LENGTH];

#if defined(_MSC_VER) && defined(TABLE_ALIGN)
#define d_1(t,n,b,v) extern __declspec(align(TABLE_ALIGN)) const t  n[256]
#define d_4(t,n,b,v) extern __declspec(align(TABLE_ALIGN)) const t  n[4][256]
#else
#define d_1(t,n,b,v) extern const t  n[256]
#define d_4(t,n,b,v) extern const t  n[4][256]
#endif
#else

extern aes_32t t_dec(r,c)[RC_LENGTH];

#if defined(_MSC_VER) && defined(TABLE_ALIGN)
#define d_1(t,n,b,v) extern __declspec(align(TABLE_ALIGN)) t  n[256]
#define d_4(t,n,b,v) extern __declspec(align(TABLE_ALIGN)) t  n[4][256]
#else
#define d_1(t,n,b,v) extern t  n[256]
#define d_4(t,n,b,v) extern t  n[4][256]
#endif
#endif

#endif

#ifdef  SBX_SET
    d_1(aes_08t, t_dec(s,box), sb_data, h);
#endif
#ifdef  ISB_SET
    d_1(aes_08t, t_dec(i,box), isb_data, h);
#endif

#ifdef  FT1_SET
    d_1(aes_32t, t_dec(f,n), sb_data, u);
#endif
#ifdef  FT4_SET
    d_4(aes_32t, t_dec(f,n), sb_data, u);
#endif

#ifdef  FL1_SET
    d_1(aes_32t, t_dec(f,l), sb_data, w);
#endif
#ifdef  FL4_SET
    d_4(aes_32t, t_dec(f,l), sb_data, w);
#endif

#ifdef  IT1_SET
    d_1(aes_32t, t_dec(i,n), isb_data, v);
#endif
#ifdef  IT4_SET
    d_4(aes_32t, t_dec(i,n), isb_data, v);
#endif

#ifdef  IL1_SET
    d_1(aes_32t, t_dec(i,l), isb_data, w);
#endif
#ifdef  IL4_SET
    d_4(aes_32t, t_dec(i,l), isb_data, w);
#endif

#ifdef  LS1_SET
#ifdef  FL1_SET
#undef  LS1_SET
#else
    d_1(aes_32t, t_dec(l,s), sb_data, w);
#endif
#endif

#ifdef  LS4_SET
#ifdef  FL4_SET
#undef  LS4_SET
#else
    d_4(aes_32t, t_dec(l,s), sb_data, w);
#endif
#endif

#ifdef  IM1_SET
    d_1(aes_32t, t_dec(i,m), mm_data, v);
#endif
#ifdef  IM4_SET
    d_4(aes_32t, t_dec(i,m), mm_data, v);
#endif

#endif

