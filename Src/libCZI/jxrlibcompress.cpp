// SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_compress.h"
#include "../JxrDecode/JxrDecode.h"
#include "BitmapOperations.h"
#include "Site.h"

using namespace libCZI;
using namespace std;

class MemoryBlockOnCompressedData : public libCZI::IMemoryBlock
{
private:
    mutable JxrDecode::CompressedData compressed_data_;
public:
    MemoryBlockOnCompressedData(JxrDecode::CompressedData&& compressed_data)
        : compressed_data_(std::move(compressed_data))
    {}

    void* GetPtr() override
    {
        return this->compressed_data_.GetMemory();
    }

    size_t GetSizeOfData() const override
    {
        return this->compressed_data_.GetSize();
    }
};

/*static*/ std::shared_ptr<IMemoryBlock> JxrLibCompress::Compress(
        libCZI::PixelType pixel_type,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t stride,
        const void* ptrData,
        const ICompressParameters* parameters)
{
    JxrDecode::PixelFormat jxrdecode_pixel_format;
    switch (pixel_type)
    {
    case PixelType::Bgr24:
        jxrdecode_pixel_format = JxrDecode::PixelFormat::kBgr24;
        break;
    case PixelType::Bgr48:
        jxrdecode_pixel_format = JxrDecode::PixelFormat::kBgr48;
        break;
    case PixelType::Gray8:
        jxrdecode_pixel_format = JxrDecode::PixelFormat::kGray8;
        break;
    case PixelType::Gray16:
        jxrdecode_pixel_format = JxrDecode::PixelFormat::kGray16;
        break;
    case PixelType::Gray32Float:
        jxrdecode_pixel_format = JxrDecode::PixelFormat::kGray32Float;
        break;
    default:
        throw std::logic_error("unsupported pixel type");
    }

    float quality = 1.f;
    if (parameters != nullptr)
    {
        CompressParameter parameter;
        if (parameters->TryGetProperty(CompressionParameterKey::JXRLIB_QUALITY, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Uint32)
        {
            quality = parameter.GetUInt32() / 1000.0f;
            quality = std::max(0.0f, std::min(1.0f, quality));
        }
    }

    // Unfortunately, the encoder does not support the pixel format Bgr48, so we need to convert it to Rgb48
    //  before passing it to the encoder (meaning: the resulting encoded data will be Rgb48, not Bgr48).
    // TODO(JBL): would be nice if the encoder would support Bgr48 directly somehow
    if (jxrdecode_pixel_format == JxrDecode::PixelFormat::kBgr48)
    {
        // unfortunately, we have to make a temporary copy
        const auto bitmap_rgb48 = GetSite()->CreateBitmap(PixelType::Bgr48, width, height);

        const ScopedBitmapLockerSP bmLck(bitmap_rgb48);
        CBitmapOperations::CopySamePixelType<PixelType::Bgr48>(
            ptrData,
            stride,
            bmLck.ptrDataRoi,
            bmLck.stride,
            width,
            height,
            false);
        CBitmapOperations::RGB48ToBGR48(width, height, static_cast<uint16_t*>(bmLck.ptrDataRoi), bmLck.stride);
        auto compressed_data = JxrDecode::Encode(
                                    jxrdecode_pixel_format,
                                    width,
                                    height,
                                    bmLck.stride,
                                    bmLck.ptrDataRoi,
                                    quality);
        return make_shared<MemoryBlockOnCompressedData>(std::move(compressed_data));
    }
    else
    {
        auto compressed_data = JxrDecode::Encode(
            jxrdecode_pixel_format,
            width,
            height,
            stride,
            ptrData,
            quality);
        return make_shared<MemoryBlockOnCompressedData>(std::move(compressed_data));
    }
}
