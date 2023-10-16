// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "testImage.h"
#include "inc_libCZI.h"
#include "utils.h"

using namespace libCZI;

static std::shared_ptr<IBitmapData> CreateTestImage()
{
    auto bm = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    ScopedBitmapLockerSP lck{ bm };
    CTestImage::CopyBgr24Image(lck.ptrDataRoi, bm->GetWidth(), bm->GetHeight(), lck.stride);
    return bm;
}

TEST(BitmapOperations, NNResize1)
{
    static const std::uint8_t expectedResult[16] = { 0xfc,0xfc,0x14,0x65,0xb0,0xc7,0xe0,0x42,0x4e,0x5e,0x12,0xcb,0x30,0x64,0x30,0x1e };

    auto srcBm = CreateTestImage();
    auto bm = CBitmapData<CHeapAllocator>::Create(srcBm->GetPixelType(), 100, 100);
    CBitmapOperations::NNResize(srcBm.get(), bm.get());
    std::uint8_t hash[16];
    CBitmapOperations::CalcMd5Sum(bm.get(), hash, sizeof(hash));
    int c = memcmp(expectedResult, hash, sizeof(hash));
    EXPECT_EQ(c, 0);
}

TEST(BitmapOperations, NNResizeWithScale1CheckResult)
{
    // check that for "minification factor of 1" we get a 1:1 copy of the source bitmap
    auto sourcebitmap = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    auto destBitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, 163, 128);
    CBitmapOperations::NNResize(sourcebitmap.get(), destBitmap.get());
    EXPECT_TRUE(AreBitmapDataEqual(sourcebitmap, destBitmap)) << "Bitmaps are expected to be equal.";
}

TEST(BitmapOperations, CopyWithOffsetGray8ToGray8_1)
{
    static const uint8_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray8(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = 1;
    info.yOffset = 1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint8_t expected_result_data[8 * 8] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 9, 10, 11, 12, 13, 14, 15,
        0, 17, 18, 19, 20, 21, 22, 23,
        0, 25, 26, 27, 28, 29, 30, 31,
        0, 33, 34, 35, 36, 37, 38, 39,
        0, 41, 42, 43, 44, 45, 46, 47,
        0, 49, 50, 51, 52, 53, 54, 55,
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8), 0);
}

TEST(BitmapOperations, CopyWithOffsetGray8ToGray8_2)
{
    static const uint8_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray8(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = -1;
    info.yOffset = -1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint8_t expected_result_data[8 * 8] =
    {
        10, 11, 12, 13, 14, 15, 16, 0,
        18, 19, 20, 21, 22, 23, 24, 0,
        26, 27, 28, 29, 30, 31, 32, 0,
        34, 35, 36, 37, 38, 39, 40, 0,
        42, 43, 44, 45, 46, 47, 48, 0,
        50, 51, 52, 53, 54, 55, 56, 0,
        58, 59, 60, 61, 62, 63, 64, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8), 0);
}


TEST(BitmapOperations, CopyWithOffsetGray16ToGray16_1)
{
    static const uint16_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8 * 2);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray16(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = 1;
    info.yOffset = 1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint16_t expected_result_data[8 * 8] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 9, 10, 11, 12, 13, 14, 15,
        0, 17, 18, 19, 20, 21, 22, 23,
        0, 25, 26, 27, 28, 29, 30, 31,
        0, 33, 34, 35, 36, 37, 38, 39,
        0, 41, 42, 43, 44, 45, 46, 47,
        0, 49, 50, 51, 52, 53, 54, 55,
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8 * 2), 0);
}

TEST(BitmapOperations, CopyWithOffsetGray16ToGray16_2)
{
    static const uint16_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8 * 2);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray16(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = -1;
    info.yOffset = -1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint16_t expected_result_data[8 * 8] =
    {
        10, 11, 12, 13, 14, 15, 16, 0,
        18, 19, 20, 21, 22, 23, 24, 0,
        26, 27, 28, 29, 30, 31, 32, 0,
        34, 35, 36, 37, 38, 39, 40, 0,
        42, 43, 44, 45, 46, 47, 48, 0,
        50, 51, 52, 53, 54, 55, 56, 0,
        58, 59, 60, 61, 62, 63, 64, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8 * 2), 0);
}

TEST(BitmapOperations, CopyWithOffsetGray8ToGray16_1)
{
    static const uint8_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray16(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = 1;
    info.yOffset = 1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint16_t expected_result_data[8 * 8] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7,
        0, 9, 10, 11, 12, 13, 14, 15,
        0, 17, 18, 19, 20, 21, 22, 23,
        0, 25, 26, 27, 28, 29, 30, 31,
        0, 33, 34, 35, 36, 37, 38, 39,
        0, 41, 42, 43, 44, 45, 46, 47,
        0, 49, 50, 51, 52, 53, 54, 55,
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8 * 2), 0);
}

TEST(BitmapOperations, CopyWithOffsetGray8ToGray16_2)
{
    static const uint8_t source_data[8 * 8] =
    {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    };

    auto source = CBitmapData<CHeapAllocator>::Create(PixelType::Gray8, 8, 8, 8);
    ScopedBitmapLockerSP source_locked{ source };
    memcpy(source_locked.ptrDataRoi, source_data, 8 * 8);

    auto destination = CBitmapData<CHeapAllocator>::Create(PixelType::Gray16, 8, 8, 8 * 2);
    ScopedBitmapLockerSP destination_locked{ destination };
    CBitmapOperations::Fill_Gray16(8, 8, destination_locked.ptrDataRoi, destination_locked.stride, 0);

    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = -1;
    info.yOffset = -1;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locked.ptrDataRoi;
    info.srcStride = source_locked.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();
    info.dstPixelType = destination->GetPixelType();
    info.dstPtr = destination_locked.ptrDataRoi;
    info.dstStride = destination_locked.stride;
    info.dstWidth = destination->GetWidth();
    info.dstHeight = destination->GetHeight();
    info.drawTileBorder = false;
    CBitmapOperations::CopyWithOffset(info);

    static const uint16_t expected_result_data[8 * 8] =
    {
        10, 11, 12, 13, 14, 15, 16, 0,
        18, 19, 20, 21, 22, 23, 24, 0,
        26, 27, 28, 29, 30, 31, 32, 0,
        34, 35, 36, 37, 38, 39, 40, 0,
        42, 43, 44, 45, 46, 47, 48, 0,
        50, 51, 52, 53, 54, 55, 56, 0,
        58, 59, 60, 61, 62, 63, 64, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    ASSERT_EQ(memcmp(destination_locked.ptrDataRoi, expected_result_data, 8 * 8 * 2), 0);
}
