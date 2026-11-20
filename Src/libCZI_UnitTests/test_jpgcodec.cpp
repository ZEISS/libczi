// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "testImage.h"
#include "libCZI_Config_Internal.h"
#include "utils.h"
#include <algorithm>
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

TEST_P(JpgCodec, CompressAndDecodeGray8)
{
    constexpr std::uint32_t width = 3;
    constexpr std::uint32_t height = 2;
    constexpr std::uint32_t stride = 4;
    const std::uint8_t source[] = { 48, 48, 48, 0, 192, 192, 192, 0 };
    const auto jpeg = JpgCompress::Compress(PixelType::Gray8, width, height, stride, source);
    const auto bitmap = decoder->Decode(jpeg->GetPtr(), jpeg->GetSizeOfData(), PixelType::Gray8, width, height);
    const ScopedBitmapLockerSP lock(bitmap);
    const auto* pixels = static_cast<const std::uint8_t*>(lock.ptrDataRoi);
    EXPECT_NEAR(pixels[width - 1], 48, 5);
    EXPECT_NEAR(pixels[lock.stride + width - 1], 192, 5);
}

TEST_P(JpgCodec, CompressAndDecodeBgr24)
{
    constexpr std::uint32_t width = 2;
    constexpr std::uint32_t height = 2;
    constexpr std::uint32_t stride = 8;
    const std::uint8_t source[] = { 35, 80, 210, 35, 80, 210, 0, 0, 35, 80, 210, 35, 80, 210, 0, 0 };
    CompressParametersOnMap parameters;
    parameters.map.emplace(static_cast<int>(CompressionParameterKey::JPG_QUALITY), CompressParameter(std::uint32_t{100}));
    const auto jpeg = JpgCompress::Compress(PixelType::Bgr24, width, height, stride, source, &parameters);
    const auto bitmap = decoder->Decode(jpeg->GetPtr(), jpeg->GetSizeOfData(), PixelType::Bgr24, width, height);
    const ScopedBitmapLockerSP lock(bitmap);
    const auto* pixel = static_cast<const std::uint8_t*>(lock.ptrDataRoi) + lock.stride + 3;
    EXPECT_NEAR(pixel[0], 35, 3);
    EXPECT_NEAR(pixel[1], 80, 3);
    EXPECT_NEAR(pixel[2], 210, 3);
}

TEST(JpgCompress, RejectInvalidSourceArguments)
{
    const std::uint8_t source[] = { 0, 0, 0 };
    EXPECT_THROW(JpgCompress::Compress(PixelType::Gray16, 1, 1, 2, source), std::invalid_argument);
    EXPECT_THROW(JpgCompress::Compress(PixelType::Gray8, 1, 1, 1, nullptr), std::invalid_argument);
    EXPECT_THROW(JpgCompress::Compress(PixelType::Gray8, 0, 1, 1, source), std::invalid_argument);
    EXPECT_THROW(JpgCompress::Compress(PixelType::Bgr24, 1, 1, 2, source), std::invalid_argument);
}

TEST(JpgCompress, WritesRequestedChromaSampling)
{
    const std::uint8_t source[] = { 35, 80, 210 };
    struct SamplingCase
    {
        std::int32_t mode;
        std::uint8_t luma_sampling;
    };
    const SamplingCase cases[] = {
        { 444, 0x11 },
        { 422, 0x21 },
        { 420, 0x22 },
    };

    for (const auto& test_case : cases)
    {
        CompressParametersOnMap parameters;
        parameters.map.emplace(static_cast<int>(CompressionParameterKey::JPG_SUBSAMP),
                               CompressParameter(test_case.mode));
        const auto jpeg = JpgCompress::Compress(PixelType::Bgr24, 1, 1, 3, source, &parameters);
        const auto* begin = static_cast<const std::uint8_t*>(jpeg->GetPtr());
        const auto* end = begin + jpeg->GetSizeOfData();
        // Baseline frame header through the first component's sampling byte.
        const std::uint8_t frame[] = { 0xff, 0xc0, 0, 17, 8, 0, 1, 0, 1, 3, 1, test_case.luma_sampling };
        EXPECT_NE(std::search(begin, end, frame, frame + sizeof(frame)), end);
    }
}

TEST(JpgCompress, OptimizesHuffmanTablesWithoutChangingPixels)
{
    const std::uint8_t source[] = { 128 };
    CompressParametersOnMap parameters;
    parameters.map.emplace(static_cast<int>(CompressionParameterKey::JPG_OPTIMIZE), CompressParameter(true));
    const auto standard = JpgCompress::Compress(PixelType::Gray8, 1, 1, 1, source);
    const auto optimized = JpgCompress::Compress(PixelType::Gray8, 1, 1, 1, source, &parameters);
    EXPECT_LT(optimized->GetSizeOfData(), standard->GetSizeOfData());

    const auto decode = [](const std::shared_ptr<IMemoryBlock>& jpeg)
    {
        return CreateBitmapFromSubBlockData(CompressionMode::Jpg, jpeg->GetPtr(), jpeg->GetSizeOfData(),
                                            PixelType::Gray8, 1, 1);
    };
    EXPECT_TRUE(AreBitmapDataEqual(decode(standard), decode(optimized)));
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
