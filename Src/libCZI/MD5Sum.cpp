// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MD5Sum.h"
#include <cstdint>
#include <cstring>

using namespace std;

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
        buffer = md5_transform(&this->ctx_, buffer, buffer_size & ~static_cast<uint32_t>(0x3f));
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

    this->digest_.bytes[0] = static_cast<uint8_t>(this->ctx_.a);
    this->digest_.bytes[1] = static_cast<uint8_t>(this->ctx_.a >> 8);
    this->digest_.bytes[2] = static_cast<uint8_t>(this->ctx_.a >> 16);
    this->digest_.bytes[3] = static_cast<uint8_t>(this->ctx_.a >> 24);
    this->digest_.bytes[4] = static_cast<uint8_t>(this->ctx_.b);
    this->digest_.bytes[5] = static_cast<uint8_t>(this->ctx_.b >> 8);
    this->digest_.bytes[6] = static_cast<uint8_t>(this->ctx_.b >> 16);
    this->digest_.bytes[7] = static_cast<uint8_t>(this->ctx_.b >> 24);
    this->digest_.bytes[8] = static_cast<uint8_t>(this->ctx_.c);
    this->digest_.bytes[9] = static_cast<uint8_t>(this->ctx_.c >> 8);
    this->digest_.bytes[10] = static_cast<uint8_t>(this->ctx_.c >> 16);
    this->digest_.bytes[11] = static_cast<uint8_t>(this->ctx_.c >> 24);
    this->digest_.bytes[12] = static_cast<uint8_t>(this->ctx_.d);
    this->digest_.bytes[13] = static_cast<uint8_t>(this->ctx_.d >> 8);
    this->digest_.bytes[14] = static_cast<uint8_t>(this->ctx_.d >> 16);
    this->digest_.bytes[15] = static_cast<uint8_t>(this->ctx_.d >> 24);
}

void CMd5Sum::getHash(char* pHash)
{
    memcpy(pHash, this->digest_.bytes, sizeof(this->digest_.bytes));
}

/*static*/const std::uint8_t* CMd5Sum::md5_transform(md5_context* ctx, const void* data, std::uintmax_t size)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
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
#define SET(n) (ctx->block[(n)] =            \
        ((uint32_t)ptr[(n)*4 + 0] << 0 )     \
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
