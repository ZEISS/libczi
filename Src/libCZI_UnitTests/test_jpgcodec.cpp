// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "testImage.h"
#include "libCZI_Config_Internal.h"
#include <cstdint>
#include <stdexcept>

using namespace libCZI;

namespace
{
    class JpgCodec : public ::testing::TestWithParam<SiteObjectType>
    {
    protected:
        std::shared_ptr<IDecoder> decoder;

        void SetUp() override
        {
            auto* site = GetDefaultSiteObject(GetParam());
            ASSERT_NE(site, nullptr);
            decoder = site->GetDecoder(ImageDecoderType::Jpg, nullptr);
            ASSERT_NE(decoder, nullptr);
        }
    };

    INSTANTIATE_TEST_SUITE_P(TurboJpeg, JpgCodec, ::testing::Values(SiteObjectType::Default));

#if LIBCZI_WINDOWSAPI_AVAILABLE
    INSTANTIATE_TEST_SUITE_P(Wic, JpgCodec, ::testing::Values(SiteObjectType::WithWICDecoder));
#endif
}

TEST_P(JpgCodec, DecodeBgr24)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Bgr24(&size, &width, &height);
    const auto bitmap = decoder->Decode(data, size, PixelType::Bgr24, width, height);
    ASSERT_EQ(bitmap->GetWidth(), static_cast<std::uint32_t>(width));
    ASSERT_EQ(bitmap->GetHeight(), static_cast<std::uint32_t>(height));
    ASSERT_EQ(bitmap->GetPixelType(), PixelType::Bgr24);

    const ScopedBitmapLockerSP lock(bitmap);
    const auto* pixels = static_cast<const std::uint8_t*>(lock.ptrDataRoi);
    // Sample different rows using the reported stride. A small tolerance
    // allows for JPEG rounding differences between decoder implementations.
    EXPECT_NEAR(pixels[0], 252, 5);
    const auto* color = pixels + 260 * lock.stride + 741 * 3;
    EXPECT_NEAR(color[0], 219, 5);
    EXPECT_NEAR(color[1], 200, 5);
    EXPECT_NEAR(color[2], 185, 5);
}

TEST_P(JpgCodec, DecodeGray8WithoutExpectedMetadata)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Gray8(&size, &width, &height);
    const auto bitmap = decoder->Decode(data, size, nullptr, nullptr, nullptr);
    ASSERT_EQ(bitmap->GetWidth(), static_cast<std::uint32_t>(width));
    ASSERT_EQ(bitmap->GetHeight(), static_cast<std::uint32_t>(height));
    ASSERT_EQ(bitmap->GetPixelType(), PixelType::Gray8);

    const ScopedBitmapLockerSP lock(bitmap);
    const auto* pixels = static_cast<const std::uint8_t*>(lock.ptrDataRoi);
    for (std::uint32_t y = 0; y < bitmap->GetHeight(); ++y)
    {
        for (std::uint32_t x = 0; x < bitmap->GetWidth(); ++x)
        {
            EXPECT_EQ(pixels[y * lock.stride + x], 128);
        }
    }
}

TEST_P(JpgCodec, RejectInvalidData)
{
    const std::uint8_t invalid[] = { 1, 2, 3, 4 };
    EXPECT_THROW(decoder->Decode(invalid, sizeof(invalid), nullptr, nullptr, nullptr), std::runtime_error);
}

TEST_P(JpgCodec, RejectMetadataMismatches)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Gray8(&size, &width, &height);
    EXPECT_THROW(decoder->Decode(data, size, PixelType::Bgr24, width, height), std::logic_error);
    EXPECT_THROW(decoder->Decode(data, size, PixelType::Gray8, width + 1, height), std::logic_error);
    EXPECT_THROW(decoder->Decode(data, size, PixelType::Gray8, width, height + 1), std::logic_error);
}

TEST(JpgBitmap, RejectTruncatedJpegWithDefaultDecoder)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Gray8(&size, &width, &height);
    // Omit the end-of-image marker. TurboJPEG reports the truncation as an error.
    EXPECT_THROW(CreateBitmapFromSubBlockData(CompressionMode::Jpg, data, size - 2,
                                            PixelType::Gray8, width, height), std::runtime_error);
}

TEST(JpgBitmap, CropPadAndConvertToDeclaredPixelType)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Gray8(&size, &width, &height);
    const auto output_width = static_cast<std::uint32_t>(width + 1);
    const auto output_height = static_cast<std::uint32_t>(height - 1);
    const auto bitmap = CreateBitmapFromSubBlockData(CompressionMode::Jpg, data, size,
                                                     PixelType::Bgr24, output_width, output_height);
    ASSERT_EQ(bitmap->GetWidth(), output_width);
    ASSERT_EQ(bitmap->GetHeight(), output_height);
    ASSERT_EQ(bitmap->GetPixelType(), PixelType::Bgr24);

    const ScopedBitmapLockerSP lock(bitmap);
    const auto* pixels = static_cast<const std::uint8_t*>(lock.ptrDataRoi);
    const auto source_row_bytes = static_cast<std::uint32_t>(width) * 3;
    for (std::uint32_t y = 0; y < output_height; ++y)
    {
        const auto* row = pixels + y * lock.stride;
        for (std::uint32_t byte = 0; byte < source_row_bytes; ++byte)
        {
            EXPECT_EQ(row[byte], 128);
        }

        EXPECT_EQ(row[source_row_bytes], 0);
        EXPECT_EQ(row[source_row_bytes + 1], 0);
        EXPECT_EQ(row[source_row_bytes + 2], 0);
    }
}

TEST(JpgBitmap, RejectMetadataMismatchesInStrictMode)
{
    size_t size;
    int width, height;
    const auto data = CTestImage::GetJpgCompressedImage_Gray8(&size, &width, &height);
    CreateBitmapOptions options;
    options.handle_jpg_bitmap_mismatch = false;
    EXPECT_THROW(CreateBitmapFromSubBlockData(CompressionMode::Jpg, data, size,
                                            PixelType::Bgr24, width, height, &options), std::logic_error);
    EXPECT_THROW(CreateBitmapFromSubBlockData(CompressionMode::Jpg, data, size,
                                            PixelType::Gray8, width + 1, height, &options), std::logic_error);
}
