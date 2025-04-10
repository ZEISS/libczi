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

namespace
{
    bool IsTokenMatch(const char* start, const char* token, size_t token_len)
    {
        // Match string content
        if (std::strncmp(start, token, token_len) != 0)
        {
            return false;
        }

        // Must be followed by ;, space, or null
        const char after = start[token_len];
        if (after != '\0' && after != ';' && !std::isspace(static_cast<unsigned char>(after)))
        {
            return false;
        }

        return true;
    }

    /// Parse the options string and check if it contains the specified token. The syntax for the
    /// options string is a semicolon-separated list of items.
    ///
    /// \param  input   The options string to parse. If nullptr, the function returns false.
    /// \param  token   The string to search for. If nullptr or empty, the function returns false.
    ///
    /// \returns    True if the specified string is found; false otherwise.
    bool ContainsToken(const char* input, const char* token)
    {
        if (!input || !token || *token == '\0')
        {
            return false;
        }

        const size_t token_len = std::strlen(token);
        const char* current = input;

        while ((current = std::strstr(current, token)))
        {
            // Check that we're at token boundary: either start or preceded by ; or whitespace
            if (current != input) 
            {
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

    uint64_t GetZstdContentSizeOrThrow(const void* ptr_data, size_t size)
    {
        const auto zstd_frame_content_size = ZSTD_getFrameContentSize(ptr_data, size);
        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_ERROR)
        {
            throw runtime_error("Could not determine size of zstd-compressed data: ZSTD_CONTENTSIZE_ERROR");
        }

        if (zstd_frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN)
        {
            throw runtime_error("Could not determine size of zstd-compressed data: ZSTD_CONTENTSIZE_UNKNOWN");
        }

        return zstd_frame_content_size;
    }

    size_t DecompressAndThrowIfError(const void* ptr_compressed_data, size_t size_compressed_data, void* ptr_destination, size_t size_destination, size_t expected_decompressed_size)
    {
        const size_t decompressed_size = ZSTD_decompress(ptr_destination, size_destination, ptr_compressed_data, size_compressed_data);
        if (ZSTD_isError(decompressed_size))
        {
            ostringstream ss;
            ss << "Zstd-decompression failed with error: " << ZSTD_getErrorName(decompressed_size);
            throw runtime_error(ss.str());
        }

        if (decompressed_size != expected_decompressed_size)
        {
            ostringstream ss;
            ss << "Zstd-decompression produced unexpected size. Expected: " << expected_decompressed_size << ", actual: " << decompressed_size;
            throw runtime_error(ss.str());
        }

        return decompressed_size;
    }

    /// Decodes zstd-compressed data to give a bitmap of the specified characteristics. If the size of the
    /// zstd-compressed data does not exactly match the size of the bitmap, an exception is thrown.
    ///
    /// \exception  runtime_error   Raised when any sort of data mismatch is encountered.
    ///
    /// \param  ptr_data    Pointer to the zstd-compressed data.
    /// \param  size        The size of the zstd-compressed data.
    /// \param  pixel_type  The pixel type of the destination bitmap.
    /// \param  width       The width of the destination bitmap.
    /// \param  height      The height of the destination bitmap.
    ///
    /// \returns    The bitmap object containing the decoded data.
    shared_ptr<libCZI::IBitmapData> DecodeRequireCorrectSize(const void* ptr_data, size_t size, libCZI::PixelType pixel_type, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        const size_t stride = width * static_cast<size_t>(Utils::GetBytesPerPixel(pixel_type));
        const size_t expected_size = height * stride;
        const auto zstd_frame_content_size = GetZstdContentSizeOrThrow(ptr_data, size);
        if (zstd_frame_content_size != expected_size)
        {
            stringstream ss;
            ss << "Zstd-compressed data has unexpected size. Expected: " << expected_size << ", actual: " << zstd_frame_content_size;
            throw runtime_error(ss.str());
        }

        auto bitmap = CStdBitmapData::Create(pixel_type, width, height, stride);
        auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);

        // Decompress the data into the bitmap       
        DecompressAndThrowIfError(ptr_data, size, bitmap_lock_info.ptrDataRoi, expected_size, zstd_frame_content_size);

        return bitmap;
    }

    /// Decodes zstd-compressed data AND do hi-lo-byte-packing to give a bitmap of the specified characteristics. If the size of the
    /// zstd-compressed data does not exactly match the size of the bitmap, an exception is thrown.
    ///
    /// \exception  runtime_error   Raised when any sort of data mismatch is encountered.
    ///
    /// \param  ptr_data    Pointer to the zstd-compressed data.
    /// \param  size        The size of the zstd-compressed data.
    /// \param  pixel_type  The pixel type of the destination bitmap (precondition: since hi-lo-byte-packing only works with 16-bit integer type, this must be either Gray16 or Bgr48).
    /// \param  width       The width of the destination bitmap.
    /// \param  height      The height of the destination bitmap.
    ///
    /// \returns    The bitmap object containing the decoded data.
    shared_ptr<libCZI::IBitmapData> DecodeAndHiLoBytePackRequireCorrectSize(const void* ptr_data, size_t size, libCZI::PixelType pixel_type, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        const auto bytes_per_pel = Utils::GetBytesPerPixel(pixel_type);
        size_t stride = width * static_cast<size_t>(bytes_per_pel);
        size_t expected_size = height * stride;
        const auto zstd_frame_content_size = GetZstdContentSizeOrThrow(ptr_data, size);
        if (zstd_frame_content_size != expected_size)
        {
            stringstream ss;
            ss << "Zstd-compressed data has unexpected size. Expected: " << expected_size << ", actual: " << zstd_frame_content_size;
            throw runtime_error(ss.str());
        }

        unique_ptr<void, void(*)(void*)> temporary_buffer(malloc(expected_size), free);
        if (temporary_buffer == nullptr)
        {
            throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
        }

        const size_t decompressed_size = DecompressAndThrowIfError(ptr_data, size, temporary_buffer.get(), expected_size, zstd_frame_content_size);

        auto bitmap = CStdBitmapData::Create(pixel_type, width, height);
        const auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);

        // Note: "width * bytes_per_pel / 2" gives the "number of 16-bit pels" in a row, and we divide by 2 because that's
        //        the number of bytes per pel for a 16-bit pel. This gives the correct size also for Bgr48.
        LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), decompressed_size, width * bytes_per_pel / 2, height, bitmap_lock_info.stride, bitmap_lock_info.ptrDataRoi);
        return bitmap;
    }

    /// Decodes zstd-compressed data to give a bitmap of the specified characteristics. If the size of the
    /// zstd-compressed data and the specified destination bitmap do not match, the function will apply the
    /// "resolution protocol" - i.e. fill the bitmap with decoded data, filling the remainder with zeroes if it
    /// is too small, and discard data which is too large.
    ///
    /// \exception  runtime_error   Raised when any sort of data mismatch is encountered.
    ///
    /// \param  ptr_data    Pointer to the zstd-compressed data.
    /// \param  size        The size of the zstd-compressed data.
    /// \param  pixel_type  The pixel type of the destination bitmap.
    /// \param  width       The width of the destination bitmap.
    /// \param  height      The height of the destination bitmap.
    ///
    /// \returns    The bitmap object containing the decoded data.
    shared_ptr<libCZI::IBitmapData> DecodeAndHandleSizeMismatch(const void* ptr_data, size_t size, libCZI::PixelType pixel_type, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        size_t stride = width * static_cast<size_t>(Utils::GetBytesPerPixel(pixel_type));
        size_t expected_size = height * stride;
        const auto zstd_frame_content_size = GetZstdContentSizeOrThrow(ptr_data, size);

        auto bitmap = CStdBitmapData::Create(pixel_type, width, height, stride);
        if (zstd_frame_content_size == expected_size)
        {
            // sizes match, so we can decode normally
            auto bmLckInfo = libCZI::ScopedBitmapLockerSP(bitmap);
            size_t decompressed_size = ZSTD_decompress(bmLckInfo.ptrDataRoi, expected_size, ptr_data, size);
            if (zstd_frame_content_size != decompressed_size)
            {
                stringstream ss;
                ss << "Zstd-decompression produced unexpected size. Expected: " << zstd_frame_content_size << ", actual: " << decompressed_size;
                throw runtime_error(ss.str());
            }

            return bitmap;
        }
        else if (zstd_frame_content_size < expected_size)
        {
            // sizes mismatch, and the decoded size is less than expected - so we need to decode and then fill up with zeroes
            auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);

            const size_t decompressed_size = DecompressAndThrowIfError(ptr_data, size, bitmap_lock_info.ptrDataRoi, expected_size, zstd_frame_content_size);

            // fill up the rest with zeroes
            memset(static_cast<uint8_t*>(bitmap_lock_info.ptrDataRoi) + decompressed_size, 0, expected_size - decompressed_size);
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

            DecompressAndThrowIfError(ptr_data, size, temporary_buffer.get(), zstd_frame_content_size, zstd_frame_content_size);

            auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);
            memcpy(bitmap_lock_info.ptrDataRoi, temporary_buffer.get(), expected_size);

            return bitmap;
        }
    }

    /// Decodes zstd-compressed data AND do lo-hi-byte-packing to give a bitmap of the specified characteristics. If the size of the
    /// zstd-compressed data and the specified destination bitmap do not match, the function will apply the
    /// "resolution protocol" - i.e. fill the bitmap with decoded data, filling the remainder with zeroes if it
    /// is too small, and discard data which is too large.
    ///
    /// \exception  runtime_error   Raised when any sort of data mismatch is encountered.
    ///
    /// \param  ptr_data    Pointer to the zstd-compressed data.
    /// \param  size        The size of the zstd-compressed data.
    /// \param  pixel_type  The pixel type of the destination bitmap.
    /// \param  width       The width of the destination bitmap.
    /// \param  height      The height of the destination bitmap.
    ///
    /// \returns    The bitmap object containing the decoded data.
    shared_ptr<libCZI::IBitmapData> DecodeAndHiLoBytePackAndHandleSizeMismatch(const void* ptr_data, size_t size, libCZI::PixelType pixel_type, uint32_t width, uint32_t height)
    {
        // calculate the expected size of the uncompressed data
        const auto bytes_per_pel = Utils::GetBytesPerPixel(pixel_type);
        size_t stride = width * static_cast<size_t>(bytes_per_pel);
        size_t expectedSize = height * stride;
        const auto zstd_frame_content_size = GetZstdContentSizeOrThrow(ptr_data, size);

        if (zstd_frame_content_size == expectedSize)
        {
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            DecompressAndThrowIfError(ptr_data, size, temporary_buffer.get(), zstd_frame_content_size, zstd_frame_content_size);

            auto bitmap = CStdBitmapData::Create(pixel_type, width, height);
            auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);
            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, stride / bytes_per_pel, height, bitmap_lock_info.stride, bitmap_lock_info.ptrDataRoi);
            return bitmap;
        }
        else if (zstd_frame_content_size < expectedSize)
        {
            unique_ptr<void, decltype(&free)> temporary_buffer(malloc(zstd_frame_content_size), free);
            if (temporary_buffer == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            const size_t decompressed_size = DecompressAndThrowIfError(ptr_data, size, temporary_buffer.get(), zstd_frame_content_size, zstd_frame_content_size);

            auto bitmap = CStdBitmapData::Create(pixel_type, width, height, stride);
            auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);
            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, decompressed_size / bytes_per_pel, 1, zstd_frame_content_size, bitmap_lock_info.ptrDataRoi);
            memset(static_cast<uint8_t*>(bitmap_lock_info.ptrDataRoi) + decompressed_size, 0, expectedSize - decompressed_size);
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

            const size_t decompressed_size = DecompressAndThrowIfError(ptr_data, size, temporary_buffer.get(), zstd_frame_content_size, zstd_frame_content_size);

            // Ok, now we need an additional temporary buffer for the packing (we simply cannot pack into the
            //  destination bitmap, at least not without a new LoHiBytePack-method which would allow this)
            unique_ptr<void, decltype(&free)> temporary_buffer_for_packed(malloc(zstd_frame_content_size), free);
            if (temporary_buffer_for_packed == nullptr)
            {
                throw runtime_error("Failed to allocate temporary buffer for Zstd-decompression.");
            }

            LoHiBytePackUnpack::LoHiBytePackStrided(temporary_buffer.get(), zstd_frame_content_size, decompressed_size / bytes_per_pel, 1, zstd_frame_content_size, temporary_buffer_for_packed.get());

            // now we can release the first temporary buffer
            temporary_buffer.reset();

            auto bitmap = CStdBitmapData::Create(pixel_type, width, height, stride);
            auto bitmap_lock_info = libCZI::ScopedBitmapLockerSP(bitmap);
            memcpy(bitmap_lock_info.ptrDataRoi, temporary_buffer_for_packed.get(), expectedSize);
            return bitmap;
        }
    }

    struct ZStd1HeaderParsingResult
    {
        /// Size of the header in bytes. If this is zero, the header did not parse correctly.
        size_t headerSize;

        /// A boolean indicating whether a hi-lo-byte packing is to be done.
        bool hiLoByteUnpackPreprocessing;
    };
    
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
}
