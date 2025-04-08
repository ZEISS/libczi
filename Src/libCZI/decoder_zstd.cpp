// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

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

#include <cstring>

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
static bool ContainsToken(const char* str, const char* token);
static bool IsTokenMatch(const char* start, const char* token, size_t token_len);

namespace
{
    shared_ptr<libCZI::IBitmapData> DecodeRequireCorrectSize(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        size_t stride = width * static_cast<size_t>(Utils::GetBytesPerPixel(pixelType));
        size_t expectedSize = height * stride;
        const auto zstd_frame_content_size = ZSTD_getFrameContentSize(ptrData, size);
        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_ERROR)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }
        else if (zstd_frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }

        if (zstd_frame_content_size != expectedSize)
        {
            stringstream ss;
            ss << "Zstd-compressed data has unexpected size. Expected: " << expectedSize << ", actual: " << zstd_frame_content_size;
            throw runtime_error(ss.str());
        }

        auto bitmap = CStdBitmapData::Create(pixelType, width, height, stride);
        auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
        const size_t decompressedSize = ZSTD_decompress(bmLckInfo.ptrDataRoi, expectedSize, ptrData, size);
        if (ZSTD_isError(decompressedSize))
        {
            stringstream ss;
            ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
            throw runtime_error(ss.str());
        }

        return bitmap;
    }

    shared_ptr<libCZI::IBitmapData> DecodeAndHiLoBytePackRequireCorrectSize(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        const auto bytes_per_pel = Utils::GetBytesPerPixel(pixelType);
        size_t stride = width * static_cast<size_t>(bytes_per_pel);
        size_t expectedSize = height * stride;
        const auto zstd_frame_content_size = ZSTD_getFrameContentSize(ptrData, size);
        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_ERROR)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }
        else if (zstd_frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }

        if (zstd_frame_content_size != expectedSize)
        {
            stringstream ss;
            ss << "Zstd-compressed data has unexpected size. Expected: " << expectedSize << ", actual: " << zstd_frame_content_size;
            throw runtime_error(ss.str());
        }

        unique_ptr<void, void(*)(void*)> temporary_buffer(malloc(expectedSize), free);
        if (temporary_buffer == nullptr)
        {
            throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
        }

        const size_t decompressedSize = ZSTD_decompress(temporary_buffer.get(), expectedSize, ptrData, size);
        if (ZSTD_isError(decompressedSize))
        {
            stringstream ss;
            ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
            throw runtime_error(ss.str());
        }

        auto bitmap = CStdBitmapData::Create(pixelType, width, height);
        auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
        LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), decompressedSize, width * bytes_per_pel / 2, height, bmLckInfo.stride, bmLckInfo.ptrDataRoi);
        return bitmap;
    }

    shared_ptr<libCZI::IBitmapData> DecodeAndHandleSizeMismatch(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        size_t stride = width * static_cast<size_t>(Utils::GetBytesPerPixel(pixelType));
        size_t expectedSize = height * stride;
        const auto zstd_frame_content_size = ZSTD_getFrameContentSize(ptrData, size);
        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_ERROR)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }
        else if (zstd_frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }

        auto bitmap = CStdBitmapData::Create(pixelType, width, height, stride);
        if (zstd_frame_content_size == expectedSize)
        {
            // sizes match, so we can decode normally
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            size_t decompressedSize = ZSTD_decompress(bmLckInfo.ptrDataRoi, expectedSize, ptrData, size);
            return bitmap;
        }
        else if (zstd_frame_content_size < expectedSize)
        {
            // sizes mismatch, and the decoded size is less than expected - so we need to decode and then fill up with zeroes
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            size_t decompressedSize = ZSTD_decompress(bmLckInfo.ptrDataRoi, zstd_frame_content_size, ptrData, size);
            if (ZSTD_isError(decompressedSize))
            {
                stringstream ss;
                ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
                throw runtime_error(ss.str());
            }

            // fill up the rest with zeroes
            memset(static_cast<uint8_t*>(bmLckInfo.ptrDataRoi) + decompressedSize, 0, expectedSize - decompressedSize);
            return bitmap;
        }
        else
        {
            // sizes mismatch, and the decoded size is larger than expected - we need to decode to a temporary buffer, and
            // copy from there into the bitmap
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            size_t decompressedSize = ZSTD_decompress(temporary_buffer.get(), zstd_frame_content_size, ptrData, size);
            if (ZSTD_isError(decompressedSize))
            {
                stringstream ss;
                ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
                throw runtime_error(ss.str());
            }

            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            memcpy(bmLckInfo.ptrDataRoi, temporary_buffer.get(), expectedSize);

            return bitmap;
        }
    }

    shared_ptr<libCZI::IBitmapData> DecodeAndHiLoBytePackAndHandleSizeMismatch(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        const auto bytes_per_pel = Utils::GetBytesPerPixel(pixelType);
        size_t stride = width * static_cast<size_t>(bytes_per_pel);
        size_t expectedSize = height * stride;
        const auto zstd_frame_content_size = ZSTD_getFrameContentSize(ptrData, size);
        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_ERROR)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }
        else if (zstd_frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw runtime_error("Zstd-compressed data is invalid.");
        }

        //auto bitmap = CStdBitmapData::Create(pixelType, width, height, stride);
        if (zstd_frame_content_size == expectedSize)
        {
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            size_t decompressedSize = ZSTD_decompress(temporary_buffer.get(), zstd_frame_content_size, ptrData, size);
            if (ZSTD_isError(decompressedSize))
            {
                stringstream ss;
                ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
                throw runtime_error(ss.str());
            }

            auto bitmap = CStdBitmapData::Create(pixelType, width, height);
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, stride / bytes_per_pel, height, bmLckInfo.stride, bmLckInfo.ptrDataRoi);
            return bitmap;
        }
        else if (zstd_frame_content_size < expectedSize)
        {
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            size_t decompressedSize = ZSTD_decompress(temporary_buffer.get(), zstd_frame_content_size, ptrData, size);
            if (ZSTD_isError(decompressedSize))
            {
                stringstream ss;
                ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
                throw runtime_error(ss.str());
            }

            auto bitmap = CStdBitmapData::Create(pixelType, width, height, stride);
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, decompressedSize / bytes_per_pel, 1, zstd_frame_content_size, bmLckInfo.ptrDataRoi);
            memset(static_cast<uint8_t*>(bmLckInfo.ptrDataRoi) + decompressedSize, 0, expectedSize - decompressedSize);
            return bitmap;
        }
        else
        {
            // sizes mismatch, and the decoded size is larger than expected - we need to decode to a temporary buffer, and
            // copy from there into the bitmap
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            size_t decompressedSize = ZSTD_decompress(temporary_buffer.get(), zstd_frame_content_size, ptrData, size);
            if (ZSTD_isError(decompressedSize))
            {
                stringstream ss;
                ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressedSize);
                throw runtime_error(ss.str());
            }

            // Ok, now we need an additional temporary buffer for the packing (we simply cannot unpack into the
            //  destination bitmap, at least not without a new LoHiBytePack-method which would allow this)
            unique_ptr<void, decltype(&free)> temporary_buffer_for_packed(malloc(zstd_frame_content_size), free);
            if (temporary_buffer_for_packed == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, decompressedSize / bytes_per_pel, 1, zstd_frame_content_size, temporary_buffer_for_packed.get());

            // now we can release the first temporary buffer
            temporary_buffer.reset();

            auto bitmap = CStdBitmapData::Create(pixelType, width, height, stride);
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            memcpy(bmLckInfo.ptrDataRoi, temporary_buffer_for_packed.get(), expectedSize);
            return bitmap;
        }
    }
}

/*static*/std::shared_ptr<CZstd0Decoder> CZstd0Decoder::Create()
{
    return make_shared<CZstd0Decoder>();
}

/*static*/const char* CZstd0Decoder::kOption_handle_data_size_mismatch = "handle_data_size_mismatch";

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CZstd0Decoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const uint32_t* width, const uint32_t* height, const char* additional_arguments)
{
    if (pixelType == nullptr || width == nullptr || height == nullptr)
    {
        throw invalid_argument("pixeltype, width and height must be specified.");
    }

    const bool handle_data_size_mismatch = ContainsToken(additional_arguments, kOption_handle_data_size_mismatch);
    return handle_data_size_mismatch ?
        DecodeAndHandleSizeMismatch(ptrData, size, *pixelType, *width, *height) :
        DecodeRequireCorrectSize(ptrData, size, *pixelType, *width, *height);
}

/*static*/std::shared_ptr<CZstd1Decoder> CZstd1Decoder::Create()
{
    return make_shared<CZstd1Decoder>();
}

/*static*/const char* CZstd1Decoder::kOption_handle_data_size_mismatch = "handle_data_size_mismatch";

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CZstd1Decoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const uint32_t* width, const std::uint32_t* height, const char* additional_arguments)
{
    if (pixelType == nullptr || width == nullptr || height == nullptr)
    {
        throw invalid_argument("pixeltype, width and height must be specified.");
    }

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
        (*pixelType != PixelType::Gray16 && *pixelType != PixelType::Bgr48))
    {
        stringstream ss;
        ss << "The preprocessing \"LoHiBytePacking\" is only supported for pixeltypes \"Gray16\" or \"Bgr48\", but was requested for pixeltype \"" << Utils::PixelTypeToInformalString(*pixelType) << "\".";
        throw runtime_error(ss.str());
    }

    const bool handle_data_size_mismatch = ContainsToken(additional_arguments, kOption_handle_data_size_mismatch);

    if (handle_data_size_mismatch)
    {
        if (zStd1Header.hiLoByteUnpackPreprocessing)
        {
            return DecodeAndHiLoBytePackAndHandleSizeMismatch(static_cast<const char*>(ptrData) + zStd1Header.headerSize, size - zStd1Header.headerSize, *pixelType, *width, *height);
        }
        else
        {
            return DecodeAndHandleSizeMismatch(static_cast<const char*>(ptrData) + zStd1Header.headerSize, size - zStd1Header.headerSize, *pixelType, *width, *height);
        }
    }
    else
    {
        if (zStd1Header.hiLoByteUnpackPreprocessing)
        {
            return DecodeAndHiLoBytePackRequireCorrectSize(static_cast<const char*>(ptrData) + zStd1Header.headerSize, size - zStd1Header.headerSize, *pixelType, *width, *height);
        }
        else
        {
            return DecodeRequireCorrectSize(static_cast<const char*>(ptrData) + zStd1Header.headerSize, size - zStd1Header.headerSize, *pixelType, *width, *height);
        }
    }

    /*
    return DecodeAndProcess(
        static_cast<const char*>(ptrData) + zStd1Header.headerSize,
        size - zStd1Header.headerSize,
        *pixelType,
        *width,
        *height,
        zStd1Header.hiLoByteUnpackPreprocessing);*/
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

    // the only possible values currently are: either 1 (i.e. no chunk) or 3 (so we expect the only existing chunk-type "1", which has
    //  a fixed size of 2 bytes)
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

//shared_ptr<libCZI::IBitmapData> DecodeAndProcess(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height, bool doHiLoByteUnpacking)
//{
//    const unsigned long long uncompressedSize = ZSTD_getFrameContentSize(ptrData, size);
//    if (uncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
//    {
//        throw std::runtime_error("The decompressed size cannot be determined.");
//    }
//    else if (uncompressedSize == ZSTD_CONTENTSIZE_ERROR)
//    {
//        throw std::runtime_error("The compressed data is not recognized.");
//    }
//
//    if (uncompressedSize != static_cast<uint64_t>(height) * width * Utils::GetBytesPerPixel(pixelType))
//    {
//        throw std::runtime_error("The compressed data is not valid.");
//    }
//
//    return doHiLoByteUnpacking ?
//        DecodeAndProcessWithHiLoByteUnpacking(ptrData, size, pixelType, width, height) :
//        DecodeAndProcessNoHiLoByteUnpacking(ptrData, size, pixelType, width, height);
//}

//shared_ptr<libCZI::IBitmapData> DecodeAndProcessWithHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
//{
//    const size_t uncompressedSize = static_cast<size_t>(height) * width * Utils::GetBytesPerPixel(pixelType);
//    unique_ptr<void, void(*)(void*)> tmpBuffer(malloc(uncompressedSize), free);
//    ZstdDecompressAndThrowIfError(ptrData, size, tmpBuffer.get(), uncompressedSize);
//    const auto bytesPerPel = Utils::GetBytesPerPixel(pixelType);
//    auto bitmap = CStdBitmapData::Create(pixelType, width, height);
//    auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
//    LoHiBytePackUnpack::LoHiBytePackStrided(tmpBuffer.get(), uncompressedSize, width * bytesPerPel / 2, height, bmLckInfo.stride, bmLckInfo.ptrDataRoi);
//    return bitmap;
//}

//shared_ptr<libCZI::IBitmapData> DecodeAndProcessNoHiLoByteUnpacking(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
//{
//    const size_t uncompressedSize = static_cast<size_t>(height) * width * Utils::GetBytesPerPixel(pixelType);
//    auto bitmap = CStdBitmapData::Create(pixelType, width, height, width * Utils::GetBytesPerPixel(pixelType));
//    auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
//    ZstdDecompressAndThrowIfError(ptrData, size, bmLckInfo.ptrDataRoi, uncompressedSize);
//    return bitmap;
//}

//void ZstdDecompressAndThrowIfError(const void* ptrData, size_t size, void* ptrDst, size_t dstSize)
//{
//    const size_t decompressedSize = ZSTD_decompress(ptrDst, dstSize, ptrData, size);
//    if (ZSTD_isError(decompressedSize))
//    {
//        switch (ZSTD_ErrorCode ec = ZSTD_getErrorCode(decompressedSize))
//        {
//        case ZSTD_error_dstSize_tooSmall:
//            throw std::runtime_error("The size of the output-buffer is too small.");
//        default:
//            const std::string errorText = "\"ZSTD_decompress\" returned with error-code " + std::to_string(ec) + ".";
//            throw std::runtime_error(errorText);
//        }
//    }
//}

bool IsTokenMatch(const char* start, const char* token, size_t token_len)
{
    // Match string content
    if (std::strncmp(start, token, token_len) != 0)
    {
        return false;
    }

    // Must be followed by ;, space, or null
    char after = start[token_len];
    if (after != '\0' && after != ';' && !std::isspace(static_cast<unsigned char>(after)))
    {
        return false;
    }

    return true;
}

bool ContainsToken(const char* input, const char* token)
{
    if (!input || !token || *token == '\0') return false;

    const size_t token_len = std::strlen(token);
    const char* current = input;

    while ((current = std::strstr(current, token)))
    {
        // Check that we're at token boundary: either start or preceded by ; or whitespace
        if (current != input) {
            const char before = *(current - 1);
            if (before != ';' && !std::isspace(static_cast<unsigned char>(before)))
            {
                ++current;
                continue;
            }
        }

        if (IsTokenMatch(current, token, token_len))
        {
            return true;
        }

        ++current;
    }

    return false;
}
