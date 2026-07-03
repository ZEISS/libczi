// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"

#include <array>
#include <memory>
#include <random>
#include <vector>

using namespace libCZI;
using namespace std;

TEST(ChunkedCompression, DecodeTestScenario1)
{
    // we construct a simple chunked-compressed data block manually here, and then decode it with the chunked-compression decoder. 

    static uint8_t data_chunk_1[] = { 1,2,3,4,5 };
    static uint8_t data_chunk_2[] = { 6,7,8,9,10 };
    static uint8_t data_chunk_3[] = { 11,12,13,14,15 };
    static uint8_t data_chunk_4[] = { 16,17,18 };

    static_assert(sizeof(data_chunk_1) == sizeof(data_chunk_2) && sizeof(data_chunk_2) == sizeof(data_chunk_3), "data chunks 1, 2 and 3 must have the same size for this test");

    const auto compressed_chunk_1 = ZstdCompress::CompressZStd0Alloc(sizeof(data_chunk_1), 1, sizeof(data_chunk_1), PixelType::Gray8, data_chunk_1, nullptr);
    const auto compressed_chunk_2 = ZstdCompress::CompressZStd0Alloc(sizeof(data_chunk_2), 1, sizeof(data_chunk_2), PixelType::Gray8, data_chunk_2, nullptr);
    const auto compressed_chunk_3 = ZstdCompress::CompressZStd0Alloc(sizeof(data_chunk_3), 1, sizeof(data_chunk_3), PixelType::Gray8, data_chunk_3, nullptr);
    const auto compressed_chunk_4 = ZstdCompress::CompressZStd0Alloc(sizeof(data_chunk_4), 1, sizeof(data_chunk_4), PixelType::Gray8, data_chunk_4, nullptr);

    ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
    header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
    header_info_for_creation.hiLoBytePackingApplied = 0xff;
    header_info_for_creation.chunkSizes =
    {
        static_cast<uint32_t>(compressed_chunk_1->GetSizeOfData()),
        static_cast<uint32_t>(compressed_chunk_2->GetSizeOfData()),
        static_cast<uint32_t>(compressed_chunk_3->GetSizeOfData()),
        static_cast<uint32_t>(compressed_chunk_4->GetSizeOfData())
    };
    header_info_for_creation.uncompressedSizes = { static_cast<uint32_t>(sizeof(data_chunk_1)), static_cast<uint32_t>(sizeof(data_chunk_4)) };

    const size_t max_header_size = ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(header_info_for_creation);

    const auto sub_block_data = std::make_unique<uint8_t[]>(max_header_size + compressed_chunk_1->GetSizeOfData() + compressed_chunk_2->GetSizeOfData() + compressed_chunk_3->GetSizeOfData() + compressed_chunk_4->GetSizeOfData());

    size_t actual_header_size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(sub_block_data.get(), max_header_size, header_info_for_creation);
    ASSERT_LE(actual_header_size, max_header_size) << "Actual header size exceeds the maximum header size";

    memcpy(sub_block_data.get() + actual_header_size, compressed_chunk_1->GetPtr(), compressed_chunk_1->GetSizeOfData());
    memcpy(sub_block_data.get() + actual_header_size + compressed_chunk_1->GetSizeOfData(), compressed_chunk_2->GetPtr(), compressed_chunk_2->GetSizeOfData());
    memcpy(sub_block_data.get() + actual_header_size + compressed_chunk_1->GetSizeOfData() + compressed_chunk_2->GetSizeOfData(), compressed_chunk_3->GetPtr(), compressed_chunk_3->GetSizeOfData());
    memcpy(sub_block_data.get() + actual_header_size + compressed_chunk_1->GetSizeOfData() + compressed_chunk_2->GetSizeOfData() + compressed_chunk_3->GetSizeOfData(), compressed_chunk_4->GetPtr(), compressed_chunk_4->GetSizeOfData());

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    sub_block_data.get(),
                                    actual_header_size + compressed_chunk_1->GetSizeOfData() + compressed_chunk_2->GetSizeOfData() + compressed_chunk_3->GetSizeOfData() + compressed_chunk_4->GetSizeOfData(),
                                    PixelType::Gray8,
                                    sizeof(data_chunk_1) + sizeof(data_chunk_2) + sizeof(data_chunk_3) + sizeof(data_chunk_4),
                                    1,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(decoded_bitmap->GetWidth(), sizeof(data_chunk_1) + sizeof(data_chunk_2) + sizeof(data_chunk_3) + sizeof(data_chunk_4));
    ASSERT_EQ(decoded_bitmap->GetHeight(), 1);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    ASSERT_EQ(memcmp(bitmap_lock_info.ptrDataRoi, data_chunk_1, sizeof(data_chunk_1)), 0) << "Decoded data chunk 1 does not match original data";
    ASSERT_EQ(memcmp(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + sizeof(data_chunk_1), data_chunk_2, sizeof(data_chunk_2)), 0) << "Decoded data chunk 2 does not match original data";
    ASSERT_EQ(memcmp(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + sizeof(data_chunk_1) + sizeof(data_chunk_2), data_chunk_3, sizeof(data_chunk_3)), 0) << "Decoded data chunk 3 does not match original data";
    ASSERT_EQ(memcmp(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + sizeof(data_chunk_1) + sizeof(data_chunk_2) + sizeof(data_chunk_3), data_chunk_4, sizeof(data_chunk_4)), 0) << "Decoded data chunk 4 does not match original data";
}

TEST(ChunkedCompression, EncodeAndDecodeSmallGray8Bitmap)
{
    // we compress a small Gray8 bitmap with the chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;    // this is more than enough for the small bitmap we are compressing here
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 4> source_data = { 1,2,3,4 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2, PixelType::Gray8, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    compressed_data_buffer.get(),
                                    compressed_data_size,
                                    PixelType::Gray8,
                                    2,
                                    2,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(decoded_bitmap->GetWidth(), 2);
    ASSERT_EQ(decoded_bitmap->GetHeight(), 2);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    ASSERT_EQ(*static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi), 1) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + 1), 2) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride), 3) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride + 1), 4) << "Decoded data does not match original data";
}

TEST(ChunkedCompression, EncodeAndDecodeSmallGray16Bitmap)
{
    // we compress a small Gray16 bitmap with the chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    compressed_data_buffer.get(),
                                    compressed_data_size,
                                    PixelType::Gray16,
                                    2,
                                    2,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray16);
    ASSERT_EQ(decoded_bitmap->GetWidth(), 2);
    ASSERT_EQ(decoded_bitmap->GetHeight(), 2);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    const auto* first_row = static_cast<const uint16_t*>(bitmap_lock_info.ptrDataRoi);
    const auto* second_row = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride);
    ASSERT_EQ(first_row[0], 1) << "Decoded data does not match original data";
    ASSERT_EQ(first_row[1], 2) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[0], 3) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[1], 4) << "Decoded data does not match original data";
}

TEST(ChunkedCompression, EncodeAndDecodeSmallGray16BitmapWithLoHiByteUnpacking)
{
    // we compress a small Gray16 bitmap with the chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    compressed_data_buffer.get(),
                                    compressed_data_size,
                                    PixelType::Gray16,
                                    2,
                                    2,
                                    "IgnorePreprocessingInstruction");  // instruct the decoder to ignore the hi-lo-byte-packing preprocessing instruction 
    // (which is expected to be present in the compressed data, since we enabled hi-lo-byte-unpacking in the encoder) - 
    // this should lead to the decoded data being still correct, but with the hi and lo bytes not being unpacked (i.e. 
    // the values in the decoded bitmap are expected to be different from the original source data, since they are 
    // expected to still be unpacked)
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray16);
    ASSERT_EQ(decoded_bitmap->GetWidth(), 2);
    ASSERT_EQ(decoded_bitmap->GetHeight(), 2);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    const uint8_t* first_row = static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi);
    const uint8_t* second_row = static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride;
    ASSERT_EQ(first_row[0], 1) << "Decoded data does not match original data";
    ASSERT_EQ(first_row[1], 2) << "Decoded data does not match original data";
    ASSERT_EQ(first_row[2], 3) << "Decoded data does not match original data";
    ASSERT_EQ(first_row[3], 4) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[0], 0) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[1], 0) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[2], 0) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[3], 0) << "Decoded data does not match original data";
}

TEST(ChunkedCompression, EncodeAndDecodeGray16BitmapWithLoHiByteUnpackingAndMultipleChunks)
{
    // This test verifies that hi-lo byte preprocessing round-trips correctly when the
    // bitmap is split into multiple chunks. The encoder applies LoHiByteUnpackStrided
    // per-chunk, and the decoder inverts this per-chunk, so each compressed chunk is
    // a self-contained independently hi-lo-packed block.

    constexpr uint32_t kWidth = 8;
    constexpr uint32_t kHeight = 4;
    constexpr uint32_t kStride = kWidth * sizeof(uint16_t);
    constexpr uint32_t kMaxChunkSize = 16; // source payload is 64 bytes, forcing 4 chunks
    constexpr size_t kDestinationBufferSize = 64 * 1024;

    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);

    array<uint16_t, kWidth * kHeight> source_data;
    for (size_t i = 0; i < source_data.size(); ++i)
    {
        // Use values with both low and high bytes populated, so incorrect low/high pairing
        // is easy to detect.
        source_data[i] = static_cast<uint16_t>(0x1200 + i * 37);
    }

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_MAXCHUNKSIZE)] = CompressParameter(kMaxChunkSize);

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(
        kWidth,
        kHeight,
        kStride,
        PixelType::Gray16,
        source_data.data(),
        compressed_data_buffer.get(),
        compressed_data_size,
        &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
        compressed_data_buffer.get(),
        compressed_data_size,
        PixelType::Gray16,
        kWidth,
        kHeight,
        nullptr);

    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray16);
    ASSERT_EQ(decoded_bitmap->GetWidth(), kWidth);
    ASSERT_EQ(decoded_bitmap->GetHeight(), kHeight);

    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    for (uint32_t y = 0; y < kHeight; ++y)
    {
        const auto* decoded_row = reinterpret_cast<const uint16_t*>(
            static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + static_cast<size_t>(y) * bitmap_lock_info.stride);
        const auto* source_row = source_data.data() + static_cast<size_t>(y) * kWidth;

        ASSERT_EQ(memcmp(decoded_row, source_row, kStride), 0) << "Decoded row " << y << " does not match original data";
    }
}

TEST(ChunkedCompression, EncodeAndDecodeGray16BitmapWithLoHiByteUnpackingAndMultipleChunks_Lz4)
{
    // This test verifies that hi-lo byte preprocessing round-trips correctly when the
    // bitmap is split into multiple chunks. The encoder applies LoHiByteUnpackStrided
    // per-chunk, and the decoder inverts this per-chunk, so each compressed chunk is
    // a self-contained independently hi-lo-packed block.

    constexpr uint32_t kWidth = 8;
    constexpr uint32_t kHeight = 4;
    constexpr uint32_t kStride = kWidth * sizeof(uint16_t);
    constexpr uint32_t kMaxChunkSize = 16; // source payload is 64 bytes, forcing 4 chunks
    constexpr size_t kDestinationBufferSize = 64 * 1024;

    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);

    array<uint16_t, kWidth* kHeight> source_data;
    for (size_t i = 0; i < source_data.size(); ++i)
    {
        // Use values with both low and high bytes populated, so incorrect low/high pairing
        // is easy to detect.
        source_data[i] = static_cast<uint16_t>(0x1200 + i * 37);
    }

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_MAXCHUNKSIZE)] = CompressParameter(kMaxChunkSize);
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_CODEC)] = CompressParameter(static_cast<int>(ChunkedCompressionHeaderHelper::Codec::Lz4));

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(
        kWidth,
        kHeight,
        kStride,
        PixelType::Gray16,
        source_data.data(),
        compressed_data_buffer.get(),
        compressed_data_size,
        &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
        compressed_data_buffer.get(),
        compressed_data_size,
        PixelType::Gray16,
        kWidth,
        kHeight,
        nullptr);

    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray16);
    ASSERT_EQ(decoded_bitmap->GetWidth(), kWidth);
    ASSERT_EQ(decoded_bitmap->GetHeight(), kHeight);

    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    for (uint32_t y = 0; y < kHeight; ++y)
    {
        const auto* decoded_row = reinterpret_cast<const uint16_t*>(
            static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + static_cast<size_t>(y) * bitmap_lock_info.stride);
        const auto* source_row = source_data.data() + static_cast<size_t>(y) * kWidth;

        ASSERT_EQ(memcmp(decoded_row, source_row, kStride), 0) << "Decoded row " << y << " does not match original data";
    }
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray8Bitmap)
{
    // we compress a small Gray8 bitmap with the chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;    // this is more than enough for the small bitmap we are compressing here
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 4> source_data = { 1,2,3,4 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2, PixelType::Gray8, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2, PixelType::Gray8, source_data.data(), nullptr);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray16Bitmap)
{
    // we compress a small Gray16 bitmap with the chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), nullptr);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray16BitmapWithHiLoBytePacking)
{
    // we compress a small Gray16 bitmap with the chunked-compression encoder (where we enable hi-lo-byte-packing)
    // with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), &parameters);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray16BitmapWithHiLoBytePacking_Lz4)
{
    // we compress a small Gray16 bitmap with the chunked-compression encoder (where we enable hi-lo-byte-packing)
    // with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_CODEC)] = CompressParameter(static_cast<int>(ChunkedCompressionHeaderHelper::Codec::Lz4));

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), &parameters);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallBgr24Bitmap)
{
    // we compress a small Bgr24 bitmap with the chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 12> source_data = { 1,2,3,4,5,6,7,8,9,10,11,12 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * 3, PixelType::Bgr24, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * 3, PixelType::Bgr24, source_data.data(), nullptr);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallBgr48Bitmap)
{
    // we compress a small Bgr48 bitmap with the chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 12> source_data = { 1,2,3,4,5,6,7,8,9,10,11,12 };

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), compressed_data_buffer.get(), compressed_data_size, nullptr);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), nullptr);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallBgr48BitmapWithHiLoBytePacking)
{
    // we compress a small Bgr48 bitmap with the chunked-compression encoder (where we enable hi-lo-byte-packing)
    // with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 12> source_data = { 1,2,3,4,5,6,7,8,9,10,11,12 };

    CompressParametersOnMap parameters;
    parameters.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING)] = CompressParameter(true);

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &parameters);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), &parameters);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}


struct ChunkedCompressionRoundTripParams
{
    uint32_t maxChunkSize;
};

struct ChunkedCompressionRoundTripFixture : public testing::TestWithParam<ChunkedCompressionRoundTripParams> {};

TEST_P(ChunkedCompressionRoundTripFixture, CompressToMemoryBlockRoundTripsRandomGray8Bitmap)
{
    // we compress a Gray8 bitmap filled with deterministic random data using CompressToMemoryBlock, then decode it again and verify the roundtrip.

    constexpr uint32_t kWidth = 1000;
    constexpr uint32_t kHeight = 1000;
    constexpr uint32_t kStride = kWidth + 13;  // stride intentionally larger than the line size

    vector<uint8_t> source_data(static_cast<size_t>(kStride) * kHeight);
    mt19937 rng(12345);
    uniform_int_distribution<int> distribution(0, 255);
    for (auto& value : source_data)
    {
        value = static_cast<uint8_t>(distribution(rng));
    }

    CompressParametersOnMap compress_params;
    compress_params.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_MAXCHUNKSIZE)] = CompressParameter(GetParam().maxChunkSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(kWidth, kHeight, kStride, PixelType::Gray8, source_data.data(), &compress_params);
    ASSERT_NE(mem_blk, nullptr);
    ASSERT_GT(mem_blk->GetSizeOfData(), 0U);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    mem_blk->GetPtr(),
                                    mem_blk->GetSizeOfData(),
                                    PixelType::Gray8,
                                    kWidth,
                                    kHeight,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(decoded_bitmap->GetWidth(), kWidth);
    ASSERT_EQ(decoded_bitmap->GetHeight(), kHeight);

    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    for (uint32_t y = 0; y < kHeight; ++y)
    {
        const auto* decoded_row = static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + static_cast<size_t>(y) * bitmap_lock_info.stride;
        const auto* source_row = source_data.data() + static_cast<size_t>(y) * kStride;
        ASSERT_EQ(memcmp(decoded_row, source_row, kWidth), 0) << "Decoded row " << y << " does not match original data";
    }
}

INSTANTIATE_TEST_SUITE_P(
    ChunkedCompression,
    ChunkedCompressionRoundTripFixture,
    testing::Values(
    ChunkedCompressionRoundTripParams{ 65536 },
    ChunkedCompressionRoundTripParams{ 32768 },
    ChunkedCompressionRoundTripParams{ 14879 },
    ChunkedCompressionRoundTripParams{ 89999 }
));

// --- LZ4 tests ---

static CompressParametersOnMap MakeLz4CompressParams()
{
    CompressParametersOnMap params;
    params.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_CODEC)] =
        CompressParameter(static_cast<int32_t>(ChunkedCompressionHeaderHelper::Codec::Lz4));
    return params;
}

TEST(ChunkedCompression, EncodeAndDecodeSmallGray8Bitmap_Lz4)
{
    // we compress a small Gray8 bitmap with the LZ4 chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 4> source_data = { 1,2,3,4 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2, PixelType::Gray8, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    compressed_data_buffer.get(),
                                    compressed_data_size,
                                    PixelType::Gray8,
                                    2,
                                    2,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(decoded_bitmap->GetWidth(), 2);
    ASSERT_EQ(decoded_bitmap->GetHeight(), 2);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    ASSERT_EQ(*static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi), 1) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + 1), 2) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride), 3) << "Decoded data does not match original data";
    ASSERT_EQ(*(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride + 1), 4) << "Decoded data does not match original data";
}

TEST(ChunkedCompression, EncodeAndDecodeSmallGray16Bitmap_Lz4)
{
    // we compress a small Gray16 bitmap with the LZ4 chunked-compression encoder and then decode it again to verify the roundtrip.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    compressed_data_buffer.get(),
                                    compressed_data_size,
                                    PixelType::Gray16,
                                    2,
                                    2,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray16);
    ASSERT_EQ(decoded_bitmap->GetWidth(), 2);
    ASSERT_EQ(decoded_bitmap->GetHeight(), 2);
    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    const auto* first_row = static_cast<const uint16_t*>(bitmap_lock_info.ptrDataRoi);
    const auto* second_row = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + bitmap_lock_info.stride);
    ASSERT_EQ(first_row[0], 1) << "Decoded data does not match original data";
    ASSERT_EQ(first_row[1], 2) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[0], 3) << "Decoded data does not match original data";
    ASSERT_EQ(second_row[1], 4) << "Decoded data does not match original data";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray8Bitmap_Lz4)
{
    // we compress a small Gray8 bitmap with the LZ4 chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 4> source_data = { 1,2,3,4 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2, PixelType::Gray8, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2, PixelType::Gray8, source_data.data(), &compress_params);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallGray16Bitmap_Lz4)
{
    // we compress a small Gray16 bitmap with the LZ4 chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 4> source_data = { 1,2,3,4 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * sizeof(uint16_t), PixelType::Gray16, source_data.data(), &compress_params);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallBgr24Bitmap_Lz4)
{
    // we compress a small Bgr24 bitmap with the LZ4 chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint8_t, 12> source_data = { 1,2,3,4,5,6,7,8,9,10,11,12 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * 3, PixelType::Bgr24, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * 3, PixelType::Bgr24, source_data.data(), &compress_params);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

TEST(ChunkedCompression, CompressToMemoryBlockMatchesCompressForSmallBgr48Bitmap_Lz4)
{
    // we compress a small Bgr48 bitmap with the LZ4 chunked-compression encoder with two different APIs and verify that the compressed output matches.

    constexpr size_t kDestinationBufferSize = 10 * 1024;
    unique_ptr<uint8_t[]> compressed_data_buffer = make_unique<uint8_t[]>(kDestinationBufferSize);
    static constexpr array<uint16_t, 12> source_data = { 1,2,3,4,5,6,7,8,9,10,11,12 };
    const auto compress_params = MakeLz4CompressParams();

    size_t compressed_data_size = kDestinationBufferSize;
    const bool success = ChunkedCompress::Compress(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), compressed_data_buffer.get(), compressed_data_size, &compress_params);

    ASSERT_TRUE(success);
    ASSERT_GT(compressed_data_size, 0);
    ASSERT_LE(compressed_data_size, kDestinationBufferSize);

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(2, 2, 2 * 3 * sizeof(uint16_t), PixelType::Bgr48, source_data.data(), &compress_params);
    ASSERT_EQ(compressed_data_size, mem_blk->GetSizeOfData()) << "Size of compressed data from CompressToMemoryBlock does not match size from Compress";
    ASSERT_EQ(memcmp(compressed_data_buffer.get(), mem_blk->GetPtr(), compressed_data_size), 0) << "Compressed data from CompressToMemoryBlock does not match data from Compress";
}

struct ChunkedCompressionRoundTripLz4Fixture : public testing::TestWithParam<ChunkedCompressionRoundTripParams> {};

TEST_P(ChunkedCompressionRoundTripLz4Fixture, CompressToMemoryBlockRoundTripsRandomGray8Bitmap)
{
    // we compress a Gray8 bitmap filled with deterministic random data using LZ4 via CompressToMemoryBlock, then decode it again and verify the roundtrip.

    constexpr uint32_t kWidth = 1000;
    constexpr uint32_t kHeight = 1000;
    constexpr uint32_t kStride = kWidth + 13;  // stride intentionally larger than the line size

    vector<uint8_t> source_data(static_cast<size_t>(kStride) * kHeight);
    mt19937 rng(12345);
    uniform_int_distribution<int> distribution(0, 255);
    for (auto& value : source_data)
    {
        value = static_cast<uint8_t>(distribution(rng));
    }

    CompressParametersOnMap compress_params;
    compress_params.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_MAXCHUNKSIZE)] = CompressParameter(GetParam().maxChunkSize);
    compress_params.map[static_cast<int>(CompressionParameterKey::CHUNKEDCOMPRESSION_CODEC)] =
        CompressParameter(static_cast<int32_t>(ChunkedCompressionHeaderHelper::Codec::Lz4));

    auto mem_blk = ChunkedCompress::CompressToMemoryBlock(kWidth, kHeight, kStride, PixelType::Gray8, source_data.data(), &compress_params);
    ASSERT_NE(mem_blk, nullptr);
    ASSERT_GT(mem_blk->GetSizeOfData(), 0U);

    const auto decoder = libCZI::GetDefaultSiteObject(SiteObjectType::Default)->GetDecoder(ImageDecoderType::ChunkedCompression, nullptr);
    const auto decoded_bitmap = decoder->Decode(
                                    mem_blk->GetPtr(),
                                    mem_blk->GetSizeOfData(),
                                    PixelType::Gray8,
                                    kWidth,
                                    kHeight,
                                    nullptr);
    ASSERT_EQ(decoded_bitmap->GetPixelType(), PixelType::Gray8);
    ASSERT_EQ(decoded_bitmap->GetWidth(), kWidth);
    ASSERT_EQ(decoded_bitmap->GetHeight(), kHeight);

    const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(decoded_bitmap);
    for (uint32_t y = 0; y < kHeight; ++y)
    {
        const auto* decoded_row = static_cast<const uint8_t*>(bitmap_lock_info.ptrDataRoi) + static_cast<size_t>(y) * bitmap_lock_info.stride;
        const auto* source_row = source_data.data() + static_cast<size_t>(y) * kStride;
        ASSERT_EQ(memcmp(decoded_row, source_row, kWidth), 0) << "Decoded row " << y << " does not match original data";
    }
}

INSTANTIATE_TEST_SUITE_P(
    ChunkedCompression,
    ChunkedCompressionRoundTripLz4Fixture,
    testing::Values(
    ChunkedCompressionRoundTripParams{ 65536 },
    ChunkedCompressionRoundTripParams{ 32768 },
    ChunkedCompressionRoundTripParams{ 14879 },
    ChunkedCompressionRoundTripParams{ 89999 }
));
