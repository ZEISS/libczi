// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "decoder.h"
#include "../JxrDecode/JxrDecode.h"
#include "turbojpeg.h"
#include "bitmapData.h"
#include "stdAllocator.h"
#include "BitmapOperations.h"
#include "Site.h"
#include <limits>

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;


static libCZI::PixelType PixelTypeFromJxrPixelFormat(JxrDecode::PixelFormat pixel_format)
{
    switch (pixel_format)
    {
    case JxrDecode::PixelFormat::kBgr24:
        return PixelType::Bgr24;
    case JxrDecode::PixelFormat::kGray8:
        return PixelType::Gray8;
    case JxrDecode::PixelFormat::kBgr48:
        return PixelType::Bgr48;
    case JxrDecode::PixelFormat::kGray16:
        return PixelType::Gray16;
    case JxrDecode::PixelFormat::kGray32Float:
        return PixelType::Gray32Float;
    default:
        return PixelType::Invalid;
    }
}

/*static*/std::shared_ptr<CJxrLibDecoder> CJxrLibDecoder::Create()
{
    return make_shared<CJxrLibDecoder>();
}

std::shared_ptr<libCZI::IBitmapData> CJxrLibDecoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const uint32_t* width, const uint32_t* height, const char* additional_arguments)
{
    (void)additional_arguments;

    std::shared_ptr<IBitmapData> bitmap;
    bool bitmap_is_locked = false;

    try
    {
        JxrDecode::Decode(
            ptrData,
            size,
            [&](JxrDecode::PixelFormat actual_pixel_format, std::uint32_t actual_width, std::uint32_t actual_height)
            -> tuple<void*, uint32_t>
            {
                const auto pixel_type_from_compressed_data = PixelTypeFromJxrPixelFormat(actual_pixel_format);
                if (pixel_type_from_compressed_data == PixelType::Invalid)
                {
                    throw std::logic_error("unsupported pixel type");
                }

                if (pixelType != nullptr && pixel_type_from_compressed_data != *pixelType)
                {
                    ostringstream ss;
                    ss << "pixel type mismatch: expected \"" << Utils::PixelTypeToInformalString(*pixelType) << "\", but got \"" << Utils::PixelTypeToInformalString(pixel_type_from_compressed_data) << "\"";
                    throw std::logic_error(ss.str());
                }

                if (width != nullptr && actual_width != *width)
                {
                    ostringstream ss;
                    ss << "width mismatch: expected " << *width << ", but got " << actual_width;
                    throw std::logic_error(ss.str());
                }

                if (height != nullptr && actual_height != *height)
                {
                    ostringstream ss;
                    ss << "height mismatch: expected " << *height << ", but got " << actual_height;
                    throw std::logic_error(ss.str());
                }

                bitmap = GetSite()->CreateBitmap(pixel_type_from_compressed_data, actual_width, actual_height);
                const auto lock_info = bitmap->Lock();
                bitmap_is_locked = true;
                return make_tuple(lock_info.ptrDataRoi, lock_info.stride);
            });
    }
    catch (const std::exception& e)
    {
        GetSite()->Log(LOGLEVEL_ERROR, e.what());
        if (bitmap_is_locked)
        {
            bitmap->Unlock();
        }

        throw;
    }

    bitmap->Unlock();

    // if the pixel type was "Rgb48", then we need to convert it to Bgr48 (which is what the rest of the code expects), and unfortunately
    //  the decoder at this point does not allow to swap the channels, so we need to do it here
    if (bitmap->GetPixelType() == PixelType::Bgr48)
    {
        const ScopedBitmapLockerSP bmLck(bitmap);
        CBitmapOperations::RGB48ToBGR48(
            bitmap->GetWidth(),
            bitmap->GetHeight(),
            static_cast<uint16_t*>(bmLck.ptrDataRoi),
            bmLck.stride);
    }

    return bitmap;
}

// ----------------------------------------------------------------------------

std::shared_ptr<JpgDecoder> JpgDecoder::Create()
{
    return make_shared<JpgDecoder>();
}

std::shared_ptr<libCZI::IBitmapData> JpgDecoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments)
{
    (void)additional_arguments;
    if (ptrData == nullptr || size == 0)
    {
        throw std::invalid_argument("JPEG input is empty");
    }

    // A site can share its decoder across threads. Keep the codec state local.
    const std::unique_ptr<void, decltype(&tj3Destroy)> decoder(tj3Init(TJINIT_DECOMPRESS), tj3Destroy);
    if (!decoder)
    {
        throw std::runtime_error(tj3GetErrorStr(nullptr));
    }

    const auto* jpeg = static_cast<const unsigned char*>(ptrData);
    if (tj3Set(decoder.get(), TJPARAM_STOPONWARNING, 1) != 0 ||
        tj3DecompressHeader(decoder.get(), jpeg, size) != 0)
    {
        throw std::runtime_error(tj3GetErrorStr(decoder.get()));
    }

    const int actual_width = tj3Get(decoder.get(), TJPARAM_JPEGWIDTH);
    const int actual_height = tj3Get(decoder.get(), TJPARAM_JPEGHEIGHT);
    if (actual_width <= 0 || actual_height <= 0)
    {
        throw std::runtime_error("Invalid JPEG dimensions");
    }

    if ((width != nullptr && *width != static_cast<std::uint32_t>(actual_width)) ||
        (height != nullptr && *height != static_cast<std::uint32_t>(actual_height)))
    {
        throw std::logic_error("JPEG dimensions do not match the expected dimensions");
    }

    if (tj3Get(decoder.get(), TJPARAM_PRECISION) != 8)
    {
        throw std::runtime_error("Only 8-bit JPEG samples are supported");
    }

    const int color_space = tj3Get(decoder.get(), TJPARAM_COLORSPACE);
    if (color_space != TJCS_GRAY && color_space != TJCS_RGB && color_space != TJCS_YCbCr)
    {
        throw std::runtime_error("Unsupported JPEG color space");
    }

    const bool grayscale = color_space == TJCS_GRAY;
    const PixelType actual_pixel_type = grayscale ? PixelType::Gray8 : PixelType::Bgr24;
    if (pixelType != nullptr && *pixelType != actual_pixel_type)
    {
        throw std::logic_error("JPEG pixel type does not match the expected pixel type");
    }

    auto bitmap = GetSite()->CreateBitmap(actual_pixel_type, actual_width, actual_height);
    const ScopedBitmapLockerSP lock(bitmap);
    const int pixel_format = grayscale ? TJPF_GRAY : TJPF_BGR;
    if (lock.stride < static_cast<std::uint32_t>(actual_width) * tjPixelSize[pixel_format] ||
        lock.stride > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
        static_cast<std::uint64_t>(lock.stride) * actual_height > (std::numeric_limits<size_t>::max)())
    {
        throw std::runtime_error("Invalid bitmap stride for JPEG decoding");
    }

    if (tj3Decompress8(decoder.get(), jpeg, size, static_cast<unsigned char*>(lock.ptrDataRoi),
                        static_cast<int>(lock.stride), pixel_format) != 0)
    {
        throw std::runtime_error(tj3GetErrorStr(decoder.get()));
    }

    return bitmap;
}
