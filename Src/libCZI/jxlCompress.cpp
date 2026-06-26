// SPDX-FileCopyrightText: 2017-2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_compress.h"
#include "inc_libCZI_Config.h"
#include "libCZI_Utilities.h"

#if LIBCZI_HAVE_LIBJXL

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <jxl/color_encoding.h>
#include <jxl/encode.h>
#include <jxl/types.h>

using namespace libCZI;
using namespace std;

namespace
{
    class MemoryBlockOnVector : public IMemoryBlock
    {
        vector<uint8_t> data_;
    public:
        explicit MemoryBlockOnVector(vector<uint8_t>&& d)
            : data_(std::move(d))
        {
        }

        void* GetPtr() override
        {
            return this->data_.empty() ? nullptr : this->data_.data();
        }

        size_t GetSizeOfData() const override
        {
            return this->data_.size();
        }
    };

    static void ThrowIfBigEndianHost()
    {
#if LIBCZI_ISBIGENDIANHOST
        throw runtime_error("JPEG XL subblock compression is not supported on big-endian hosts.");
#endif
    }

    static size_t MinStrideBytes(PixelType pt, uint32_t width)
    {
        return static_cast<size_t>(width) * static_cast<size_t>(Utils::GetBytesPerPixel(pt));
    }

    static void CopyRowsToPacked(const void* src, uint32_t src_stride, uint32_t width, uint32_t height, size_t row_bytes, void* dst)
    {
        auto* d = static_cast<uint8_t*>(dst);
        const auto* s = static_cast<const uint8_t*>(src);
        for (uint32_t y = 0; y < height; ++y)
        {
            memcpy(d + static_cast<size_t>(y) * row_bytes, s + static_cast<size_t>(y) * src_stride, row_bytes);
        }
    }

    static void Bgr24ToRgb24Packed(const void* src, uint32_t src_stride, uint32_t width, uint32_t height, uint8_t* dst_rgb)
    {
        for (uint32_t y = 0; y < height; ++y)
        {
            const auto* row = static_cast<const uint8_t*>(src) + static_cast<size_t>(y) * src_stride;
            uint8_t* d = dst_rgb + static_cast<size_t>(y) * width * 3;
            for (uint32_t x = 0; x < width; ++x)
            {
                d[0] = row[2];
                d[1] = row[1];
                d[2] = row[0];
                row += 3;
                d += 3;
            }
        }
    }

    static void Bgr48ToRgb48Packed(const void* src, uint32_t src_stride, uint32_t width, uint32_t height, uint16_t* dst_rgb)
    {
        for (uint32_t y = 0; y < height; ++y)
        {
            const auto* row = static_cast<const uint8_t*>(src) + static_cast<size_t>(y) * src_stride;
            uint16_t* d = dst_rgb + static_cast<size_t>(y) * width * 3;
            for (uint32_t x = 0; x < width; ++x)
            {
                const auto* p = reinterpret_cast<const uint16_t*>(row);
                d[0] = p[2];
                d[1] = p[1];
                d[2] = p[0];
                row += 6;
                d += 3;
            }
        }
    }

    static void Bgr96FloatToRgb96Packed(const void* src, uint32_t src_stride, uint32_t width, uint32_t height, float* dst_rgb)
    {
        for (uint32_t y = 0; y < height; ++y)
        {
            const auto* row = static_cast<const uint8_t*>(src) + static_cast<size_t>(y) * src_stride;
            float* d = dst_rgb + static_cast<size_t>(y) * width * 3;
            for (uint32_t x = 0; x < width; ++x)
            {
                const auto* p = reinterpret_cast<const float*>(row);
                d[0] = p[2];
                d[1] = p[1];
                d[2] = p[0];
                row += 12;
                d += 3;
            }
        }
    }

    static void FillBasicInfoForPixelType(PixelType pt, uint32_t width, uint32_t height, JxlBasicInfo* bi)
    {
        memset(bi, 0, sizeof(*bi));
        JxlEncoderInitBasicInfo(bi);
        bi->xsize = width;
        bi->ysize = height;
        bi->num_extra_channels = 0;
        bi->alpha_bits = 0;
        bi->orientation = JXL_ORIENT_IDENTITY;
        bi->have_container = JXL_FALSE;
        /* Required for JxlEncoderSetFrameLossless when basic info implies XYB. */
        bi->uses_original_profile = JXL_TRUE;

        switch (pt)
        {
        case PixelType::Gray8:
            bi->bits_per_sample = 8;
            bi->exponent_bits_per_sample = 0;
            bi->num_color_channels = 1;
            break;
        case PixelType::Gray16:
            bi->bits_per_sample = 16;
            bi->exponent_bits_per_sample = 0;
            bi->num_color_channels = 1;
            break;
        case PixelType::Gray32Float:
            bi->bits_per_sample = 32;
            bi->exponent_bits_per_sample = 8;
            bi->num_color_channels = 1;
            break;
        case PixelType::Bgr24:
            bi->bits_per_sample = 8;
            bi->exponent_bits_per_sample = 0;
            bi->num_color_channels = 3;
            break;
        case PixelType::Bgr48:
            bi->bits_per_sample = 16;
            bi->exponent_bits_per_sample = 0;
            bi->num_color_channels = 3;
            break;
        case PixelType::Bgr96Float:
            bi->bits_per_sample = 32;
            bi->exponent_bits_per_sample = 8;
            bi->num_color_channels = 3;
            break;
        default:
            throw logic_error("unsupported pixel type for JXL");
        }
    }

    static void SetColorEncodingForPixelType(PixelType pt, JxlColorEncoding* color)
    {
        switch (pt)
        {
        case PixelType::Gray8:
        case PixelType::Gray16:
        case PixelType::Bgr24:
        case PixelType::Bgr48:
            JxlColorEncodingSetToSRGB(color, (pt == PixelType::Gray8 || pt == PixelType::Gray16) ? JXL_TRUE : JXL_FALSE);
            break;
        case PixelType::Gray32Float:
        case PixelType::Bgr96Float:
            JxlColorEncodingSetToLinearSRGB(color, (pt == PixelType::Gray32Float) ? JXL_TRUE : JXL_FALSE);
            break;
        default:
            throw logic_error("unsupported pixel type for JXL");
        }
    }

    static JxlPixelFormat PixelFormatFor(PixelType pt)
    {
        JxlPixelFormat fmt{};
        fmt.endianness = JXL_NATIVE_ENDIAN;
        fmt.align = 0;
        switch (pt)
        {
        case PixelType::Gray8:
        case PixelType::Bgr24:
            fmt.num_channels = (pt == PixelType::Gray8) ? 1u : 3u;
            fmt.data_type = JXL_TYPE_UINT8;
            break;
        case PixelType::Gray16:
        case PixelType::Bgr48:
            fmt.num_channels = (pt == PixelType::Gray16) ? 1u : 3u;
            fmt.data_type = JXL_TYPE_UINT16;
            break;
        case PixelType::Gray32Float:
        case PixelType::Bgr96Float:
            fmt.num_channels = (pt == PixelType::Gray32Float) ? 1u : 3u;
            fmt.data_type = JXL_TYPE_FLOAT;
            break;
        default:
            throw logic_error("unsupported pixel type for JXL");
        }

        return fmt;
    }

    static void PreparePackedPixels(
        PixelType pt,
        const void* ptr_data,
        uint32_t stride,
        uint32_t width,
        uint32_t height,
        vector<uint8_t>* packed,
        JxlPixelFormat* fmt)
    {
        const size_t row_b = MinStrideBytes(pt, width);
        const size_t total = row_b * height;
        packed->resize(total);
        *fmt = PixelFormatFor(pt);

        switch (pt)
        {
        case PixelType::Gray8:
        case PixelType::Gray16:
        case PixelType::Gray32Float:
            CopyRowsToPacked(ptr_data, stride, width, height, row_b, packed->data());
            break;
        case PixelType::Bgr24:
            Bgr24ToRgb24Packed(ptr_data, stride, width, height, packed->data());
            break;
        case PixelType::Bgr48:
            Bgr48ToRgb48Packed(ptr_data, stride, width, height, reinterpret_cast<uint16_t*>(packed->data()));
            break;
        case PixelType::Bgr96Float:
            Bgr96FloatToRgb96Packed(ptr_data, stride, width, height, reinterpret_cast<float*>(packed->data()));
            break;
        default:
            throw logic_error("unsupported pixel type for JXL");
        }
    }

    static void CheckJxlEnc(JxlEncoderStatus st, const char* what)
    {
        if (st != JXL_ENC_SUCCESS)
        {
            stringstream ss;
            ss << what << " failed with status " << static_cast<int>(st);
            throw runtime_error(ss.str());
        }
    }

    int ClampInt(int v, int lo, int hi)
    {
        if (v < lo)
        {
            return lo;
        }

        if (v > hi)
        {
            return hi;
        }

        return v;
    }

    int ReadEffort(const ICompressParameters* parameters)
    {
        CompressParameter c;
        if (parameters != nullptr && parameters->TryGetProperty(CompressionParameterKey::JXL_EFFORT, &c))
        {
            if (c.GetType() == CompressParameter::Type::Int32)
            {
                return ClampInt(c.GetInt32(), 1, 9);
            }

            if (c.GetType() == CompressParameter::Type::Uint32)
            {
                return ClampInt(static_cast<int>(c.GetUInt32()), 1, 9);
            }
        }

        return 7;
    }

    bool ReadModular(const ICompressParameters* parameters)
    {
        CompressParameter c;
        if (parameters != nullptr && parameters->TryGetProperty(CompressionParameterKey::JXL_MODULAR, &c)
            && c.GetType() == CompressParameter::Type::Boolean)
        {
            return c.GetBoolean();
        }

        return true;
    }

    bool TryReadClampedInt32(
        const ICompressParameters* parameters,
        CompressionParameterKey key,
        int lo,
        int hi,
        int* out)
    {
        CompressParameter c;
        if (parameters == nullptr || !parameters->TryGetProperty(key, &c))
        {
            return false;
        }

        if (c.GetType() == CompressParameter::Type::Int32)
        {
            *out = ClampInt(c.GetInt32(), lo, hi);
            return true;
        }

        if (c.GetType() == CompressParameter::Type::Uint32)
        {
            *out = ClampInt(static_cast<int>(c.GetUInt32()), lo, hi);
            return true;
        }

        return false;
    }

    void ApplyJxlFrameSettings(JxlEncoderFrameSettings* fs, const ICompressParameters* parameters)
    {
        const int effort = ReadEffort(parameters);
        CheckJxlEnc(JxlEncoderFrameSettingsSetOption(fs, JXL_ENC_FRAME_SETTING_EFFORT, effort), "JXL_ENC_FRAME_SETTING_EFFORT");

        const bool modular = ReadModular(parameters);
        CheckJxlEnc(
            JxlEncoderFrameSettingsSetOption(fs, JXL_ENC_FRAME_SETTING_MODULAR, modular ? 1 : 0),
            "JXL_ENC_FRAME_SETTING_MODULAR");

        int decodingSpeed;
        if (TryReadClampedInt32(parameters, CompressionParameterKey::JXL_DECODING_SPEED, 0, 4, &decodingSpeed))
        {
            CheckJxlEnc(
                JxlEncoderFrameSettingsSetOption(fs, JXL_ENC_FRAME_SETTING_DECODING_SPEED, decodingSpeed),
                "JXL_ENC_FRAME_SETTING_DECODING_SPEED");
        }

        int responsive;
        if (TryReadClampedInt32(parameters, CompressionParameterKey::JXL_RESPONSIVE, 0, 2, &responsive))
        {
            CheckJxlEnc(
                JxlEncoderFrameSettingsSetOption(fs, JXL_ENC_FRAME_SETTING_RESPONSIVE, responsive),
                "JXL_ENC_FRAME_SETTING_RESPONSIVE");
        }
    }
}

/*static*/ std::shared_ptr<IMemoryBlock> JxlLibCompress::Compress(
    PixelType pixel_type,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    const void* ptr_data,
    const ICompressParameters* parameters)
{
    ThrowIfBigEndianHost();

    switch (pixel_type)
    {
    case PixelType::Gray8:
    case PixelType::Gray16:
    case PixelType::Gray32Float:
    case PixelType::Bgr24:
    case PixelType::Bgr48:
    case PixelType::Bgr96Float:
        break;
    default:
        throw logic_error("unsupported pixel type for JXL");
    }

    JxlBasicInfo basic_info;
    FillBasicInfoForPixelType(pixel_type, width, height, &basic_info);

    JxlColorEncoding color_encoding;
    memset(&color_encoding, 0, sizeof(color_encoding));
    SetColorEncodingForPixelType(pixel_type, &color_encoding);

    vector<uint8_t> packed;
    JxlPixelFormat pixel_format;
    PreparePackedPixels(pixel_type, ptr_data, stride, width, height, &packed, &pixel_format);

    unique_ptr<JxlEncoder, void (*)(JxlEncoder*)> enc(JxlEncoderCreate(nullptr), JxlEncoderDestroy);
    if (enc == nullptr)
    {
        throw runtime_error("JxlEncoderCreate failed");
    }

    CheckJxlEnc(JxlEncoderUseContainer(enc.get(), JXL_FALSE), "JxlEncoderUseContainer");
    CheckJxlEnc(JxlEncoderSetBasicInfo(enc.get(), &basic_info), "JxlEncoderSetBasicInfo");
    CheckJxlEnc(JxlEncoderSetColorEncoding(enc.get(), &color_encoding), "JxlEncoderSetColorEncoding");

    const int required_codestream_level = JxlEncoderGetRequiredCodestreamLevel(enc.get());
    if (required_codestream_level == -1)
    {
        throw runtime_error("JxlEncoderGetRequiredCodestreamLevel: no valid codestream level for this configuration");
    }

    if (required_codestream_level == 10)
    {
        CheckJxlEnc(JxlEncoderSetCodestreamLevel(enc.get(), 10), "JxlEncoderSetCodestreamLevel(10)");
    }
    else
    {
        CheckJxlEnc(JxlEncoderSetCodestreamLevel(enc.get(), 5), "JxlEncoderSetCodestreamLevel(5)");
    }

    JxlEncoderFrameSettings* fs = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);
    if (fs == nullptr)
    {
        throw runtime_error("JxlEncoderFrameSettingsCreate failed");
    }

    CheckJxlEnc(JxlEncoderSetFrameLossless(fs, JXL_TRUE), "JxlEncoderSetFrameLossless");
    ApplyJxlFrameSettings(fs, parameters);

    const size_t frame_size = packed.size();
    CheckJxlEnc(JxlEncoderAddImageFrame(fs, &pixel_format, packed.data(), frame_size), "JxlEncoderAddImageFrame");

    JxlEncoderCloseInput(enc.get());

    vector<uint8_t> compressed(4096);
    size_t used = 0;
    for (;;)
    {
        uint8_t* next = compressed.data() + used;
        size_t avail = compressed.size() - used;
        if (avail < 32)
        {
            compressed.resize(max(compressed.size() * 2, used + 4096));
            continue;
        }

        JxlEncoderStatus pr = JxlEncoderProcessOutput(enc.get(), &next, &avail);
        used = static_cast<size_t>(next - compressed.data());
        if (pr == JXL_ENC_NEED_MORE_OUTPUT)
        {
            compressed.resize(max(compressed.size() * 2, used + 4096));
            continue;
        }

        if (pr != JXL_ENC_SUCCESS)
        {
            stringstream ss;
            ss << "JxlEncoderProcessOutput failed: " << static_cast<int>(pr);
            throw runtime_error(ss.str());
        }

        break;
    }

    compressed.resize(used);
    return make_shared<MemoryBlockOnVector>(std::move(compressed));
}

#endif
