// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_compress.h"
#include <turbojpeg.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace libCZI;

namespace
{
    using JpegBuffer = std::unique_ptr<void, decltype(&tj3Free)>;

    class JpegMemoryBlock : public IMemoryBlock
    {
    public:
        JpegMemoryBlock(JpegBuffer data, size_t size)
            : data_(std::move(data)), size_(size)
        {
        }

        void* GetPtr() override { return this->data_.get(); }
        size_t GetSizeOfData() const override { return this->size_; }

    private:
        JpegBuffer data_;
        size_t size_;
    };

    int GetJpegSubsampling(std::int32_t subsampling)
    {
        switch (subsampling)
        {
        case 444: return TJSAMP_444;
        case 422: return TJSAMP_422;
        case 420: return TJSAMP_420;
        }

        throw std::invalid_argument("Invalid JPEG chroma subsampling");
    }
}

std::shared_ptr<IMemoryBlock> JpgCompress::Compress(
    PixelType pixel_type,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t stride,
    const void* ptrData,
    const ICompressParameters* parameters)
{
    if (pixel_type != PixelType::Gray8 && pixel_type != PixelType::Bgr24)
    {
        throw std::invalid_argument("JPEG compression requires Gray8 or Bgr24");
    }

    // JPEG stores dimensions in 16-bit fields.
    constexpr std::uint32_t max_jpeg_dimension = 65535;
    if (ptrData == nullptr || width == 0 || height == 0 ||
        width > max_jpeg_dimension || height > max_jpeg_dimension)
    {
        throw std::invalid_argument("Invalid JPEG source bitmap or dimensions");
    }

    const bool grayscale = pixel_type == PixelType::Gray8;
    const int format = grayscale ? TJPF_GRAY : TJPF_BGR;
    const std::uint32_t row_bytes = width * tjPixelSize[format];
    if (stride < row_bytes || stride > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
        static_cast<std::uint64_t>(stride) * (height - 1) + row_bytes > (std::numeric_limits<size_t>::max)())
    {
        throw std::invalid_argument("Invalid source stride for JPEG compression");
    }

    int quality = 90;
    int subsampling = grayscale ? TJSAMP_GRAY : TJSAMP_444;
    bool optimize = false;
    if (parameters != nullptr)
    {
        CompressParameter parameter;
        if (parameters->TryGetProperty(CompressionParameterKey::JPG_QUALITY, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Uint32)
        {
            quality = static_cast<int>((std::max)(1u, (std::min)(100u, parameter.GetUInt32())));
        }

        if (!grayscale && parameters->TryGetProperty(CompressionParameterKey::JPG_SUBSAMP, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Int32)
        {
            subsampling = GetJpegSubsampling(parameter.GetInt32());
        }

        if (parameters->TryGetProperty(CompressionParameterKey::JPG_OPTIMIZE, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Boolean)
        {
            optimize = parameter.GetBoolean();
        }
    }

    const std::unique_ptr<void, decltype(&tj3Destroy)> encoder(tj3Init(TJINIT_COMPRESS), tj3Destroy);
    if (!encoder)
    {
        throw std::runtime_error(tj3GetErrorStr(nullptr));
    }

    if (tj3Set(encoder.get(), TJPARAM_QUALITY, quality) != 0 ||
        tj3Set(encoder.get(), TJPARAM_SUBSAMP, subsampling) != 0 ||
        tj3Set(encoder.get(), TJPARAM_OPTIMIZE, optimize) != 0)
    {
        throw std::runtime_error(tj3GetErrorStr(encoder.get()));
    }

    unsigned char* output = nullptr;
    size_t output_size = 0;
    const int result = tj3Compress8(encoder.get(), static_cast<const unsigned char*>(ptrData),
                                   static_cast<int>(width), static_cast<int>(stride), static_cast<int>(height),
                                   format, &output, &output_size);
    JpegBuffer buffer(output, tj3Free);
    if (result != 0)
    {
        throw std::runtime_error(tj3GetErrorStr(encoder.get()));
    }

    return std::make_shared<JpegMemoryBlock>(std::move(buffer), output_size);
}
