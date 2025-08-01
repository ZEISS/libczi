// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>

/// A simplistic implementation of the MD5 hash algorithm.
/// The implementation is derived from the public domain MD5 implementation https://github.com/galenguyer/md5.
/// The mode of operation is:
/// * Initialize the MD5 context with `CMd5Sum()`  
/// * Add data to the MD5 context with `update(const void* buffer, size_t buffer_size)` (as many times as needed)  
/// * Call `complete()` to finalize the MD5 context and compute the hash  
/// * Call `getHash(char* pHash)` to retrieve the computed MD5 hash as a 16 byte binary data blob.
class CMd5Sum
{
private:
    constexpr static size_t MD5_HASH_SIZE = 16;

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

    md5_context ctx_;
    md5_digest digest_;
public:
    CMd5Sum();
    void update(const void* buffer, size_t buffer_size);
    void complete();

    /// Copy the resulting MD5 hash into the specified buffer. The buffer must be at least 16 bytes long.
    ///
    /// \param	    pHash	Pointer to a buffer where the MD5 hash will be copied. The hash code is 16 bytes long,
    /// 					and the destination buffer must therefore be at least 16 bytes long.
    void getHash(char* pHash);
private:
    static const std::uint8_t* md5_transform(md5_context* ctx, const void* data, std::uintmax_t size);
};
