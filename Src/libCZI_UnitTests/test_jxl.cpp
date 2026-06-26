// SPDX-FileCopyrightText: 2017-2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/bitmapData.h"
#include "../libCZI/decoder_jxl.h"
#include "utils.h"

#include <cmath>
#include <cstring>
#include <memory>

using namespace libCZI;
using namespace libCZI::detail;

namespace
{
    void RoundTripLossless(PixelType pt, uint32_t w, uint32_t h)
    {
        std::shared_ptr<IBitmapData> img = CreateRandomBitmap(pt, w, h);
        ScopedBitmapLockerSP lock_in{ img };
        std::shared_ptr<IMemoryBlock> compressed = JxlLibCompress::Compress(
            pt,
            w,
            h,
            lock_in.stride,
            lock_in.ptrDataRoi,
            nullptr);
        ASSERT_TRUE(compressed != nullptr);
        ASSERT_GT(compressed->GetSizeOfData(), 0u);

        std::shared_ptr<CJxlDecoder> dec = CJxlDecoder::Create();
        std::shared_ptr<IBitmapData> out = dec->Decode(
            compressed->GetPtr(),
            compressed->GetSizeOfData(),
            pt,
            w,
            h);

        EXPECT_TRUE(AreBitmapDataEqual(img, out)) << "Pixel-perfect roundtrip failed for pixel type " << static_cast<int>(pt);
    }

    std::shared_ptr<IBitmapData> MakeFloatBitmapGray(uint32_t w, uint32_t h)
    {
        auto bm = CStdBitmapData::Create(PixelType::Gray32Float, w, h);
        ScopedBitmapLockerSP lck{ bm };
        for (uint32_t y = 0; y < h; ++y)
        {
            float* row = reinterpret_cast<float*>(static_cast<uint8_t*>(lck.ptrDataRoi) + static_cast<size_t>(y) * lck.stride);
            for (uint32_t x = 0; x < w; ++x)
            {
                row[x] = static_cast<float>(std::sin(static_cast<double>(x + y * w)) * 0.25f);
            }
        }

        return bm;
    }

    std::shared_ptr<IBitmapData> MakeFloatBitmapBgr(uint32_t w, uint32_t h)
    {
        auto bm = CStdBitmapData::Create(PixelType::Bgr96Float, w, h);
        ScopedBitmapLockerSP lck{ bm };
        for (uint32_t y = 0; y < h; ++y)
        {
            float* row = reinterpret_cast<float*>(static_cast<uint8_t*>(lck.ptrDataRoi) + static_cast<size_t>(y) * lck.stride);
            for (uint32_t x = 0; x < w; ++x)
            {
                row[x * 3 + 0] = static_cast<float>(y) / static_cast<float>(h + 1);
                row[x * 3 + 1] = static_cast<float>(x) / static_cast<float>(w + 1);
                row[x * 3 + 2] = static_cast<float>((x ^ y) & 255) / 255.0f;
            }
        }

        return bm;
    }

    void RoundTripLosslessBitmap(const std::shared_ptr<IBitmapData>& img)
    {
        const PixelType pt = img->GetPixelType();
        const uint32_t w = img->GetWidth();
        const uint32_t h = img->GetHeight();
        ScopedBitmapLockerSP lock_in{ img };
        std::shared_ptr<IMemoryBlock> compressed = JxlLibCompress::Compress(
            pt,
            w,
            h,
            lock_in.stride,
            lock_in.ptrDataRoi,
            nullptr);
        ASSERT_TRUE(compressed != nullptr);

        std::shared_ptr<CJxlDecoder> dec = CJxlDecoder::Create();
        std::shared_ptr<IBitmapData> out = dec->Decode(
            compressed->GetPtr(),
            compressed->GetSizeOfData(),
            pt,
            w,
            h);

        EXPECT_TRUE(AreBitmapDataEqual(img, out));
    }
}

TEST(JxlLossless, Gray8)
{
    RoundTripLossless(PixelType::Gray8, 64, 48);
}

TEST(JxlLossless, Gray16)
{
    RoundTripLossless(PixelType::Gray16, 55, 33);
}

TEST(JxlLossless, Bgr24)
{
    RoundTripLossless(PixelType::Bgr24, 61, 37);
}

TEST(JxlLossless, Bgr48)
{
    RoundTripLossless(PixelType::Bgr48, 29, 41);
}

TEST(JxlLossless, Gray32Float)
{
    RoundTripLosslessBitmap(MakeFloatBitmapGray(47, 39));
}

TEST(JxlLossless, Bgr96Float)
{
    RoundTripLosslessBitmap(MakeFloatBitmapBgr(31, 27));
}
