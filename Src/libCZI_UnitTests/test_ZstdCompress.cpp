// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "utils.h"
#include "../libCZI/decoder_zstd.h"

/**
 * \brief	This file contains tests of ZStd1 compression and decompression algorithms.
 */

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

class ZStd1DecodeParametersFixture : public testing::TestWithParam<const char*>
{
protected:
    const char* GetDecodeParameters() const
    {
        return GetParam();
    }
};

static void _testImageCompressDecompressZStd0Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType);
static void _testImageCompressDecompressZStd0Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters);

static void _testImageCompressDecompressZStd1Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType);
static void _testImageCompressDecompressZStd1Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters, const char* decode_parameters);

//!< ZStd0 Compress and decompress image, pass null pointer as a compression parameter
static void _testImageCompressDecompressZStd0Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType)
{
    _testImageCompressDecompressZStd0Param(imgWidth, imgHeight, pixelType, nullptr);
}

//!< ZStd1 Compress and decompress image, pass null pointer as a compression parameter
static void _testImageCompressDecompressZStd1Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType)
{
    _testImageCompressDecompressZStd1Param(imgWidth, imgHeight, pixelType, nullptr, nullptr);
}

//!< ZStd0 Compress and decompress image, pass a compression parameter.
//! If compression parameter is nullptr, it uses default parameters.
//! The function creates bitmap image with random pixels, which type is defined by 'pixelType' parameter.
//! At the moment only Gray8, Gray16, Brg24 and Brg48 are supported.
//! After decompression, the image buffer must have same data.
static void _testImageCompressDecompressZStd0Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters)
{
    const size_t maxSize = ZstdCompress::CalculateMaxCompressedSizeZStd0(imgWidth, imgHeight, pixelType);
    std::unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[maxSize]);
    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, imgWidth, imgHeight);

    size_t imgSize = maxSize;
    ScopedBitmapLockerSP lockRandom{ img };
    bool result = ZstdCompress::CompressZStd0(img->GetWidth()
        , img->GetHeight()
        , lockRandom.stride
        , img->GetPixelType()
        , lockRandom.ptrDataRoi
        , buffer.get()
        , imgSize
        , parameters);

    EXPECT_TRUE(result) << "Failed to compress bitmap image";
    EXPECT_TRUE(maxSize >= imgSize) << "Unexpected compress image size";

    std::shared_ptr<CZstd0Decoder> dec = CZstd0Decoder::Create();
    std::shared_ptr<libCZI::IBitmapData> decImg = dec->Decode(buffer.get(), imgSize, pixelType, img->GetWidth(), img->GetHeight());

    EXPECT_TRUE(decImg != nullptr) << "Failed to create decoded image";
    EXPECT_TRUE(decImg->GetHeight() == imgHeight) << "The decoded image has wrong height";
    EXPECT_TRUE(decImg->GetWidth() == imgWidth) << "The decoded image has wrong width";
    EXPECT_TRUE(decImg->GetPixelType() == pixelType) << "The decoded image has wrong pixel type";

    EXPECT_TRUE(AreBitmapDataEqual(img, decImg)) << "The bitmaps are not equal";
}

//!< ZStd1 Compress and decompress image, pass a compression parameter.
//! If compression parameter is nullptr, it uses default parameters.
//! The function creates bitmap image with random pixels, which type is defined by 'pixelType' parameter.
//! At the moment only Gray8, Gray16, Brg24 and Brg48 are supported.
//! After decompression, the image buffer must have same data.
static void _testImageCompressDecompressZStd1Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters, const char* decode_parameters)
{
    const size_t maxSize = ZstdCompress::CalculateMaxCompressedSizeZStd1(imgWidth, imgHeight, pixelType);
    std::unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[maxSize]);
    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, imgWidth, imgHeight);

    size_t imgSize = maxSize;
    ScopedBitmapLockerSP lockRandom{ img };
    bool result = ZstdCompress::CompressZStd1(img->GetWidth()
        , img->GetHeight()
        , lockRandom.stride
        , img->GetPixelType()
        , lockRandom.ptrDataRoi
        , buffer.get()
        , imgSize
        , parameters);

    EXPECT_TRUE(result) << "Failed to compress bitmap image";
    EXPECT_TRUE(maxSize >= imgSize) << "Unexpected compress image size";

    std::shared_ptr<CZstd1Decoder> dec = CZstd1Decoder::Create();
    std::shared_ptr<libCZI::IBitmapData> decImg = dec->Decode(buffer.get(), imgSize, pixelType, img->GetWidth(), img->GetHeight(), decode_parameters);

    EXPECT_TRUE(decImg != nullptr) << "Failed to create decoded image";
    EXPECT_TRUE(decImg->GetHeight() == imgHeight) << "The decoded image has wrong height";
    EXPECT_TRUE(decImg->GetWidth() == imgWidth) << "The decoded image has wrong width";
    EXPECT_TRUE(decImg->GetPixelType() == pixelType) << "The decoded image has wrong pixel type";

    EXPECT_TRUE(AreBitmapDataEqual(img, decImg)) << "The bitmaps are not equal";
}

//! Test ZStd0 compression and decompression for pixel type Gray8 
//! and use default compression parameters.
TEST(ZStdCompress, CompressZStd0Gray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;

    _testImageCompressDecompressZStd0Basic(64, 64, pixelType);
    _testImageCompressDecompressZStd0Basic(61, 61, pixelType);
}

//! Test ZStd1 compression and decompression for pixel type Gray8 
//! and use default compression parameters.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, nullptr, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, nullptr, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Gray16 
//! and use default compression parameters.
TEST(ZStdCompress, CompressZStd0Gray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;

    _testImageCompressDecompressZStd0Basic(64, 64, pixelType);
    _testImageCompressDecompressZStd0Basic(61, 61, pixelType);
}

//! Test ZStd1 compression and decompression for pixel type Gray16 
//! and use default compression parameters.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, nullptr, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, nullptr, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Brg24 
//! and use default compression parameters.
TEST(ZStdCompress, CompressZStd0Brg24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;

    _testImageCompressDecompressZStd0Basic(64, 64, pixelType);
    _testImageCompressDecompressZStd0Basic(61, 61, pixelType);
}

//! Test ZStd1 compression and decompression for pixel type Brg24 
//! and use default compression parameters.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, nullptr, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, nullptr, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Brg48 
//! and use default compression parameters.
TEST(ZStdCompress, CompressZStd0Brg48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;

    _testImageCompressDecompressZStd0Basic(64, 64, pixelType);
    _testImageCompressDecompressZStd0Basic(61, 61, pixelType);
}

//! Test ZStd1 compression and decompression for pixel type Brg48 
//! and use default compression parameters.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, nullptr, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, nullptr, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Gray8 
//! and use compression parameter "zstd0:ExplicitLevel=2"
TEST(ZStdCompress, CompressZStd0Gray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Gray8 
//! and use compression parameter "zstd1:ExplicitLevel=2"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Gray16
//! and use compression parameter "zstd0:ExplicitLevel=2"
TEST(ZStdCompress, CompressZStd0Gray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Gray16
//! and use compression parameter "zstd1:ExplicitLevel=2"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Brg24 
//! and use compression parameter "zstd0:ExplicitLevel=2"
TEST(ZStdCompress, CompressZStd0Brg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Brg24 
//! and use compression parameter "zstd1:ExplicitLevel=2"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Brg48
//! and use compression parameter "zstd0:ExplicitLevel=2"
TEST(ZStdCompress, CompressZStd0Brg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Brg48
//! and use compression parameter "zstd1:ExplicitLevel=2"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Gray8
//! and use compression parameter "zstd0:ExplicitLevel=2;PreProcess=HiLoByteUnpack".
//! The Low-high byte packing is ignored for ZStd0 compression.
TEST(ZStdCompress, CompressZStd0Gray8Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Gray8
//! and use compression parameter "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
//! The Low-high byte packing is ignored for the Gray8.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray8Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Gray16
//! and use compression parameter "zstd0:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
//! The Low-high byte packing is ignored for ZStd0 compression.
TEST(ZStdCompress, CompressZStd0Gray16Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Gray16
//! and use compression parameter "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Gray16Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Bgr24
//! and use compression parameter "zstd0:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
//! The Low-high byte packing is ignored for ZStd0 compression.
TEST(ZStdCompress, CompressZStd0Brg24Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Bgr24
//! and use compression parameter "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
//! The Low-high byte packing is ignored for the Bgr24.
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg24Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

//! Test ZStd0 compression and decompression for pixel type Bgr48
//! and use compression parameter "zstd0:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
//! The Low-high byte packing is ignored for ZStd0 compression.
TEST(ZStdCompress, CompressZStd0Brg48Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd0Param(64, 64, pixelType, &params);
    _testImageCompressDecompressZStd0Param(61, 61, pixelType, &params);
}

//! Test ZStd1 compression and decompression for pixel type Bgr48
//! and use compression parameter "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
TEST_P(ZStd1DecodeParametersFixture, CompressZStd1Brg48Level2LowByte)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t valLevel = 2;
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
    constexpr bool valLowPack = true;

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(valLevel);
    params.map[keyLowPack] = CompressParameter(valLowPack);

    _testImageCompressDecompressZStd1Param(64, 64, pixelType, &params, GetDecodeParameters());
    _testImageCompressDecompressZStd1Param(61, 61, pixelType, &params, GetDecodeParameters());
}

// run the tests with and without decode parameter "handle_data_size_mismatch", which is used to
// handle the data size mismatch between compressed data and expected data size in decoder, and 
// it should not affect the decompression result.
INSTANTIATE_TEST_SUITE_P(
    DecodeParameters,
    ZStd1DecodeParametersFixture,
    testing::Values(
        nullptr,
        "handle_data_size_mismatch"));

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, WalkCompressionHeaderScenario1)
{
    static uint8_t headerData[] =
    {
        0x01,                          // id
        0x05,                          // size of chunk data
        0x10, 0x20, 0x30, 0x40, 0x50,  // header-chunk data
        0x00                           // terminator
    };

    size_t bytes_consumed = 0;
    const bool b = ChunkedCompressionHeaderHelper::WalkCompressionHeader(
        headerData,
        sizeof(headerData),
        [](const ChunkedCompressionHeaderHelper::CompressionHeaderChunk& compression_header_chunk) -> bool
            {
                EXPECT_EQ(compression_header_chunk.chunkId, 0x01) << "Unexpected header chunk id";
                EXPECT_EQ(compression_header_chunk.chunkSize, 5) << "Unexpected header chunk data size";
                EXPECT_TRUE(memcmp(compression_header_chunk.chunkPayload, "\x10\x20\x30\x40\x50", 5) == 0) << "Unexpected header chunk data";
                return true;
            },
        &bytes_consumed);
    EXPECT_TRUE(b) << "WalkCompressionHeader failed";
    EXPECT_EQ(bytes_consumed, sizeof(headerData)) << "Unexpected number of bytes consumed in WalkCompressionHeader";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, WalkCompressionHeaderScenario2)
{
    static uint8_t header_data_no_terminator[] =
    {
        0x01,                          // id
        0x05,                          // size of chunk data
        0x10, 0x20, 0x30, 0x40, 0x50
    };

    // now, we pass one byte less than the full header data, and expect WalkCompressionHeader to fail due to missing terminator
    EXPECT_THROW(
        ChunkedCompressionHeaderHelper::WalkCompressionHeader(
        header_data_no_terminator,
        sizeof(header_data_no_terminator),
        [](const ChunkedCompressionHeaderHelper::CompressionHeaderChunk& compression_header_chunk) -> bool
        {
            EXPECT_EQ(compression_header_chunk.chunkId, 0x01) << "Unexpected header chunk id";
            EXPECT_EQ(compression_header_chunk.chunkSize, 5) << "Unexpected header chunk data size";
            EXPECT_TRUE(memcmp(compression_header_chunk.chunkPayload, "\x10\x20\x30\x40\x50", 5) == 0) << "Unexpected header chunk data";
            return true;
        },
        nullptr),
        exception) << "WalkCompressionHeader should have thrown due to missing terminator";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, WalkCompressionHeaderScenario3)
{
    static uint8_t headerData[] =
    {
        0x01,                          // id #1
        0x05,                          // size of chunk data #1
        0x10, 0x20, 0x30, 0x40, 0x50,  // header-chunk data #1
        0x03,                          // id #2
        0x02,                          // size of chunk data #2
        0xe0, 0xf0,                    // header-chunk data #2
        0x00                           // terminator
    };

    size_t bytes_consumed = 0;
    const bool b = ChunkedCompressionHeaderHelper::WalkCompressionHeader(
        headerData,
        sizeof(headerData),
        [](const ChunkedCompressionHeaderHelper::CompressionHeaderChunk& compression_header_chunk) -> bool
            {
                switch (compression_header_chunk.chunkId)
                {
                case 0x01:
                    EXPECT_EQ(compression_header_chunk.chunkSize, 5) << "Unexpected header chunk data size for chunk id 0x01";
                    EXPECT_TRUE(memcmp(compression_header_chunk.chunkPayload, "\x10\x20\x30\x40\x50", 5) == 0) << "Unexpected header chunk data for chunk id 0x01";
                    break;
                case 0x03:
                    EXPECT_EQ(compression_header_chunk.chunkSize, 2) << "Unexpected header chunk data size for chunk id 0x03";
                    EXPECT_TRUE(memcmp(compression_header_chunk.chunkPayload, "\xe0\xf0", 2) == 0) << "Unexpected header chunk data for chunk id 0x02";
                    break;
                default:
                    EXPECT_TRUE(false) << "Unexpected header chunk id";
                }

                return true;
            },
        &bytes_consumed);
    EXPECT_TRUE(b) << "WalkCompressionHeader failed";
    EXPECT_EQ(bytes_consumed, sizeof(headerData)) << "Unexpected number of bytes consumed in WalkCompressionHeader";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, GetCompressionHeaderSizeScenario1)
{
    static uint8_t headerData[] =
    {
        0x01,                          // id #1
        0x05,                          // size of chunk data #1
        0x10, 0x20, 0x30, 0x40, 0x50,  // header-chunk data #1
        0x03,                          // id #2
        0x02,                          // size of chunk data #2
        0xe0, 0xf0,                    // header-chunk data #2
        0x00                           // terminator
    };

    size_t header_size = ChunkedCompressionHeaderHelper::GetCompressionHeaderSize(headerData, sizeof(headerData));
    EXPECT_EQ(header_size, sizeof(headerData)) << "Unexpected compression header size";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, GetCompressionHeaderSizeScenario2)
{
    static uint8_t headerData[] =
    {
        0x01,                          // id #1
        0x05,                          // size of chunk data #1
        0x10, 0x20, 0x30, 0x40, 0x50,  // header-chunk data #1
        0x03,                          // id #2
        0x02,                          // size of chunk data #2
        0xe0, 0xf0                    // header-chunk data #2
    };

    EXPECT_THROW(
        ChunkedCompressionHeaderHelper::GetCompressionHeaderSize(headerData, sizeof(headerData)),
        exception) << "GetCompressionHeaderSize should have thrown due to missing terminator";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, GetCompressionHeaderSizeScenario3)
{
    static uint8_t headerData[] =
    {
        0x01,                          // id
        0x05,                          // size of chunk data
        0x10, 0x20, 0x30, 0x40         // only 4 bytes of header-chunk data #1 (instead of 5)
    };

    EXPECT_THROW(
        ChunkedCompressionHeaderHelper::GetCompressionHeaderSize(headerData, sizeof(headerData)),
        exception) << "GetCompressionHeaderSize should have thrown due to incomplete header chunk data";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, ParseCompressionHeaderScenario1)
{
    static uint8_t headerData[] =
    {
        0x01, // id -> CompressedChunkSizes
        0x03,
        0x01, 0x02, 0x03,
        0x03, // id -> DecompressedChunkSizes
        0x01,
        0x08,
        0x00
    };

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(headerData, sizeof(headerData));
    EXPECT_EQ(get<0>(size_and_header_info), sizeof(headerData));
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks.size(), 3);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 1);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 2);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 3);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 8);

    // since the header does not contain a chunk with id = 0x02, the codec should be set to ZStd by default
    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::ZStd);
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, ParseCompressionHeaderScenario2)
{
    static uint8_t headerData[] =
    {
        0x01, // id -> CompressedChunkSizes
        0x03,
        0x01, 0x02, 0x03,
        0x03, // id -> DecompressedChunkSizes
        0x02,
        0x08, 0x09,
        0x00
    };

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(headerData, sizeof(headerData));
    EXPECT_EQ(get<0>(size_and_header_info), sizeof(headerData));
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks.size(), 3);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 1);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 2);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 3);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 9);

    // since the header does not contain a chunk with id = 0x02, the codec should be set to ZStd by default
    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::ZStd);
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, ParseCompressionHeaderScenario3)
{
    static uint8_t headerData[] =
    {
        0x01, // id -> CompressedChunkSizes
        0x03,
        0x01, 0x02, 0x03,
        0x03, // id -> DecompressedChunkSizes
        0x03,
        0x08, 0x09, 0x0a,
        0x00
    };

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(headerData, sizeof(headerData));
    EXPECT_EQ(get<0>(size_and_header_info), sizeof(headerData));
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks.size(), 3);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 1);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 2);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 9);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 3);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 10);

    // since the header does not contain a chunk with id = 0x02, the codec should be set to ZStd by default
    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::ZStd);
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, ParseCompressionHeaderScenario4)
{
    static uint8_t headerData[] =
    {
        0x01, // id -> CompressedChunkSizes
        0x03,
        0x01, 0x02, 0x03,
        0x03, // id -> DecompressedChunkSizes
        0x04,
        0x08, 0x09, 0x0a, 0x0b,
        0x00
    };

    EXPECT_THROW(
        ChunkedCompressionHeaderHelper::ParseCompressionHeader(headerData, sizeof(headerData)),
        exception) << "ParseCompressionHeader should have thrown due to mismatch in number of compressed and decompressed chunk sizes";
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, ParseCompressionHeaderScenario5)
{
    static uint8_t headerData[] =
    {
        0x01, // id -> CompressedChunkSizes
        0x03,
        0x01, 0x02, 0x03,
        0x03, // id -> DecompressedChunkSizes
        0x02,
        0x08, 0x09,
        0x02, // id -> Codec
        0x01,
        0x01, // codec value -> LZ4
        0x00
    };

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(headerData, sizeof(headerData));
    EXPECT_EQ(get<0>(size_and_header_info), sizeof(headerData));
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks.size(), 3);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 1);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 2);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 8);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 3);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 9);

    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::Lz4);
    EXPECT_EQ(header_info.hiLoBytePackingApplied, false);
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, CreateCompressionHeaderScenario1)
{
    ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
    header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
    header_info_for_creation.hiLoBytePackingApplied = 0xff;
    header_info_for_creation.chunkSizes = { 4,5,6,7 };
    header_info_for_creation.uncompressedSizes = { 10,11 };

    uint8_t header_buffer[256] = { 0 };
    size_t size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(header_buffer, sizeof(header_buffer), header_info_for_creation);

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(header_buffer, sizeof(header_buffer));
    EXPECT_EQ(size, get<0>(size_and_header_info)) << "Size returned by CreateCompressionHeader does not match actual header size";

    EXPECT_EQ(get<1>(size_and_header_info).chunks.size(), 4) << "Unexpected number of chunks in parsed header info";
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 4);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 5);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 6);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[3].compressedSize, 7);
    EXPECT_EQ(header_info.chunks[3].uncompressedSize, 11);
    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::ZStd);
    EXPECT_EQ(header_info.hiLoBytePackingApplied, false);
}
#endif

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
TEST(ZStdCompress, CreateCompressionHeaderScenario2)
{
    ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
    header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
    header_info_for_creation.hiLoBytePackingApplied = 0x01;
    header_info_for_creation.chunkSizes = { 4,5,6,7 };
    header_info_for_creation.uncompressedSizes = { 10,11 };

    uint8_t header_buffer[256] = { 0 };
    size_t size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(header_buffer, sizeof(header_buffer), header_info_for_creation);

    const auto size_and_header_info = ChunkedCompressionHeaderHelper::ParseCompressionHeader(header_buffer, sizeof(header_buffer));
    EXPECT_EQ(size, get<0>(size_and_header_info)) << "Size returned by CreateCompressionHeader does not match actual header size";

    EXPECT_EQ(get<1>(size_and_header_info).chunks.size(), 4) << "Unexpected number of chunks in parsed header info";
    const auto& header_info = get<1>(size_and_header_info);
    EXPECT_EQ(header_info.chunks[0].compressedSize, 4);
    EXPECT_EQ(header_info.chunks[0].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[1].compressedSize, 5);
    EXPECT_EQ(header_info.chunks[1].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[2].compressedSize, 6);
    EXPECT_EQ(header_info.chunks[2].uncompressedSize, 10);
    EXPECT_EQ(header_info.chunks[3].compressedSize, 7);
    EXPECT_EQ(header_info.chunks[3].uncompressedSize, 11);
    EXPECT_EQ(header_info.codec, ChunkedCompressionHeaderHelper::Codec::ZStd);
    EXPECT_EQ(header_info.hiLoBytePackingApplied, true);
}
#endif
