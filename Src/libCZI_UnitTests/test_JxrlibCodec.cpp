// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include <cstdint>
#include <cstdlib>
#include  <array>
#include <memory>
#include "inc_libCZI.h"
#include "testImage.h"
#include "utils.h"
#include "../libCZI/decoder.h"

using namespace libCZI;
using namespace std;

TEST(JxrlibCodec, DecodeBgr24)
{
    const auto dec = CJxrLibDecoder::Create();
    size_t sizeEncoded; int expectedWidth, expectedHeight;
    const auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage_Bgr24(&sizeEncoded, &expectedWidth, &expectedHeight);
    const auto bmDecoded = dec->Decode(ptrEncodedData, sizeEncoded, libCZI::PixelType::Bgr24, expectedWidth, expectedHeight);
    EXPECT_EQ((uint32_t)expectedWidth, bmDecoded->GetWidth()) << "Width is expected to be equal";
    EXPECT_EQ((uint32_t)expectedHeight, bmDecoded->GetHeight()) << "Height is expected to be equal";
    EXPECT_EQ(bmDecoded->GetPixelType(), PixelType::Bgr24) << "Not the correct pixeltype.";

    uint8_t hash[16] = { 0 };
    Utils::CalcMd5SumHash(bmDecoded.get(), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x04,0x77,0x2f,0x32,0x2f,0x94,0x9b,0x07,0x0d,0x53,0xa5,0x24,0xea,0x64,0x5a,0x1a };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}

TEST(JxrlibCodec, DecodeGray8)
{
    const auto dec = CJxrLibDecoder::Create();
    size_t sizeEncoded; int expectedWidth, expectedHeight;
    const auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage_Gray8(&sizeEncoded, &expectedWidth, &expectedHeight);
    const auto bmDecoded = dec->Decode(ptrEncodedData, sizeEncoded, libCZI::PixelType::Gray8, expectedWidth, expectedHeight);
    EXPECT_EQ(static_cast<uint32_t>(expectedWidth), bmDecoded->GetWidth()) << "Width is expected to be equal";
    EXPECT_EQ(static_cast<uint32_t>(expectedHeight), bmDecoded->GetHeight()) << "Height is expected to be equal";
    EXPECT_EQ(bmDecoded->GetPixelType(), PixelType::Gray8) << "Not the correct pixeltype.";

    uint8_t hash[16] = { 0 };
    Utils::CalcMd5SumHash(bmDecoded.get(), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x95, 0x4c, 0x70, 0x70, 0xae, 0xfb, 0x63, 0xc6, 0xc4, 0x0a, 0xb5, 0xec, 0xef, 0x73, 0x09, 0x8d };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}

TEST(JxrlibCodec, TryDecodeInvalidDataExpectException)
{
    // pass invalid data to decoder, and expect an exception
    const auto dec = CJxrLibDecoder::Create();
    constexpr size_t sizeEncoded = 2345;
    const unique_ptr<uint8_t[]> encoded_data(new uint8_t[sizeEncoded]);
    for (size_t i = 0; i < sizeEncoded; i++)
    {
        encoded_data.get()[i] = static_cast<uint8_t>(i);
    }

    EXPECT_ANY_THROW(dec->Decode(encoded_data.get(), sizeEncoded, libCZI::PixelType::Invalid, 0, 0));
}

TEST(JxrlibCodec, CompressNonLossyAndDecompressCheckForSameContent_Bgr24)
{
    const auto bitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const ScopedBitmapLockerSP lck{ bitmap };
        CTestImage::CopyBgr24Image(lck.ptrDataRoi, bitmap->GetWidth(), bitmap->GetHeight(), lck.stride);
    }
    
    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap };
        encodedData = JxrLibCompress::Compress(
            bitmap->GetPixelType(),
            bitmap->GetWidth(),
            bitmap->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            nullptr);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap->GetPixelType()) * bitmap->GetWidth() * bitmap->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Bgr24,
        bitmap->GetWidth(),
        bitmap->GetHeight());
    const bool are_equal = AreBitmapDataEqual(bitmap, bitmap_decoded);
    EXPECT_TRUE(are_equal) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CompressNonLossyAndDecompressCheckForSameContent_Gray8)
{
    const auto bitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const auto bitmap_bgr24 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
        const ScopedBitmapLockerSP locked_bgr24{ bitmap_bgr24 };
        const ScopedBitmapLockerSP locked_gray8{ bitmap };
        CTestImage::CopyBgr24Image(locked_bgr24.ptrDataRoi, bitmap_bgr24->GetWidth(), bitmap_bgr24->GetHeight(), locked_bgr24.stride);
        for (size_t y = 0; y < bitmap_bgr24->GetHeight(); ++y)
        {
            for (size_t x = 0; x < bitmap_bgr24->GetWidth(); ++x)
            {
                const auto bgr24_pixel = &static_cast<const uint8_t*>(locked_bgr24.ptrDataRoi)[y * locked_bgr24.stride + x * 3];
                const auto gray8_pixel = &static_cast<uint8_t*>(locked_gray8.ptrDataRoi)[y * locked_gray8.stride + x];
                *gray8_pixel = static_cast<uint8_t>(bgr24_pixel[0] * 0.299 + bgr24_pixel[1] * 0.587 + bgr24_pixel[2] * 0.114);
            }
        }
    }

    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap };
        encodedData = JxrLibCompress::Compress(
            bitmap->GetPixelType(),
            bitmap->GetWidth(),
            bitmap->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            nullptr);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap->GetPixelType()) * bitmap->GetWidth() * bitmap->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Gray8,
        bitmap->GetWidth(),
        bitmap->GetHeight());
    const bool are_equal = AreBitmapDataEqual(bitmap, bitmap_decoded);
    EXPECT_TRUE(are_equal) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CompressNonLossyAndDecompressCheckForSameContent_Gray16)
{
    const auto bitmap_gray16 = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const auto bitmap_bgr24 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
        const ScopedBitmapLockerSP locked_bgr24{ bitmap_bgr24 };
        const ScopedBitmapLockerSP locked_gray16{ bitmap_gray16 };
        CTestImage::CopyBgr24Image(locked_bgr24.ptrDataRoi, bitmap_bgr24->GetWidth(), bitmap_bgr24->GetHeight(), locked_bgr24.stride);
        for (size_t y = 0; y < bitmap_bgr24->GetHeight(); ++y)
        {
            for (size_t x = 0; x < bitmap_bgr24->GetWidth(); ++x)
            {
                const auto bgr24_pixel = &static_cast<const uint8_t*>(locked_bgr24.ptrDataRoi)[y * locked_bgr24.stride + x * 3];
                const auto gray16_pixel = reinterpret_cast<uint16_t*>(&static_cast<uint8_t*>(locked_gray16.ptrDataRoi)[y * locked_gray16.stride + 2 * x]);
                *gray16_pixel = static_cast<uint16_t>(
                    (static_cast<uint16_t>(bgr24_pixel[0]) << 8) * 0.299 +
                    (static_cast<uint16_t>(bgr24_pixel[1]) << 8) * 0.587 +
                    (static_cast<uint16_t>(bgr24_pixel[2]) << 8) * 0.114);
            }
        }
    }

    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap_gray16 };
        encodedData = JxrLibCompress::Compress(
            bitmap_gray16->GetPixelType(),
            bitmap_gray16->GetWidth(),
            bitmap_gray16->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            nullptr);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap_gray16->GetPixelType()) * bitmap_gray16->GetWidth() * bitmap_gray16->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Gray16,
        bitmap_gray16->GetWidth(),
        bitmap_gray16->GetHeight());
    const bool are_equal = AreBitmapDataEqual(bitmap_gray16, bitmap_decoded);
    EXPECT_TRUE(are_equal) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CompressNonLossyAndDecompressCheckForSameContent_Gray32Float)
{
    const auto bitmap_gray32float = CBitmapData<CHeapAllocator>::Create(PixelType::Gray32Float, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const auto bitmap_bgr24 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
        const ScopedBitmapLockerSP locked_bgr24{ bitmap_bgr24 };
        const ScopedBitmapLockerSP locked_gray16{ bitmap_gray32float };
        CTestImage::CopyBgr24Image(locked_bgr24.ptrDataRoi, bitmap_bgr24->GetWidth(), bitmap_bgr24->GetHeight(), locked_bgr24.stride);
        for (size_t y = 0; y < bitmap_bgr24->GetHeight(); ++y)
        {
            for (size_t x = 0; x < bitmap_bgr24->GetWidth(); ++x)
            {
                const auto bgr24_pixel = &static_cast<const uint8_t*>(locked_bgr24.ptrDataRoi)[y * locked_bgr24.stride + x * 3];
                const auto gray32float_pixel = reinterpret_cast<float*>(&static_cast<uint8_t*>(locked_gray16.ptrDataRoi)[y * locked_gray16.stride + 4 * x]);
                *gray32float_pixel =
                    bgr24_pixel[0] * 0.299f +
                    bgr24_pixel[1] * 0.587f +
                    bgr24_pixel[2] * 0.114f;
            }
        }
    }

    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap_gray32float };
        encodedData = JxrLibCompress::Compress(
            bitmap_gray32float->GetPixelType(),
            bitmap_gray32float->GetWidth(),
            bitmap_gray32float->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            nullptr);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap_gray32float->GetPixelType()) * bitmap_gray32float->GetWidth() * bitmap_gray32float->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Gray32Float,
        bitmap_gray32float->GetWidth(),
        bitmap_gray32float->GetHeight());
    const bool are_equal = CompareGrayFloat32Bitmaps(bitmap_gray32float, bitmap_decoded, 0.01f);
    EXPECT_TRUE(are_equal) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CompressNonLossyAndDecompressCheckForSameContent_Bgr48)
{
    const auto bitmap_bgr48 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr48, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const auto bitmap_bgr24 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
        const ScopedBitmapLockerSP locked_bgr24{ bitmap_bgr24 };
        const ScopedBitmapLockerSP locked_bgr48{ bitmap_bgr48 };
        CTestImage::CopyBgr24Image(locked_bgr24.ptrDataRoi, bitmap_bgr24->GetWidth(), bitmap_bgr24->GetHeight(), locked_bgr24.stride);

        for (size_t y = 0; y < bitmap_bgr48->GetHeight(); ++y)
        {
            for (size_t x = 0; x < bitmap_bgr48->GetWidth(); ++x)
            {
                const auto bgr24_pixel = &static_cast<const uint8_t*>(locked_bgr24.ptrDataRoi)[y * locked_bgr24.stride + x * 3];
                const auto bgr48_pixel = reinterpret_cast<uint16_t*>(&static_cast<uint8_t*>(locked_bgr48.ptrDataRoi)[y * locked_bgr48.stride + 6 * x]);

                // we fill the lower 8 bits with a random byte
                bgr48_pixel[0] = static_cast<uint16_t>((static_cast<uint16_t>(bgr24_pixel[0]) << 8) | static_cast<uint8_t>(rand() & 0xff));
                bgr48_pixel[1] = static_cast<uint16_t>((static_cast<uint16_t>(bgr24_pixel[1]) << 8) | static_cast<uint8_t>(rand() & 0xff));
                bgr48_pixel[2] = static_cast<uint16_t>((static_cast<uint16_t>(bgr24_pixel[2]) << 8) | static_cast<uint8_t>(rand() & 0xff));
            }
        }
    }

    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap_bgr48 };
        encodedData = JxrLibCompress::Compress(
            bitmap_bgr48->GetPixelType(),
            bitmap_bgr48->GetWidth(),
            bitmap_bgr48->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            nullptr);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap_bgr48->GetPixelType()) * bitmap_bgr48->GetWidth() * bitmap_bgr48->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Bgr48,
        bitmap_bgr48->GetWidth(),
        bitmap_bgr48->GetHeight());
    const bool are_equal = AreBitmapDataEqual(bitmap_bgr48, bitmap_decoded);
    EXPECT_TRUE(are_equal) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CompressLossyAndDecompressCheckForSimilarity_Gray8)
{
    const auto bitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        const auto bitmap_bgr24 = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
        const ScopedBitmapLockerSP locked_bgr24{ bitmap_bgr24 };
        const ScopedBitmapLockerSP locked_gray8{ bitmap };
        CTestImage::CopyBgr24Image(locked_bgr24.ptrDataRoi, bitmap_bgr24->GetWidth(), bitmap_bgr24->GetHeight(), locked_bgr24.stride);
        for (size_t y = 0; y < bitmap_bgr24->GetHeight(); ++y)
        {
            for (size_t x = 0; x < bitmap_bgr24->GetWidth(); ++x)
            {
                const auto bgr24_pixel = &static_cast<const uint8_t*>(locked_bgr24.ptrDataRoi)[y * locked_bgr24.stride + x * 3];
                const auto gray8_pixel = &static_cast<uint8_t*>(locked_gray8.ptrDataRoi)[y * locked_gray8.stride + x];
                *gray8_pixel = static_cast<uint8_t>(bgr24_pixel[0] * 0.299 + bgr24_pixel[1] * 0.587 + bgr24_pixel[2] * 0.114);
            }
        }
    }

    shared_ptr<libCZI::IMemoryBlock> encodedData;

    {
        const ScopedBitmapLockerSP lck{ bitmap };
        libCZI::CompressParametersOnMap params;
        params.map[static_cast<int>(libCZI::CompressionParameterKey::JXRLIB_QUALITY)] = libCZI::CompressParameter(900u);
        encodedData = JxrLibCompress::Compress(
            bitmap->GetPixelType(),
            bitmap->GetWidth(),
            bitmap->GetHeight(),
            lck.stride,
            lck.ptrDataRoi,
            &params);
    }

    void* encoded_data_ptr = encodedData->GetPtr();
    ASSERT_NE(encoded_data_ptr, nullptr) << "Encoded data is null.";
    const size_t size_of_encoded_data = encodedData->GetSizeOfData();
    ASSERT_LT(size_of_encoded_data, static_cast<size_t>(Utils::GetBytesPerPixel(bitmap->GetPixelType()) * bitmap->GetWidth() * bitmap->GetHeight())) <<
        "Encoded data is too large (larger than the original data), which is unexpected.";

    /*
    FILE* fp = fopen("N:\\test.jxr", "wb");
    fwrite(encoded_data_ptr, size_of_encoded_data, 1, fp);
    fclose(fp);
    */

    const auto codec = CJxrLibDecoder::Create();
    const auto bitmap_decoded = codec->Decode(
        encodedData->GetPtr(),
        encodedData->GetSizeOfData(),
        libCZI::PixelType::Gray8,
        bitmap->GetWidth(),
        bitmap->GetHeight());
    const auto max_difference_mean_difference = CalculateMaxDifferenceMeanDifference(bitmap, bitmap_decoded);
    EXPECT_TRUE(get<0>(max_difference_mean_difference) <= 5 && get<1>(max_difference_mean_difference) < 1) << "Original bitmap and encoded-decoded one are not identical.";
}

TEST(JxrlibCodec, CallEncoderWithInvalidArgumentsExpectException)
{
    const auto bitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 5, 5);
    //const auto codec = CJxrLibDecoder::Create();
    EXPECT_THROW(
        {
            const ScopedBitmapLockerSP lck{ bitmap };
            JxrLibCompress::Compress(
                PixelType::Invalid,
                bitmap->GetWidth(),
                bitmap->GetHeight(),
                lck.stride,
                lck.ptrDataRoi,
                nullptr);
        },
        exception);
    EXPECT_THROW(
        {
            const ScopedBitmapLockerSP lck{ bitmap };
            JxrLibCompress::Compress(
                bitmap->GetPixelType(),
                bitmap->GetWidth(),
                bitmap->GetHeight(),
                4,                      // invalid stride provided here
                lck.ptrDataRoi,
                nullptr);
        },
        exception);
    EXPECT_THROW(
        {
            const ScopedBitmapLockerSP lck{ bitmap };
            JxrLibCompress::Compress(
                bitmap->GetPixelType(),
                bitmap->GetWidth(),
                0,
                lck.stride,
                lck.ptrDataRoi,
                nullptr);
        },
        exception);
    EXPECT_THROW(
        {
            const ScopedBitmapLockerSP lck{ bitmap };
            JxrLibCompress::Compress(
                bitmap->GetPixelType(),
                0,
                bitmap->GetHeight(),
                lck.stride,
                lck.ptrDataRoi,
                nullptr);
        },
        exception);
    EXPECT_THROW(
        {
            const ScopedBitmapLockerSP lck{ bitmap };
            JxrLibCompress::Compress(
                bitmap->GetPixelType(),
                bitmap->GetWidth(),
                bitmap->GetHeight(),
                lck.stride,
                nullptr,
                nullptr);
        },
        exception);
}

TEST(JxrlibCodec, CallDecoderWithInvalidArgumentsExpectException)
{
    constexpr size_t kSizeOfEncodedData = 223;
    const unique_ptr<void, decltype(&free)> encoded_data{ malloc(kSizeOfEncodedData), &free };
    for (size_t i = 0; i < kSizeOfEncodedData; ++i)
    {
        static_cast<uint8_t*>(encoded_data.get())[i] = static_cast<uint8_t>(i);
    }

    const auto codec = CJxrLibDecoder::Create();
    EXPECT_THROW(
        {
            codec->Decode(
                encoded_data.get(),
                kSizeOfEncodedData,
                libCZI::PixelType::Gray8,
                42,
                42);
        },
        exception);
    EXPECT_THROW(
        {
            codec->Decode(
                nullptr,
                kSizeOfEncodedData,
                libCZI::PixelType::Gray8,
                42,
                42);
        },
        exception);
    EXPECT_THROW(
        {
            codec->Decode(
                encoded_data.get(),
                0,
                libCZI::PixelType::Gray8,
                42,
                42);
        },
        exception);
}

TEST(JxrlibCodec, CallDecoderExpectingADifferentBitmapTypeAndExpectException)
{
    size_t size_encoded_data;
    int expected_width, expected_height;
    const auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage_Bgr24(&size_encoded_data, &expected_width, &expected_height);
    const auto codec = CJxrLibDecoder::Create();

    static constexpr array<PixelType, 4> kPixelTypesToTest = { PixelType::Gray8, PixelType::Gray16, PixelType::Bgr48, PixelType::Gray32Float };

    for (const auto pixel_type : kPixelTypesToTest)
    {
        EXPECT_THROW(
            {
                codec->Decode(
                    ptrEncodedData,
                    size_encoded_data,
                    pixel_type,
                    expected_width,
                    expected_height);
            },
            exception);
    }
}
