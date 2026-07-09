// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_Config_Internal.h"
#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
#include "decoder_chunkedcompression.h"

#include "libCZI_compress.h"
#include "utilities.h"

#include <cstring>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include "bitmapData.h"
#include <zstd.h>
#if (ZSTD_VERSION_MAJOR >= 1 && ZSTD_VERSION_MINOR >= 5) 
#include <zstd_errors.h>
#else
#include <common/zstd_errors.h>
#endif

#include <lz4.h>

using namespace std;
using namespace libCZI;
using namespace libCZI::detail;

namespace
{
    bool CheckIfCompressedChunkSizesAreValid(const std::tuple<size_t, ChunkedCompressionHeaderHelper::HeaderInfo>& size_and_header_info, size_t size_of_data)
    {
        const size_t header_size = get<0>(size_and_header_info);

        // The header alone must not exceed the total data size.
        if (header_size > size_of_data)
        {
            return false;
        }

        // Accumulate the compressed chunk sizes directly against the remaining budget.
        // This avoids a two-step approach (accumulate-then-add) where either addition
        // could silently wrap on malformed input with many / large chunks:
        //   - sum of compressedSizes could overflow size_t
        //   - header_size + that sum could then also overflow size_t
        // Both wraps would make the subsequent "<= size_of_data" check pass erroneously,
        // allowing out-of-bounds reads when the chunks are later iterated.
        //
        // Loop invariant: running_total <= size_of_data, so (size_of_data - running_total)
        // never underflows and the addition running_total += ... never overflows.
        size_t running_total = header_size;
        for (const auto& chunk : get<1>(size_and_header_info).chunks)
        {
            // compressedSize is uint32_t; widening to size_t is safe on all platforms.
            if (static_cast<size_t>(chunk.compressedSize) > size_of_data - running_total)
            {
                return false;
            }

            running_total += chunk.compressedSize;
        }

        // running_total == header_size + sum(compressedSizes) and is guaranteed <= size_of_data.
        return true;
    }
}

/*static*/const char* CChunkedCompressionDecoder::kOption_IgnorePreprocessingInstruction = "IgnorePreprocessingInstruction";
/*static*/const char* CChunkedCompressionDecoder::kOption_handle_data_size_mismatch = "handle_data_size_mismatch";

/*static*/std::shared_ptr<CChunkedCompressionDecoder> CChunkedCompressionDecoder::Create()
{
    return make_shared<CChunkedCompressionDecoder>();
}

std::shared_ptr<libCZI::IBitmapData> CChunkedCompressionDecoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments)
{
    if (pixelType == nullptr || width == nullptr || height == nullptr)
    {
        throw invalid_argument("pixeltype, width and height must be specified.");
    }

    auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(ptrData, size);
    const auto& chunks = get<1>(size_and_header_info).chunks;

    // Sum all per-chunk uncompressed sizes.  uncompressedSize is uint32_t, so each
    // individual value widens safely to size_t.  However, the accumulated total can
    // still overflow size_t when a malformed header contains many / large chunks.
    // An overflowing sum would silently corrupt the size comparison, the error
    // message, and the downstream decode path, so we detect and reject it here.
    size_t total_size_of_decompressed_data = 0;
    for (const auto& chunk : chunks)
    {
        if (static_cast<size_t>(chunk.uncompressedSize) > (numeric_limits<size_t>::max)() - total_size_of_decompressed_data)
        {
            throw runtime_error("Overflow computing total decompressed size: chunked-compression header is malformed.");
        }

        total_size_of_decompressed_data += chunk.uncompressedSize;
    }

    if (get<1>(size_and_header_info).hiLoBytePackingApplied == true &&
        Utilities::ContainsToken(additional_arguments, CChunkedCompressionDecoder::kOption_IgnorePreprocessingInstruction))
    {
        // if the "ignore preprocessing instruction" option is specified, then we ignore the instruction about hi-lo byte packing in the header 
        // and proceed as if no preprocessing was applied
        get<1>(size_and_header_info).hiLoBytePackingApplied = false;
    }

    // now - if this size matches the expected size (given by width, height and pixel type), then we can proceed with the decompression, otherwise we
    //  use the "resolution protocol"

    // calculate the expected size of the uncompressed data (with overflow checks)
    const size_t bytes_per_pixel = static_cast<size_t>(Utils::GetBytesPerPixel(*pixelType));
    if (static_cast<size_t>(*width) > (numeric_limits<size_t>::max)() / bytes_per_pixel)
    {
        throw runtime_error("Invalid bitmap width/pixel type: size overflow.");
    }

    size_t stride = static_cast<size_t>(*width) * bytes_per_pixel;
    if (static_cast<size_t>(*height) > (numeric_limits<size_t>::max)() / stride)
    {
        throw runtime_error("Invalid bitmap height/stride: size overflow.");
    }

    size_t expected_size = static_cast<size_t>(*height) * stride;
    if (expected_size == total_size_of_decompressed_data)
    {
        // ok, so the reported sizes add up to exactly what is expected
        DecodeInformation decode_information{ *pixelType, *width, *height, ptrData, size, std::move(size_and_header_info) };
        return CChunkedCompressionDecoder::DecodeSizeMatchesExactly(decode_information);
    }

    const bool handle_data_size_mismatch = Utilities::ContainsToken(additional_arguments, CChunkedCompressionDecoder::kOption_handle_data_size_mismatch);
    if (!handle_data_size_mismatch)
    {
        stringstream ss;
        ss << "chunked-compressed data has unexpected size. Expected: " << expected_size << ", actual: " << total_size_of_decompressed_data;
        throw runtime_error(ss.str());
    }

    DecodeInformation decode_information{ *pixelType, *width, *height, ptrData, size, std::move(size_and_header_info) };
    return CChunkedCompressionDecoder::DecodeSizeMatchesHandleSizeMismatch(decode_information, total_size_of_decompressed_data);

}

/*static*/std::shared_ptr<libCZI::IBitmapData> CChunkedCompressionDecoder::DecodeSizeMatchesExactly(const DecodeInformation& decode_information)
{
    // first - we check whether the compressed chunk sizes are valid (i.e. that they do not exceed the total size of the data)
    if (!CheckIfCompressedChunkSizesAreValid(decode_information.chunk_header_info, decode_information.size_subblock_data))
    {
        throw runtime_error("The compressed chunk sizes reported in the header do not match the total size of the data.");
    }

    // calculate the expected size of the uncompressed data
    const size_t stride = decode_information.width * static_cast<size_t>(Utils::GetBytesPerPixel(decode_information.pixelType));
    if (stride == 0 || stride > numeric_limits<uint32_t>::max())
    {
        throw runtime_error("Invalid stride calculated from width and pixel type.");
    }

    // note that the cast to uint32_t is safe because we have checked that the stride is not larger than uint32_t::max before
    auto bitmap = CStdBitmapData::Create(decode_information.pixelType, decode_information.width, decode_information.height, static_cast<uint32_t>(stride));

    // now - decode the chunks, one by one, directly into the bitmap (we can do this because we know that 
    //   the total size of the decompressed data matches the expected size of the bitmap)

    const uint64_t source_offset = get<0>(decode_information.chunk_header_info);  // chunk-data starts after the chunk-header, so we start reading from there
    auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);

    if (get<1>(decode_information.chunk_header_info).hiLoBytePackingApplied)
    {
        CChunkedCompressionDecoder::DecompressLoHiBytePackingPreprocessing(decode_information, source_offset, bitmap_lock_info.ptrDataRoi, bitmap_lock_info.size);
    }
    else
    {
        CChunkedCompressionDecoder::DecompressNoPreprocessing(decode_information, source_offset, bitmap_lock_info.ptrDataRoi, bitmap_lock_info.size);
    }

    return bitmap;
}

/*static*/void CChunkedCompressionDecoder::DecompressNoPreprocessing(const DecodeInformation& decode_information, uint64_t source_offset, void* destination, size_t size_destination)
{
    size_t destination_offset = 0;

    for (size_t i = 0; i < get<1>(decode_information.chunk_header_info).chunks.size(); ++i)
    {
        const auto& chunk = get<1>(decode_information.chunk_header_info).chunks[i];

        if (chunk.uncompressedSize > size_destination - destination_offset)
        {
            throw runtime_error("Chunked decompression would exceed the destination buffer size.");
        }

        size_t decompressed_size = 0;

        switch (get<1>(decode_information.chunk_header_info).codec)
        {
        case ChunkedCompressionHeaderHelper::Codec::ZStd:
        {
            decompressed_size = ZSTD_decompress(
                static_cast<uint8_t*>(destination) + destination_offset,
                chunk.uncompressedSize,
                static_cast<const uint8_t*>(decode_information.ptr_subblock_data) + source_offset,
                chunk.compressedSize);

            if (ZSTD_isError(decompressed_size))
            {
                throw runtime_error("ZStd decompression of chunk failed.");
            }

            break;
        }
        case ChunkedCompressionHeaderHelper::Codec::Lz4:
        {
            if (chunk.compressedSize > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
                chunk.uncompressedSize > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
            {
                throw runtime_error("Chunk size is too large for LZ4 decompression.");
            }

            const int lz4_decompressed_size = LZ4_decompress_safe(
                static_cast<const char*>(decode_information.ptr_subblock_data) + source_offset,
                static_cast<char*>(destination) + destination_offset,
                static_cast<int>(chunk.compressedSize),
                static_cast<int>(chunk.uncompressedSize));

            if (lz4_decompressed_size < 0)
            {
                throw runtime_error("LZ4 decompression of chunk failed.");
            }

            decompressed_size = static_cast<size_t>(lz4_decompressed_size);
            break;
        }
        default:
            throw runtime_error("Unsupported codec for chunked decompression.");
        }

        if (decompressed_size != chunk.uncompressedSize)
        {
            throw runtime_error("Decompressed chunk size does not match the uncompressed size declared in the chunked-compression header.");
        }

        destination_offset += chunk.uncompressedSize;
        source_offset += chunk.compressedSize;
    }
}

/*static*/void CChunkedCompressionDecoder::DecompressLoHiBytePackingPreprocessing(const DecodeInformation& decode_information, uint64_t source_offset, void* destination, size_t size_destination)
{
    // Allocate a staging buffer for the decompressed data (since we need to do the hi-lo byte unpacking as a post-processing step, we cannot directly decompress into the destination buffer).
    // This buffer must be large enough to hold the largest (decompressed) chunk.
    size_t largest_decompressed_chunk_size = 0;
    for (const auto& chunk : get<1>(decode_information.chunk_header_info).chunks)
    {
        if (chunk.uncompressedSize > largest_decompressed_chunk_size)
        {
            largest_decompressed_chunk_size = chunk.uncompressedSize;
        }
    }

    std::vector<uint8_t> staging_buffer(largest_decompressed_chunk_size);

    size_t destination_offset = 0;
    for (size_t i = 0; i < get<1>(decode_information.chunk_header_info).chunks.size(); ++i)
    {
        const auto& chunk = get<1>(decode_information.chunk_header_info).chunks[i];

        if (chunk.uncompressedSize > size_destination - destination_offset)
        {
            throw runtime_error("Chunked decompression would exceed the destination buffer size.");
        }

        size_t decompressed_size = 0;

        switch (get<1>(decode_information.chunk_header_info).codec)
        {
        case ChunkedCompressionHeaderHelper::Codec::ZStd:
        {
            decompressed_size = ZSTD_decompress(
                staging_buffer.data(),
                staging_buffer.size(),
                static_cast<const uint8_t*>(decode_information.ptr_subblock_data) + source_offset,
                chunk.compressedSize);

            if (ZSTD_isError(decompressed_size))
            {
                throw runtime_error("ZStd decompression of chunk failed.");
            }

            break;
        }
        case ChunkedCompressionHeaderHelper::Codec::Lz4:
        {
            if (chunk.compressedSize > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
                chunk.uncompressedSize > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
            {
                throw runtime_error("Chunk size is too large for LZ4 decompression.");
            }

            const int lz4_decompressed_size = LZ4_decompress_safe(
                static_cast<const char*>(decode_information.ptr_subblock_data) + source_offset,
                reinterpret_cast<char*>(staging_buffer.data()),
                static_cast<int>(chunk.compressedSize),
                static_cast<int>(chunk.uncompressedSize));

            if (lz4_decompressed_size < 0)
            {
                throw runtime_error("LZ4 decompression of chunk failed.");
            }

            decompressed_size = static_cast<size_t>(lz4_decompressed_size);
            break;
        }
        default:
            throw runtime_error("Unsupported codec for chunked decompression.");
        }

        if (decompressed_size != chunk.uncompressedSize)
        {
            throw runtime_error("Decompressed chunk size does not match the uncompressed size declared in the chunked-compression header.");
        }

        // Each chunk is a self-contained LoHiByte-packed flat buffer without row-stride concerns.
        // Chunk sizes are byte-based and may be odd, so use the byte-sized flat-buffer helper
        // instead of the strided bitmap-row variant.
        uint8_t* chunk_destination = static_cast<uint8_t*>(destination) + destination_offset;
        LoHiBytePackUnpack::LoHiBytePackStridedByteSized(
            staging_buffer.data(),
            chunk_destination,
            decompressed_size);

        destination_offset += chunk.uncompressedSize;
        source_offset += chunk.compressedSize;
    }
}

/*static*/std::shared_ptr<libCZI::IBitmapData> CChunkedCompressionDecoder::DecodeSizeMatchesHandleSizeMismatch(const DecodeInformation& decode_information, size_t total_size_of_decompressed_data)
{
    // first - we check whether the compressed chunk sizes are valid (i.e. that they do not exceed the total size of the data)
    if (!CheckIfCompressedChunkSizesAreValid(decode_information.chunk_header_info, decode_information.size_subblock_data))
    {
        throw runtime_error("The compressed chunk sizes reported in the header do not match the total size of the data.");
    }

    // calculate the expected size of the uncompressed data
    const size_t stride = decode_information.width * static_cast<size_t>(Utils::GetBytesPerPixel(decode_information.pixelType));
    if (stride == 0 || stride > numeric_limits<uint32_t>::max())
    {
        throw runtime_error("Invalid stride calculated from width and pixel type.");
    }

    if (static_cast<size_t>(decode_information.height) > (numeric_limits<size_t>::max)() / stride)
    {
        throw runtime_error("Invalid bitmap height/stride: size overflow.");
    }

    const size_t expected_size = static_cast<size_t>(decode_information.height) * stride;
    const uint64_t source_offset = get<0>(decode_information.chunk_header_info);  // chunk-data starts after the chunk-header, so we start reading from there

    auto bitmap = CStdBitmapData::Create(decode_information.pixelType, decode_information.width, decode_information.height, static_cast<uint32_t>(stride));

    if (total_size_of_decompressed_data < expected_size)
    {
        // sizes mismatch, and the decoded size is less than expected - so we need to decode and then fill up with zeroes
        auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);

        if (get<1>(decode_information.chunk_header_info).hiLoBytePackingApplied)
        {
            CChunkedCompressionDecoder::DecompressLoHiBytePackingPreprocessing(decode_information, source_offset, bitmap_lock_info.ptrDataRoi, bitmap_lock_info.size);
        }
        else
        {
            CChunkedCompressionDecoder::DecompressNoPreprocessing(decode_information, source_offset, bitmap_lock_info.ptrDataRoi, bitmap_lock_info.size);
        }

        // fill up the rest with zeroes
        memset(static_cast<uint8_t*>(bitmap_lock_info.ptrDataRoi) + total_size_of_decompressed_data, 0, expected_size - total_size_of_decompressed_data);
        return bitmap;
    }
    else
    {
        // sizes mismatch, and the decoded size is larger than expected - we need to decode to a temporary buffer, and
        // copy from there into the bitmap
        unique_ptr<void, decltype(&free)> temporary_buffer(malloc(total_size_of_decompressed_data), free);
        if (temporary_buffer == nullptr)
        {
            throw runtime_error("Failed to allocate temporary buffer for chunked decompression.");
        }

        if (get<1>(decode_information.chunk_header_info).hiLoBytePackingApplied)
        {
            CChunkedCompressionDecoder::DecompressLoHiBytePackingPreprocessing(decode_information, source_offset, temporary_buffer.get(), total_size_of_decompressed_data);
        }
        else
        {
            CChunkedCompressionDecoder::DecompressNoPreprocessing(decode_information, source_offset, temporary_buffer.get(), total_size_of_decompressed_data);
        }

        auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);
        memcpy(bitmap_lock_info.ptrDataRoi, temporary_buffer.get(), expected_size);

        return bitmap;
    }
}
#endif
