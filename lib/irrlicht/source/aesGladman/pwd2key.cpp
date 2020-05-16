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

 This is an implementation of RFC2898, which specifies key derivation from
 a password and a salt value.
*/

#include <memory.h>
#include "hmac.h"

void derive_key(const unsigned char pwd[],  /* the PASSWORD     */
               unsigned int pwd_len,        /* and its length   */
               const unsigned char salt[],  /* the SALT and its */
               unsigned int salt_len,       /* length           */
               unsigned int iter,   /* the number of iterations */
               unsigned char key[], /* space for the output key */
               unsigned int key_len)/* and its required length  */
{
    unsigned int    i, j, k, n_blk;
    unsigned char uu[HMAC_HASH_OUTPUT_SIZE], ux[HMAC_HASH_OUTPUT_SIZE];
    hmac_ctx c1[1], c2[1], c3[1];

    /* set HMAC context (c1) for password               */
    hmac_sha_begin(c1);
    hmac_sha_key(pwd, pwd_len, c1);

    /* set HMAC context (c2) for password and salt      */
    memcpy(c2, c1, sizeof(hmac_ctx));
    hmac_sha_data(salt, salt_len, c2);

    /* find the number of SHA blocks in the key         */
    n_blk = 1 + (key_len - 1) / HMAC_HASH_OUTPUT_SIZE;

    for(i = 0; i < n_blk; ++i) /* for each block in key */
    {
        /* ux[] holds the running xor value             */
        memset(ux, 0, HMAC_HASH_OUTPUT_SIZE);

        /* set HMAC context (c3) for password and salt  */
        memcpy(c3, c2, sizeof(hmac_ctx));

        /* enter additional data for 1st block into uu  */
        uu[0] = (unsigned char)((i + 1) >> 24);
        uu[1] = (unsigned char)((i + 1) >> 16);
        uu[2] = (unsigned char)((i + 1) >> 8);
        uu[3] = (unsigned char)(i + 1);

        /* this is the key mixing iteration         */
        for(j = 0, k = 4; j < iter; ++j)
        {
            /* add previous round data to HMAC      */
            hmac_sha_data(uu, k, c3);

            /* obtain HMAC for uu[]                 */
            hmac_sha_end(uu, HMAC_HASH_OUTPUT_SIZE, c3);

            /* xor into the running xor block       */
            for(k = 0; k < HMAC_HASH_OUTPUT_SIZE; ++k)
                ux[k] ^= uu[k];

            /* set HMAC context (c3) for password   */
            memcpy(c3, c1, sizeof(hmac_ctx));
        }

        /* compile key blocks into the key output   */
        j = 0; k = i * HMAC_HASH_OUTPUT_SIZE;
        while(j < HMAC_HASH_OUTPUT_SIZE && k < key_len)
            key[k++] = ux[j++];
    }
}

#ifdef TEST

#include <stdio.h>

struct
{   unsigned int    pwd_len;
    unsigned int    salt_len;
    unsigned int    it_count;
    unsigned char   *pwd;
    unsigned char   salt[32];
    unsigned char   key[32];
} tests[] =
{
    {   8, 4, 5, (unsigned char*)"password",
        {
            0x12, 0x34, 0x56, 0x78
        },
        {
            0x5c, 0x75, 0xce, 0xf0, 0x1a, 0x96, 0x0d, 0xf7,
            0x4c, 0xb6, 0xb4, 0x9b, 0x9e, 0x38, 0xe6, 0xb5
        }
    },
    {   8, 8, 5, (unsigned char*)"password",
        {
            0x12, 0x34, 0x56, 0x78, 0x78, 0x56, 0x34, 0x12
        },
        {
            0xd1, 0xda, 0xa7, 0x86, 0x15, 0xf2, 0x87, 0xe6,
            0xa1, 0xc8, 0xb1, 0x20, 0xd7, 0x06, 0x2a, 0x49
        }
    },
    {   8, 21, 1, (unsigned char*)"password",
        {
            "ATHENA.MIT.EDUraeburn"
        },
        {
            0xcd, 0xed, 0xb5, 0x28, 0x1b, 0xb2, 0xf8, 0x01,
            0x56, 0x5a, 0x11, 0x22, 0xb2, 0x56, 0x35, 0x15
        }
    },
    {   8, 21, 2, (unsigned char*)"password",
        {
            "ATHENA.MIT.EDUraeburn"
        },
        {
            0x01, 0xdb, 0xee, 0x7f, 0x4a, 0x9e, 0x24, 0x3e,
            0x98, 0x8b, 0x62, 0xc7, 0x3c, 0xda, 0x93, 0x5d
        }
    },
    {   8, 21, 1200, (unsigned char*)"password",
        {
            "ATHENA.MIT.EDUraeburn"
        },
        {
            0x5c, 0x08, 0xeb, 0x61, 0xfd, 0xf7, 0x1e, 0x4e,
            0x4e, 0xc3, 0xcf, 0x6b, 0xa1, 0xf5, 0x51, 0x2b
        }
    }
};

int main()
{   unsigned int    i, j, key_len = 256;
    unsigned char   key[256];

    printf("\nTest of RFC2898 Password Based Key Derivation");
    for(i = 0; i < 5; ++i)
    {
        derive_key(tests[i].pwd, tests[i].pwd_len, tests[i].salt,
                    tests[i].salt_len, tests[i].it_count, key, key_len);

        printf("\ntest %i: ", i + 1);
        printf("key %s", memcmp(tests[i].key, key, 16) ? "is bad" : "is good");
        for(j = 0; j < key_len && j < 64; j += 4)
        {
            if(j % 16 == 0)
                printf("\n");
            printf("0x%02x%02x%02x%02x ", key[j], key[j + 1], key[j + 2], key[j + 3]);
        }
        printf(j < key_len ? " ... \n" : "\n");
    }
    printf("\n");
    return 0;
}

#endif

