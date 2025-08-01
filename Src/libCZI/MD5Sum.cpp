// SPDX-FileCopyrightText: 1991-2 RSA Data Security, Inc.
//
// SPDX-License-Identifier: RSA-MD

#include "MD5Sum.h"
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>


// -> https://github.com/galenguyer/
#if false
namespace
{
#define MD5_HASH_SIZE 16

    struct md5_context
    {
        // state
        unsigned int a;
        unsigned int b;
        unsigned int c;
        unsigned int d;
        // number of bits, modulo 2^64 (lsb first)
        unsigned int count[2];
        // input buffer
        unsigned char input[64];
        // current block
        unsigned int block[16];
    };

    struct md5_digest
    {
        unsigned char bytes[MD5_HASH_SIZE];
    };

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define ROTATE_LEFT(x, s) (x<<s | x>>(32-s))

#define STEP(f, a, b, c, d, x, t, s) ( \
    a += f(b, c, d) + x + t, \
    a = ROTATE_LEFT(a, s), \
    a += b \
)

    void md5_init(struct md5_context* ctx) {
        ctx->a = 0x67452301;
        ctx->b = 0xefcdab89;
        ctx->c = 0x98badcfe;
        ctx->d = 0x10325476;

        ctx->count[0] = 0;
        ctx->count[1] = 0;
    }

    uint8_t* md5_transform(struct md5_context* ctx, const void* data, uintmax_t size) {
        uint8_t* ptr = (uint8_t*)data;
        uint32_t a, b, c, d, aa, bb, cc, dd;

#define GET(n) (ctx->block[(n)])
#define SET(n) (ctx->block[(n)] =        \
          ((uint32_t)ptr[(n)*4 + 0] << 0 )   \
        | ((uint32_t)ptr[(n)*4 + 1] << 8 )   \
        | ((uint32_t)ptr[(n)*4 + 2] << 16)   \
        | ((uint32_t)ptr[(n)*4 + 3] << 24) )

        a = ctx->a;
        b = ctx->b;
        c = ctx->c;
        d = ctx->d;

        do {
            aa = a;
            bb = b;
            cc = c;
            dd = d;

            STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7);
            STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12);
            STEP(F, c, d, a, b, SET(2), 0x242070db, 17);
            STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22);
            STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7);
            STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12);
            STEP(F, c, d, a, b, SET(6), 0xa8304613, 17);
            STEP(F, b, c, d, a, SET(7), 0xfd469501, 22);
            STEP(F, a, b, c, d, SET(8), 0x698098d8, 7);
            STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12);
            STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17);
            STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22);
            STEP(F, a, b, c, d, SET(12), 0x6b901122, 7);
            STEP(F, d, a, b, c, SET(13), 0xfd987193, 12);
            STEP(F, c, d, a, b, SET(14), 0xa679438e, 17);
            STEP(F, b, c, d, a, SET(15), 0x49b40821, 22);

            STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5);
            STEP(G, d, a, b, c, GET(6), 0xc040b340, 9);
            STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14);
            STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20);
            STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5);
            STEP(G, d, a, b, c, GET(10), 0x02441453, 9);
            STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14);
            STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20);
            STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5);
            STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9);
            STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14);
            STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20);
            STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5);
            STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9);
            STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14);
            STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20);

            STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4);
            STEP(H, d, a, b, c, GET(8), 0x8771f681, 11);
            STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16);
            STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23);
            STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4);
            STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11);
            STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16);
            STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23);
            STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4);
            STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11);
            STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16);
            STEP(H, b, c, d, a, GET(6), 0x04881d05, 23);
            STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4);
            STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11);
            STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16);
            STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23);

            STEP(I, a, b, c, d, GET(0), 0xf4292244, 6);
            STEP(I, d, a, b, c, GET(7), 0x432aff97, 10);
            STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15);
            STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21);
            STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6);
            STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10);
            STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15);
            STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21);
            STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6);
            STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10);
            STEP(I, c, d, a, b, GET(6), 0xa3014314, 15);
            STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21);
            STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6);
            STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10);
            STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15);
            STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21);

            a += aa;
            b += bb;
            c += cc;
            d += dd;

            ptr += 64;
        }
        while (size -= 64);

        ctx->a = a;
        ctx->b = b;
        ctx->c = c;
        ctx->d = d;

#undef GET
#undef SET

        return ptr;
    }

    void md5_update(struct md5_context* ctx, const void* buffer, uint32_t buffer_size) {
        uint32_t saved_low = ctx->count[0];
        uint32_t used;
        uint32_t free;

        if ((ctx->count[0] = ((saved_low + buffer_size) & 0x1fffffff)) < saved_low) {
            ctx->count[1]++;
        }
        ctx->count[1] += (uint32_t)(buffer_size >> 29);

        used = saved_low & 0x3f;

        if (used) {
            free = 64 - used;

            if (buffer_size < free) {
                memcpy(&ctx->input[used], buffer, buffer_size);
                return;
            }

            memcpy(&ctx->input[used], buffer, free);
            buffer = (uint8_t*)buffer + free;
            buffer_size -= free;
            md5_transform(ctx, ctx->input, 64);
        }

        if (buffer_size >= 64) {
            buffer = md5_transform(ctx, buffer, buffer_size & ~(unsigned long)0x3f);
            buffer_size = buffer_size % 64;
        }

        memcpy(ctx->input, buffer, buffer_size);
    }

    void md5_finalize(struct md5_context* ctx, struct md5_digest* digest) {
        uint32_t used = ctx->count[0] & 0x3f;
        ctx->input[used++] = 0x80;
        uint32_t free = 64 - used;

        if (free < 8) {
            memset(&ctx->input[used], 0, free);
            md5_transform(ctx, ctx->input, 64);
            used = 0;
            free = 64;
        }

        memset(&ctx->input[used], 0, free - 8);

        ctx->count[0] <<= 3;
        ctx->input[56] = (uint8_t)(ctx->count[0]);
        ctx->input[57] = (uint8_t)(ctx->count[0] >> 8);
        ctx->input[58] = (uint8_t)(ctx->count[0] >> 16);
        ctx->input[59] = (uint8_t)(ctx->count[0] >> 24);
        ctx->input[60] = (uint8_t)(ctx->count[1]);
        ctx->input[61] = (uint8_t)(ctx->count[1] >> 8);
        ctx->input[62] = (uint8_t)(ctx->count[1] >> 16);
        ctx->input[63] = (uint8_t)(ctx->count[1] >> 24);

        md5_transform(ctx, ctx->input, 64);

        digest->bytes[0] = (uint8_t)(ctx->a);
        digest->bytes[1] = (uint8_t)(ctx->a >> 8);
        digest->bytes[2] = (uint8_t)(ctx->a >> 16);
        digest->bytes[3] = (uint8_t)(ctx->a >> 24);
        digest->bytes[4] = (uint8_t)(ctx->b);
        digest->bytes[5] = (uint8_t)(ctx->b >> 8);
        digest->bytes[6] = (uint8_t)(ctx->b >> 16);
        digest->bytes[7] = (uint8_t)(ctx->b >> 24);
        digest->bytes[8] = (uint8_t)(ctx->c);
        digest->bytes[9] = (uint8_t)(ctx->c >> 8);
        digest->bytes[10] = (uint8_t)(ctx->c >> 16);
        digest->bytes[11] = (uint8_t)(ctx->c >> 24);
        digest->bytes[12] = (uint8_t)(ctx->d);
        digest->bytes[13] = (uint8_t)(ctx->d >> 8);
        digest->bytes[14] = (uint8_t)(ctx->d >> 16);
        digest->bytes[15] = (uint8_t)(ctx->d >> 24);
    }

    //char* md5(const char* input) {
    //    struct md5_context context;
    //    struct md5_digest digest;

    //    md5_init(&context);
    //    md5_update(&context, input, (unsigned long)strlen(input));
    //    md5_finalize(&context, &digest);

    //    char* output = malloc(sizeof(char) * (sizeof(digest) * 2 + 1));
    //    output[sizeof(digest)] = '\0';

    //    for (int i = 0; i < sizeof(digest); i++)
    //    {
    //        sprintf(&output[i * 2], "%2.2x", digest.bytes[i]);
    //    }

    //    return output;
    //}
}
#endif



#if false
/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
*/

#include <string.h>
#include "MD5Sum.h"

unsigned char CMd5Sum::PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

CMd5Sum::CMd5Sum()
{
    MD5_init();
    m_count[0] = m_count[1] = 0;
    // Load magic initialization constants.
    m_state[0] = 0x67452301;
    m_state[1] = 0xefcdab89;
    m_state[2] = 0x98badcfe;
    m_state[3] = 0x10325476;

    m_complete = false;
}

void CMd5Sum::MD5_init()
{
    // initialize.
    MD5_memset((POINTER)m_count, 0, sizeof(m_count));
    MD5_memset((POINTER)m_state, 0, sizeof(m_state));
    MD5_memset((POINTER)m_buffer, 0, sizeof(m_buffer));
}

void CMd5Sum::update(const void* pData, size_t dwSize)
{
    MD5Update((unsigned char*)(pData), dwSize);
}

void CMd5Sum::complete()
{
    MD5Final(m_digest);
    m_complete = true;
}

void CMd5Sum::getHash(char* pHash)
{
    if (m_complete) {
        MD5_memcpy((POINTER)pHash, m_digest, 16);
    }
}

void CMd5Sum::MD5Update(unsigned char* input,       // input block
    size_t inputLen)    // length of input block
{
    // MD5 block update operation. Continues an MD5 message-digest
    // operation, processing another message block, and updating the
    // context.

    size_t i;
    size_t index;
    size_t partLen;

    // Compute number of bytes mod 64
    index = (size_t)((m_count[0] >> 3) & 0x3F);

    // Update number of bits
    m_count[0] += (UINT4)inputLen << 3;
    if (m_count[0] < ((UINT4)inputLen << 3)) {
        m_count[1]++;
    }
    m_count[1] += ((UINT4)inputLen >> 29);

    partLen = 64 - index;

    // Transform as many times as possible.
    if (inputLen >= partLen) {
        MD5_memcpy((POINTER)&m_buffer[index], (POINTER)input, partLen);
        MD5Transform((unsigned char*)m_buffer);

        for (i = partLen; i + 63 < inputLen; i += 64) {
            MD5Transform(&input[i]);
        }

        index = 0;

    }
    else {
        i = 0;
    }

    // Buffer remaining input
    MD5_memcpy((POINTER)&m_buffer[index], (POINTER)&input[i],
        inputLen - i);

}

void CMd5Sum::MD5Final(unsigned char* digest)       // message digest
{
    // MD5 finalization. Ends an MD5 message-digest operation, writing the
    // the message digest and zeroizing the context.

    unsigned char bits[8];
    size_t index, padLen;

    // Save number of bits
    Encode(bits, m_count, 8);

    // Pad out to 56 mod 64.
    index = (size_t)((m_count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5Update(PADDING, padLen);

    // Append length (before padding)
    MD5Update(bits, 8);

    // Store state in digest
    Encode(digest, m_state, 16);

    // Zeroize sensitive information.
    MD5_init();
}

void CMd5Sum::MD5Transform(unsigned char* block)
{
    // MD5 basic transformation. Transforms state based on block.

    UINT4 a = m_state[0];
    UINT4 b = m_state[1];
    UINT4 c = m_state[2];
    UINT4 d = m_state[3];
    UINT4 x[16];

    Decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;

    // Zeroize sensitive information.
}

void CMd5Sum::Encode(unsigned char* output, const UINT4* input, size_t len)
{
    // Encodes input (UINT4) into output (unsigned char). Assumes len is
    // a multiple of 4.

    size_t i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (unsigned char)(input[i] & 0xff);
        output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
        output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
        output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
    }
}

void CMd5Sum::Decode(UINT4* output, unsigned char* input, size_t len)
{
    // Decodes input (unsigned char) into output (UINT4). Assumes len is
    // a multiple of 4.

    size_t i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[i] =
            ((UINT4)input[j]) | (((UINT4)input[j + 1]) << 8) |
            (((UINT4)input[j + 2]) << 16) | (((UINT4)input[j + 3]) << 24);
    }
}
#endif


CMd5Sum::CMd5Sum()
{
    this->ctx_.a = 0x67452301;
    this->ctx_.b = 0xefcdab89;
    this->ctx_.c = 0x98badcfe;
    this->ctx_.d = 0x10325476;
    this->ctx_.count[0] = 0;
    this->ctx_.count[1] = 0;
}

void CMd5Sum::update(const void* buffer, size_t buffer_size)
{
    const uint32_t saved_low = this->ctx_.count[0];

    if ((this->ctx_.count[0] = ((saved_low + buffer_size) & 0x1fffffff)) < saved_low)
    {
        this->ctx_.count[1]++;
    }

    this->ctx_.count[1] += static_cast<uint32_t>(buffer_size >> 29);

    const uint32_t used = saved_low & 0x3f;

    if (used) 
    {
        const uint32_t free = 64 - used;

        if (buffer_size < free) 
        {
            memcpy(&this->ctx_.input[used], buffer, buffer_size);
            return;
        }

        memcpy(&this->ctx_.input[used], buffer, free);
        buffer = static_cast<const uint8_t*>(buffer) + free;
        buffer_size -= free;
        md5_transform(&this->ctx_, this->ctx_.input, 64);
    }

    if (buffer_size >= 64) 
    {
        buffer = md5_transform(&this->ctx_, buffer, buffer_size & ~static_cast<unsigned long>(0x3f));
        buffer_size = buffer_size % 64;
    }

    memcpy(this->ctx_.input, buffer, buffer_size);
}

void CMd5Sum::complete()
{
    uint32_t used = this->ctx_.count[0] & 0x3f;
    this->ctx_.input[used++] = 0x80;
    uint32_t free = 64 - used;

    if (free < 8) 
    {
        memset(&this->ctx_.input[used], 0, free);
        md5_transform(&this->ctx_, this->ctx_.input, 64);
        used = 0;
        free = 64;
    }

    memset(&this->ctx_.input[used], 0, free - 8);

    this->ctx_.count[0] <<= 3;
    this->ctx_.input[56] = static_cast<uint8_t>(this->ctx_.count[0]);
    this->ctx_.input[57] = static_cast<uint8_t>(this->ctx_.count[0] >> 8);
    this->ctx_.input[58] = static_cast<uint8_t>(this->ctx_.count[0] >> 16);
    this->ctx_.input[59] = static_cast<uint8_t>(this->ctx_.count[0] >> 24);
    this->ctx_.input[60] = static_cast<uint8_t>(this->ctx_.count[1]);
    this->ctx_.input[61] = static_cast<uint8_t>(this->ctx_.count[1] >> 8);
    this->ctx_.input[62] = static_cast<uint8_t>(this->ctx_.count[1] >> 16);
    this->ctx_.input[63] = static_cast<uint8_t>(this->ctx_.count[1] >> 24);

    md5_transform(&this->ctx_, this->ctx_.input, 64);

    this->digest_.bytes[0] = (uint8_t)(this->ctx_.a);
    this->digest_.bytes[1] = (uint8_t)(this->ctx_.a >> 8);
    this->digest_.bytes[2] = (uint8_t)(this->ctx_.a >> 16);
    this->digest_.bytes[3] = (uint8_t)(this->ctx_.a >> 24);
    this->digest_.bytes[4] = (uint8_t)(this->ctx_.b);
    this->digest_.bytes[5] = (uint8_t)(this->ctx_.b >> 8);
    this->digest_.bytes[6] = (uint8_t)(this->ctx_.b >> 16);
    this->digest_.bytes[7] = (uint8_t)(this->ctx_.b >> 24);
    this->digest_.bytes[8] = (uint8_t)(this->ctx_.c);
    this->digest_.bytes[9] = (uint8_t)(this->ctx_.c >> 8);
    this->digest_.bytes[10] = (uint8_t)(this->ctx_.c >> 16);
    this->digest_.bytes[11] = (uint8_t)(this->ctx_.c >> 24);
    this->digest_.bytes[12] = (uint8_t)(this->ctx_.d);
    this->digest_.bytes[13] = (uint8_t)(this->ctx_.d >> 8);
    this->digest_.bytes[14] = (uint8_t)(this->ctx_.d >> 16);
    this->digest_.bytes[15] = (uint8_t)(this->ctx_.d >> 24);
}

void CMd5Sum::getHash(char* pHash)
{
    memcpy(pHash, this->digest_.bytes, sizeof(this->digest_.bytes));
}

/*static*/const std::uint8_t* CMd5Sum::md5_transform(md5_context* ctx, const void* data, std::uintmax_t size)
{
    const uint8_t* ptr = (const uint8_t*)data;
    uint32_t a, b, c, d, aa, bb, cc, dd;

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))
#define ROTATE_LEFT(x, s) (x<<s | x>>(32-s))
#define STEP(f, a, b, c, d, x, t, s) ( \
a += f(b, c, d) + x + t, \
a = ROTATE_LEFT(a, s), \
a += b \
)
#define GET(n) (ctx->block[(n)])
#define SET(n) (ctx->block[(n)] =        \
          ((uint32_t)ptr[(n)*4 + 0] << 0 )   \
        | ((uint32_t)ptr[(n)*4 + 1] << 8 )   \
        | ((uint32_t)ptr[(n)*4 + 2] << 16)   \
        | ((uint32_t)ptr[(n)*4 + 3] << 24) )

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    do 
    {
        aa = a;
        bb = b;
        cc = c;
        dd = d;

        STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7);
        STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12);
        STEP(F, c, d, a, b, SET(2), 0x242070db, 17);
        STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22);
        STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7);
        STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12);
        STEP(F, c, d, a, b, SET(6), 0xa8304613, 17);
        STEP(F, b, c, d, a, SET(7), 0xfd469501, 22);
        STEP(F, a, b, c, d, SET(8), 0x698098d8, 7);
        STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12);
        STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17);
        STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22);
        STEP(F, a, b, c, d, SET(12), 0x6b901122, 7);
        STEP(F, d, a, b, c, SET(13), 0xfd987193, 12);
        STEP(F, c, d, a, b, SET(14), 0xa679438e, 17);
        STEP(F, b, c, d, a, SET(15), 0x49b40821, 22);

        STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5);
        STEP(G, d, a, b, c, GET(6), 0xc040b340, 9);
        STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14);
        STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20);
        STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5);
        STEP(G, d, a, b, c, GET(10), 0x02441453, 9);
        STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14);
        STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20);
        STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5);
        STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9);
        STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14);
        STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20);
        STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5);
        STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9);
        STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14);
        STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20);

        STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4);
        STEP(H, d, a, b, c, GET(8), 0x8771f681, 11);
        STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16);
        STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23);
        STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4);
        STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11);
        STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16);
        STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23);
        STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4);
        STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11);
        STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16);
        STEP(H, b, c, d, a, GET(6), 0x04881d05, 23);
        STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4);
        STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11);
        STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16);
        STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23);

        STEP(I, a, b, c, d, GET(0), 0xf4292244, 6);
        STEP(I, d, a, b, c, GET(7), 0x432aff97, 10);
        STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15);
        STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21);
        STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6);
        STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10);
        STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15);
        STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21);
        STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6);
        STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10);
        STEP(I, c, d, a, b, GET(6), 0xa3014314, 15);
        STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21);
        STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6);
        STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10);
        STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15);
        STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21);

        a += aa;
        b += bb;
        c += cc;
        d += dd;

        ptr += 64;
    }
    while (size -= 64);

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

#undef GET
#undef SET
#undef F
#undef G
#undef H
#undef I
#undef ROTATE_LEFT
#undef STEP

    return ptr;
}
