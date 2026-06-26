// SPDX-FileCopyrightText: 2017-2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "decoder_jxl.h"

#if LIBCZI_HAVE_LIBJXL

#include "bitmapData.h"
#include "BitmapOperations.h"
#include "inc_libCZI_Config.h"
#include "libCZI_Utilities.h"
#include "Site.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <jxl/decode.h>
#include <jxl/types.h>

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

namespace
{
    void ThrowIfBigEndianHost()
    {
#if LIBCZI_ISBIGENDIANHOST
        throw runtime_error("JPEG XL subblock decompression is not supported on big-endian hosts.");
#endif
    }

    static JxlPixelFormat OutFormatForPixelType(PixelType pt)
    {
        JxlPixelFormat fmt{};
        fmt.endianness = JXL_NATIVE_ENDIAN;
        fmt.align = 0;
        switch (pt)
        {
        case PixelType::Gray8:
            fmt.num_channels = 1;
            fmt.data_type = JXL_TYPE_UINT8;
            break;
        case PixelType::Gray16:
            fmt.num_channels = 1;
            fmt.data_type = JXL_TYPE_UINT16;
            break;
        case PixelType::Gray32Float:
            fmt.num_channels = 1;
            fmt.data_type = JXL_TYPE_FLOAT;
            break;
        case PixelType::Bgr24:
            fmt.num_channels = 3;
            fmt.data_type = JXL_TYPE_UINT8;
            break;
        case PixelType::Bgr48:
            fmt.num_channels = 3;
            fmt.data_type = JXL_TYPE_UINT16;
            break;
        case PixelType::Bgr96Float:
            fmt.num_channels = 3;
            fmt.data_type = JXL_TYPE_FLOAT;
            break;
        default:
            throw logic_error("unsupported pixel type for JXL decode");
        }

        return fmt;
    }

    static void CopyPackedToBitmap(const uint8_t* packed, size_t row_bytes, uint32_t height, const shared_ptr<IBitmapData>& bitmap)
    {
        ScopedBitmapLockerSP locker(bitmap);
        for (uint32_t y = 0; y < height; ++y)
        {
            memcpy(
                static_cast<uint8_t*>(locker.ptrDataRoi) + static_cast<size_t>(y) * locker.stride,
                packed + static_cast<size_t>(y) * row_bytes,
                row_bytes);
        }
    }

    static void SwapRgbToBgrInPlace(PixelType pt, void* ptr, uint32_t width, uint32_t height, uint32_t stride_bytes)
    {
        switch (pt)
        {
        case PixelType::Bgr24:
        {
            auto* p = static_cast<uint8_t*>(ptr);
            for (uint32_t y = 0; y < height; ++y)
            {
                uint8_t* row = p + static_cast<size_t>(y) * stride_bytes;
                for (uint32_t x = 0; x < width; ++x)
                {
                    uint8_t* px = row + x * 3;
                    std::swap(px[0], px[2]);
                }
            }
            break;
        }
        case PixelType::Bgr48:
            CBitmapOperations::RGB48ToBGR48(
                width,
                height,
                static_cast<uint16_t*>(ptr),
                stride_bytes);
            break;
        case PixelType::Bgr96Float:
        {
            auto* p = static_cast<uint8_t*>(ptr);
            for (uint32_t y = 0; y < height; ++y)
            {
                float* row = reinterpret_cast<float*>(p + static_cast<size_t>(y) * stride_bytes);
                for (uint32_t x = 0; x < width; ++x)
                {
                    float* px = row + x * 3;
                    std::swap(px[0], px[2]);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

/*static*/ std::shared_ptr<CJxlDecoder> CJxlDecoder::Create()
{
    return make_shared<CJxlDecoder>();
}

std::shared_ptr<IBitmapData> CJxlDecoder::Decode(
    const void* ptr_data,
    size_t size,
    const PixelType* pixel_type,
    const uint32_t* width,
    const uint32_t* height,
    const char* additional_arguments)
{
    (void)additional_arguments;
    ThrowIfBigEndianHost();

    if (pixel_type == nullptr || width == nullptr || height == nullptr)
    {
        throw invalid_argument("JXL decoder requires pixel type, width and height");
    }

    const size_t row_bytes = static_cast<size_t>(*width) * static_cast<size_t>(Utils::GetBytesPerPixel(*pixel_type));
    const size_t expected_bytes = row_bytes * static_cast<size_t>(*height);
    vector<uint8_t> decode_buffer(expected_bytes);
    JxlPixelFormat out_fmt = OutFormatForPixelType(*pixel_type);

    unique_ptr<JxlDecoder, void (*)(JxlDecoder*)> dec(JxlDecoderCreate(nullptr), JxlDecoderDestroy);
    if (dec == nullptr)
    {
        throw runtime_error("JxlDecoderCreate failed");
    }

    /* Only informative events (>= 0x40); low bits like JXL_DEC_NEED_IMAGE_OUT_BUFFER are invalid here. */
    const JxlDecoderStatus sub_st = JxlDecoderSubscribeEvents(
        dec.get(),
        JXL_DEC_BASIC_INFO | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE);
    if (sub_st != JXL_DEC_SUCCESS)
    {
        stringstream ss;
        ss << "JxlDecoderSubscribeEvents failed: " << static_cast<int>(sub_st);
        throw runtime_error(ss.str());
    }

    if (JxlDecoderSetInput(dec.get(), static_cast<const uint8_t*>(ptr_data), size) != JXL_DEC_SUCCESS)
    {
        throw runtime_error("JxlDecoderSetInput failed");
    }

    JxlDecoderCloseInput(dec.get());

    shared_ptr<IBitmapData> bitmap = CStdBitmapData::Create(*pixel_type, *width, *height);

    bool set_out_buffer = false;

    for (;;)
    {
        const JxlDecoderStatus st = JxlDecoderProcessInput(dec.get());
        if (st == JXL_DEC_ERROR)
        {
            throw runtime_error("JXL decode error");
        }

        if (st == JXL_DEC_BASIC_INFO)
        {
            JxlBasicInfo info;
            if (JxlDecoderGetBasicInfo(dec.get(), &info) != JXL_DEC_SUCCESS)
            {
                throw runtime_error("JxlDecoderGetBasicInfo failed");
            }

            if (info.xsize != *width || info.ysize != *height)
            {
                stringstream ss;
                ss << "JXL dimension mismatch: expected " << *width << "x" << *height << ", got " << info.xsize << "x" << info.ysize;
                throw logic_error(ss.str());
            }

            continue;
        }

        if (st == JXL_DEC_FRAME)
        {
            continue;
        }

        if (st == JXL_DEC_NEED_IMAGE_OUT_BUFFER)
        {
            size_t need_buffer_size = 0;
            if (JxlDecoderImageOutBufferSize(dec.get(), &out_fmt, &need_buffer_size) != JXL_DEC_SUCCESS)
            {
                throw runtime_error("JxlDecoderImageOutBufferSize failed");
            }

            if (need_buffer_size != expected_bytes)
            {
                stringstream ss;
                ss << "JXL output buffer size mismatch: expected " << expected_bytes << ", got " << need_buffer_size;
                throw logic_error(ss.str());
            }

            if (JxlDecoderSetImageOutBuffer(dec.get(), &out_fmt, decode_buffer.data(), decode_buffer.size()) != JXL_DEC_SUCCESS)
            {
                throw runtime_error("JxlDecoderSetImageOutBuffer failed");
            }

            set_out_buffer = true;
            continue;
        }

        if (st == JXL_DEC_FULL_IMAGE)
        {
            break;
        }

        if (st == JXL_DEC_SUCCESS)
        {
            break;
        }

        if (st == JXL_DEC_NEED_MORE_INPUT)
        {
            throw runtime_error("JXL decode: unexpected need for more input");
        }

        stringstream ss;
        ss << "JXL decode: unexpected decoder status " << static_cast<int>(st);
        throw runtime_error(ss.str());
    }

    if (!set_out_buffer)
    {
        throw runtime_error("JXL decode: output buffer was never requested");
    }

    CopyPackedToBitmap(decode_buffer.data(), row_bytes, *height, bitmap);
    {
        ScopedBitmapLockerSP locker(bitmap);
        SwapRgbToBgrInPlace(*pixel_type, locker.ptrDataRoi, *width, *height, locker.stride);
    }

    return bitmap;
}

#endif
