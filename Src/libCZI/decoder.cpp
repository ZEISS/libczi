// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "decoder.h"
#include "../JxrDecode/JxrDecode.h"
#include "bitmapData.h"
#include "stdAllocator.h"
#include "BitmapOperations.h"
#include "Site.h"

using namespace libCZI;
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

std::shared_ptr<libCZI::IBitmapData> CJxrLibDecoder::Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
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

                if (pixel_type_from_compressed_data != pixelType)
                {
                    ostringstream ss;
                    ss << "pixel type mismatch: expected \"" << Utils::PixelTypeToInformalString(pixelType) << "\", but got \"" << Utils::PixelTypeToInformalString(pixel_type_from_compressed_data) << "\"";
                    throw std::logic_error(ss.str());
                }

                if (actual_width != width || actual_height != height)
                {
                    ostringstream ss;
                    ss << "size mismatch: expected " << width << "x" << height << ", but got " << actual_width << "x" << actual_height;
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
