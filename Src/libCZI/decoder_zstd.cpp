// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "decoder_zstd.h"
#include <zstd.h>
#if (ZSTD_VERSION_MAJOR >= 1 && ZSTD_VERSION_MINOR >= 5) 
 #include <zstd_errors.h>
#else
 #include <common/zstd_errors.h>
#endif
#include "bitmapData.h"
#include "libCZI_Utilities.h"
#include "utilities.h"

using namespace std;
using namespace libCZI;

struct ZStd1HeaderParsingResult
{
		/// Size of the header in bytes. If this is zero, the header did not parse correctly.
		size_t headerSize;

        /// A boolean indicating whether a hi-lo-byte packing is to be done.
		bool hiLoByteUnpackPreprocessing;
};

static ZStd1HeaderParsingResult ParseZStd1Header(const uint8_t* ptrData, size_t size);
static shared_ptr<libCZI::IBitmapData> DecodeAndProcess(const void* pData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height, bool doHiLoByteUnpacking);
static shared_ptr<libCZI::IBitmapData> DecodeAndProcessNoHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height);
static shared_ptr<libCZI::IBitmapData> DecodeAndProcessWithHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height);
static void ZstdDecompressAndThrowIfError(const void* ptrData, size_t size, void* ptrDst, size_t dstSize);

/*static*/std::shared_ptr<CZstd0Decoder> CZstd0Decoder::Create()
{
		return make_shared<CZstd0Decoder>();
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CZstd0Decoder::Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
		return DecodeAndProcessNoHiLoByteUnpacking(ptrData, size, pixelType, width, height);
}

/*static*/std::shared_ptr<CZstd1Decoder> CZstd1Decoder::Create()
{
		return make_shared<CZstd1Decoder>();
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CZstd1Decoder::Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, std::uint32_t height)
{
        const ZStd1HeaderParsingResult zStd1Header = ParseZStd1Header(static_cast<const uint8_t*>(ptrData), size);
		if (zStd1Header.headerSize == 0)
		{
				if (GetSite()->IsEnabled(LOGLEVEL_ERROR))
				{
						GetSite()->Log(LOGLEVEL_ERROR, "Exception 'runtime_error' caught from ZStd1-decoder-invocation -> \"Error in ZSTD1 header\".");
				}

        throw runtime_error("The zstd1-header is invalid.");
		}

		if (zStd1Header.headerSize >= size)
		{
        throw runtime_error("Zstd1-compressed data is invalid.");
		}

    if (zStd1Header.hiLoByteUnpackPreprocessing == true &&
        (pixelType != PixelType::Gray16 && pixelType != PixelType::Bgr48))
    {
        stringstream ss;
        ss << "The preprocessing \"LoHiBytePacking\" is only supported for pixeltypes \"Gray16\" or \"Bgr48\", but was requested for pixeltype \"" << Utils::PixelTypeToInformalString(pixelType) << "\".";
        throw runtime_error(ss.str());
    }

    return DecodeAndProcess(
        static_cast<const char*>(ptrData) + zStd1Header.headerSize,
        size - zStd1Header.headerSize,
        pixelType,
        width,
        height,
        zStd1Header.hiLoByteUnpackPreprocessing);
}

ZStd1HeaderParsingResult ParseZStd1Header(const uint8_t* ptrData, size_t size)
{
		// set the out-parameter to the default value (which is false)
		ZStd1HeaderParsingResult retVal{ 0, false };

		if (size < 1)
		{
				// 1 byte is the absolute minimum of data we need
				return retVal;
		}

		// the only possible values currently are: either 1 (i. e. not chunk) or 3 (so we expect the only existing chunk-type "1", which has
		//  a fixed size of 2 bytes
    if (*ptrData == 1)
		{
				// this is valid, and it means that the size of the header is 1 byte
				retVal.headerSize = 1;
				return retVal;
		}

    if (*ptrData == 3)
		{
				// in this case... the size must be at least 3 
				if (size < 3)
				{
						return retVal;
				}
		}

		// is "chunk type 1" next?
    if (*(ptrData + 1) == 1)
		{
				// yes, so now the LSB gives the information about "hiLoByteUnpackPreprocessing"
        retVal.hiLoByteUnpackPreprocessing = ((*(ptrData + 2)) & 1) == 1;

				retVal.headerSize = 3;
				return retVal;
		}

		// we currently don't have any other chunk-type, so we fall through to "error return"
		// otherwise - this is not a valid "zstd1"-header
		return retVal;
}

shared_ptr<libCZI::IBitmapData> DecodeAndProcess(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height, bool doHiLoByteUnpacking)
{
    const unsigned long long uncompressedSize = ZSTD_getFrameContentSize(ptrData, size);
		if (uncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
		{
				throw std::runtime_error("The decompressed size cannot be determined.");
		}
		else if (uncompressedSize == ZSTD_CONTENTSIZE_ERROR)
		{
				throw std::runtime_error("The compressed data is not recognized.");
		}

    if (uncompressedSize != static_cast<uint64_t>(height) * width * Utils::GetBytesPerPixel(pixelType))
		{
				throw std::runtime_error("The compressed data is not valid.");
		}

		return doHiLoByteUnpacking ?
				DecodeAndProcessWithHiLoByteUnpacking(ptrData, size, pixelType, width, height) :
				DecodeAndProcessNoHiLoByteUnpacking(ptrData, size, pixelType, width, height);
}

shared_ptr<libCZI::IBitmapData> DecodeAndProcessWithHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
    const size_t uncompressedSize = static_cast<size_t>(height) * width * Utils::GetBytesPerPixel(pixelType);
    unique_ptr<void, void(*)(void*)> tmpBuffer(malloc(uncompressedSize), free);
		ZstdDecompressAndThrowIfError(ptrData, size, tmpBuffer.get(), uncompressedSize);
    const auto bytesPerPel = Utils::GetBytesPerPixel(pixelType);
    auto bitmap = CStdBitmapData::Create(pixelType, width, height);
		auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
    LoHiBytePackUnpack::LoHiBytePackStrided(tmpBuffer.get(), uncompressedSize, width * bytesPerPel / 2, height, bmLckInfo.stride, bmLckInfo.ptrDataRoi);
		return bitmap;
}

shared_ptr<libCZI::IBitmapData> DecodeAndProcessNoHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
    const size_t uncompressedSize = static_cast<size_t>(height) * width * Utils::GetBytesPerPixel(pixelType);
		auto bitmap = CStdBitmapData::Create(pixelType, width, height, width * Utils::GetBytesPerPixel(pixelType));
		auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
		ZstdDecompressAndThrowIfError(ptrData, size, bmLckInfo.ptrDataRoi, uncompressedSize);
		return bitmap;
}

void ZstdDecompressAndThrowIfError(const void* ptrData, size_t size, void* ptrDst, size_t dstSize)
{
    const size_t decompressedSize = ZSTD_decompress(ptrDst, dstSize, ptrData, size);
		if (ZSTD_isError(decompressedSize))
		{
				switch (ZSTD_ErrorCode ec = ZSTD_getErrorCode(decompressedSize))
				{
				case ZSTD_error_dstSize_tooSmall:
						throw std::runtime_error("The size of the output-buffer is too small.");
				default:
                        const std::string errorText = "\"ZSTD_decompress\" returned with error-code " + std::to_string(ec) + ".";
						throw std::runtime_error(errorText);
				}
		}
}
