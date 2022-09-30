// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"
#include "utils.h"
#include "../libCZI/decoder_zstd.h"

/**
 * \brief	This file contains tests of ZStd1 compression and decompression algorithms.
 */

using namespace libCZI;
using namespace std;

static void _testImageCompressDecompressZStd0Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType);
static void _testImageCompressDecompressZStd0Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters);

static void _testImageCompressDecompressZStd1Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType);
static void _testImageCompressDecompressZStd1Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters);

//!< ZStd0 Compress and decompress image, pass null pointer as a compression parameter
static void _testImageCompressDecompressZStd0Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType)
{
	_testImageCompressDecompressZStd0Param(imgWidth, imgHeight, pixelType, nullptr);
}

//!< ZStd1 Compress and decompress image, pass null pointer as a compression parameter
static void _testImageCompressDecompressZStd1Basic(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType)
{
	_testImageCompressDecompressZStd1Param(imgWidth, imgHeight, pixelType, nullptr);
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
static void _testImageCompressDecompressZStd1Param(uint32_t imgWidth, uint32_t imgHeight, PixelType pixelType, const ICompressParameters* parameters)
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
	std::shared_ptr<libCZI::IBitmapData> decImg = dec->Decode(buffer.get(), imgSize, pixelType, img->GetWidth(), img->GetHeight());

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
TEST(ZStdCompress, CompressZStd1Gray8Basic)
{
	constexpr PixelType pixelType = PixelType::Gray8;

	_testImageCompressDecompressZStd1Basic(64, 64, pixelType);
	_testImageCompressDecompressZStd1Basic(61, 61, pixelType);
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
TEST(ZStdCompress, CompressZStd1Gray16Basic)
{
	constexpr PixelType pixelType = PixelType::Gray16;

	_testImageCompressDecompressZStd1Basic(64, 64, pixelType);
	_testImageCompressDecompressZStd1Basic(61, 61, pixelType);
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
TEST(ZStdCompress, CompressZStd1Brg24Basic)
{
	constexpr PixelType pixelType = PixelType::Bgr24;

	_testImageCompressDecompressZStd1Basic(64, 64, pixelType);
	_testImageCompressDecompressZStd1Basic(61, 61, pixelType);
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
TEST(ZStdCompress, CompressZStd1Brg48Basic)
{
	constexpr PixelType pixelType = PixelType::Bgr48;

	_testImageCompressDecompressZStd1Basic(64, 64, pixelType);
	_testImageCompressDecompressZStd1Basic(61, 61, pixelType);
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
TEST(ZStdCompress, CompressZStd1Gray8Level2)
{
	constexpr PixelType pixelType = PixelType::Gray8;
	constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Gray16Level2)
{
	constexpr PixelType pixelType = PixelType::Gray16;
	constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Brg24Level2)
{
	constexpr PixelType pixelType = PixelType::Bgr24;
	constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Brg48Level2)
{
	constexpr PixelType pixelType = PixelType::Bgr48;
	constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Gray8Level2LowByte)
{
	constexpr PixelType pixelType = PixelType::Gray8;
	constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;
	constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
	constexpr bool valLowPack = true;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);
	params.map[keyLowPack] = CompressParameter(valLowPack);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Gray16Level2LowByte)
{
	constexpr PixelType pixelType = PixelType::Gray16;
	constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;
	constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
	constexpr bool valLowPack = true;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);
	params.map[keyLowPack] = CompressParameter(valLowPack);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Brg24Level2LowByte)
{
	constexpr PixelType pixelType = PixelType::Bgr24;
	constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;
	constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
	constexpr bool valLowPack = true;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);
	params.map[keyLowPack] = CompressParameter(valLowPack);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
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
TEST(ZStdCompress, CompressZStd1Brg48Level2LowByte)
{
	constexpr PixelType pixelType = PixelType::Bgr48;
	constexpr int32_t keyLevel = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
	constexpr int32_t valLevel = 2;
	constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);
	constexpr bool valLowPack = true;

	libCZI::CompressParametersOnMap params;
	params.map[keyLevel] = CompressParameter(valLevel);
	params.map[keyLowPack] = CompressParameter(valLowPack);

	_testImageCompressDecompressZStd1Param(64, 64, pixelType, &params);
	_testImageCompressDecompressZStd1Param(61, 61, pixelType, &params);
}
