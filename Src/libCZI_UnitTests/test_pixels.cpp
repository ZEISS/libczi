// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/bitmapData.h"

using namespace libCZI;
using namespace std;

namespace
{
    string BitonalBitmapToString(IBitonalBitmapData* bitonal_bitmap, const char* line_end = "")
    {
        stringstream ss;
        const IntSize size = bitonal_bitmap->GetSize();
        ScopedBitonalBitmapLockerP lock{ bitonal_bitmap };
        for (uint32_t y = 0; y < size.h; ++y)
        {
            for (uint32_t x = 0; x < size.w; ++x)
            {
                ss << (BitonalBitmapOperations::GetPixelValue(lock, size, x, y) ? '*' : '.');
            }

            ss << line_end;
        }
        return ss.str();
    }

    template <PixelType tPixelType, typename tPixelDataType>
    void TestBitonalBitmapOperationsCopyAtScenario1()
    {
        // Arrange
        constexpr int kSourceWidth = 5;
        constexpr int kSourceHeight = 5;
        constexpr int kDestinationWidth = 5 + 2;
        constexpr int kDestinationHeight = 5 + 2;

        // Create source bitmap
        auto src_bitmap = CStdBitmapData::Create(tPixelType, kSourceWidth, kSourceHeight);

        {
            // Fill source bitmap with test pattern (which is 0, 1, 2, ..., 24)
            ScopedBitmapLockerSP source_locker{ src_bitmap };
            for (int y = 0; y < kSourceHeight; ++y)
            {
                tPixelDataType* row_ptr = reinterpret_cast<tPixelDataType*>(static_cast<uint8_t*>(source_locker.ptrDataRoi) + static_cast<size_t>(y) * source_locker.stride);
                for (int x = 0; x < kSourceWidth; ++x)
                {
                    row_ptr[x] = static_cast<tPixelDataType>(y * kSourceWidth + x);
                }
            }
        }

        // Create bitonal mask bitmap
        auto mask_bitmap = CStdBitonalBitmapData::Create(kDestinationWidth, kSourceHeight);

        // Fill mask bitmap with test pattern
        for (int y = 0; y < kSourceHeight; ++y)
        {
            for (int x = 0; x < kDestinationWidth; ++x)
            {
                const int pixel_number = y * kDestinationWidth + x;
                BitonalBitmapOperations::SetPixelValue(mask_bitmap, x, y, pixel_number & 1);
            }
        }

        // Create destination bitmap
        auto dst_bitmap = CStdBitmapData::Create(tPixelType, kDestinationWidth, kDestinationWidth);

        {
            // Fill destination bitmap with constant value (123)
            ScopedBitmapLockerSP destination_locker{ dst_bitmap };
            for (int y = 0; y < kDestinationHeight; ++y)
            {
                tPixelDataType* row_ptr = reinterpret_cast<tPixelDataType*>(static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride);
                for (int x = 0; x < kDestinationWidth; ++x)
                {
                    row_ptr[x] = 123;
                }
            }
        }

        // Act
        BitonalBitmapOperations::CopyAt(src_bitmap.get(), mask_bitmap.get(), IntPoint{ 1, 1 }, dst_bitmap.get());

        // Assert
        {
            ScopedBitmapLockerSP destination_locker{ dst_bitmap };
            for (int y = 0; y < kDestinationHeight; ++y)
            {
                const tPixelDataType* row_ptr = reinterpret_cast<tPixelDataType*>(static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride);
                for (int x = 0; x < kDestinationWidth; ++x)
                {
                    tPixelDataType expected_value;
                    if (y == 0 || y == kDestinationHeight - 1 || x == 0 || x == kDestinationWidth - 1)
                    {
                        expected_value = 123;
                    }
                    else
                    {
                        const int pixel_number = (y - 1) * kSourceHeight + (x - 1);
                        expected_value = (pixel_number & 1) ? static_cast<tPixelDataType>(pixel_number) : 123;
                    }

                    EXPECT_EQ(row_ptr[x], expected_value);
                }
            }
        }
    }
}

TEST(Pixels, BitonalBitmapOperationsGetPixelValue)
{
    const auto bm = CStdBitonalBitmapData::Create(10, 10);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    BitonalBitmapOperations::SetPixelValue(bm, 0, 0, true);
    BitonalBitmapOperations::SetPixelValue(bm, 9, 9, true);
    BitonalBitmapOperations::SetPixelValue(bm, 5, 5, true);

    EXPECT_TRUE(BitonalBitmapOperations::GetPixelValue(bm, 0, 0));
    EXPECT_TRUE(BitonalBitmapOperations::GetPixelValue(bm, 9, 9));
    EXPECT_TRUE(BitonalBitmapOperations::GetPixelValue(bm, 5, 5));
    EXPECT_FALSE(BitonalBitmapOperations::GetPixelValue(bm, 1, 1));
    EXPECT_FALSE(BitonalBitmapOperations::GetPixelValue(bm, 8, 8));
    EXPECT_FALSE(BitonalBitmapOperations::GetPixelValue(bm, 4, 4));

    EXPECT_ANY_THROW(BitonalBitmapOperations::GetPixelValue(bm, 10, 10));
    EXPECT_ANY_THROW(BitonalBitmapOperations::GetPixelValue(bm, 10, 0));
    EXPECT_ANY_THROW(BitonalBitmapOperations::GetPixelValue(bm, 0, 10));
}

TEST(Pixels, BitonalBitmapOperationsCopyAt_Gray16_Scenario1)
{
    TestBitonalBitmapOperationsCopyAtScenario1<PixelType::Gray16, uint16_t>();
}

TEST(Pixels, BitonalBitmapOperationsCopyAt_Gray8_Scenario1)
{
    TestBitonalBitmapOperationsCopyAtScenario1<PixelType::Gray8, uint8_t>();
}

TEST(Pixels, BitonalBitmapOperationsCopyAt_GrayFloat_Scenario1)
{
    TestBitonalBitmapOperationsCopyAtScenario1<PixelType::Gray32Float, float>();
}

TEST(Pixels, BitonalFillScenario1)
{
    // Arrange
    const auto bm = CStdBitonalBitmapData::Create(20, 20);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    // Act
    BitonalBitmapOperations::Fill(bm, IntRect{ 5,5,5,5 }, true);

    // Assert
    constexpr char expected_result[] =
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        ".....*****.........."
        ".....*****.........."
        ".....*****.........."
        ".....*****.........."
        ".....*****.........."
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        "...................."
        "....................";
    EXPECT_EQ(BitonalBitmapToString(bm.get()), expected_result);
}

TEST(Pixels, BitonalFillScenario2)
{
    // Arrange
    const auto bm = CStdBitonalBitmapData::Create(70, 20);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    // Act
    BitonalBitmapOperations::Fill(bm, IntRect{ 5, 5, 40, 5 }, true);
    BitonalBitmapOperations::Fill(bm, IntRect{ 6, 6, 38, 3 }, false);

    // Assert
    constexpr char expected_result[] =
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        ".....****************************************........................."
        ".....*......................................*........................."
        ".....*......................................*........................."
        ".....*......................................*........................."
        ".....****************************************........................."
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................"
        "......................................................................";
    EXPECT_EQ(BitonalBitmapToString(bm.get()), expected_result);
}

TEST(Pixels, BitonalFillScenario3)
{
    // Arrange
    const auto bm = CStdBitonalBitmapData::Create(10, 10);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    // Act
    BitonalBitmapOperations::Fill(bm, IntRect{ 8, 3, 2, 2 }, true);

    // Assert
    constexpr char expected_result[] =
        ".........."
        ".........."
        ".........."
        "........**"
        "........**"
        ".........."
        ".........."
        ".........."
        ".........."
        "..........";
    EXPECT_EQ(BitonalBitmapToString(bm.get()), expected_result);
}

TEST(Pixels, BitonalFillScenario4)
{
    // Arrange
    const auto bm = CStdBitonalBitmapData::Create(10, 10);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    // Act
    BitonalBitmapOperations::Fill(bm, IntRect{ 7, 3, 2, 2 }, true);

    // Assert
    constexpr char expected_result[] =
        ".........."
        ".........."
        ".........."
        ".......**."
        ".......**."
        ".........."
        ".........."
        ".........."
        ".........."
        "..........";
    EXPECT_EQ(BitonalBitmapToString(bm.get()), expected_result);
}

TEST(Pixels, BitonalFillScenario5)
{
    // Arrange
    const auto bm = CStdBitonalBitmapData::Create(70, 20);
    BitonalBitmapOperations::SetAllPixels(bm, false);

    // Act
    // test clipping to bitmap area
    BitonalBitmapOperations::Fill(bm, IntRect{ -5, 5, 100, 5 }, true);

    // Assert
    constexpr char expected_result[] =
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "**********************************************************************" 
        "**********************************************************************" 
        "**********************************************************************" 
        "**********************************************************************" 
        "**********************************************************************" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................" 
        "......................................................................";
    EXPECT_EQ(BitonalBitmapToString(bm.get()), expected_result);
}

TEST(Pixels, BitonalDecimateScenario1)
{
    // arrange
    auto source = CStdBitonalBitmapData::Create(6,6);

    {
        ScopedBitonalBitmapLockerSP source_locker{ source };
        static_cast<uint8_t*>(source_locker.ptrData)[0] = 0b11110100;
        static_cast<uint8_t*>(source_locker.ptrData)[1] = 0b11110000;
        static_cast<uint8_t*>(source_locker.ptrData)[2] = 0b01101000;
        static_cast<uint8_t*>(source_locker.ptrData)[3] = 0b10000000;
        static_cast<uint8_t*>(source_locker.ptrData)[4] = 0b11000010;
        static_cast<uint8_t*>(source_locker.ptrData)[5] = 0b11000001;
    }

    // act
    auto decimated = BitonalBitmapOperations::Decimate(1, source.get());

    // assert
    constexpr char expected_result[] =
        "**."
        "..."
        "...";

    EXPECT_EQ(BitonalBitmapToString(decimated.get()), expected_result);
}

TEST(Pixels, BitonalDecimateScenario2)
{
    // arrange
    auto source = CStdBitonalBitmapData::Create(6,6);

    {
        ScopedBitonalBitmapLockerSP source_locker{ source };
        static_cast<uint8_t*>(source_locker.ptrData)[0] = 0b11110100;
        static_cast<uint8_t*>(source_locker.ptrData)[1] = 0b11110000;
        static_cast<uint8_t*>(source_locker.ptrData)[2] = 0b01101000;
        static_cast<uint8_t*>(source_locker.ptrData)[3] = 0b10000000;
        static_cast<uint8_t*>(source_locker.ptrData)[4] = 0b11000010;
        static_cast<uint8_t*>(source_locker.ptrData)[5] = 0b11000001;
    }

    // act
    auto decimated = BitonalBitmapOperations::Decimate(0, source.get());

    // assert
    constexpr char expected_result[] =
        "**."
        ".**"
        "*..";

    EXPECT_EQ(BitonalBitmapToString(decimated.get()), expected_result);
}

TEST(Pixels, CallSetPixelValueWithInvalidArgumentsAndExpectException1)
{
    BitonalBitmapLockInfo lockInfo;
    lockInfo.ptrData = nullptr;
    lockInfo.stride = 10;
    lockInfo.size = 100;
    const IntSize extent{ 80, 1 };
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(lockInfo, extent, 0, 0, true));

    auto data = std::make_unique<uint8_t[]>(100);
    lockInfo.ptrData = data.get();
    lockInfo.stride = 9;        // this tride it too small (should be at least 10)
    lockInfo.size = 100;
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(lockInfo, extent, 10, 0, true));

    lockInfo.stride = 10;        
    lockInfo.size = 9;      // this size it too small (should be at least 10)
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(lockInfo, extent, 10, 0, true));

    lockInfo.size = 100;
    // now, the x-position is out of bounds
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(lockInfo, extent, 80, 0, true));
    // now, the y-position is out of bounds
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(lockInfo, extent, 10, 1, true));
}

TEST(Pixels, CallSetPixelValueWithInvalidArgumentsAndExpectException2)
{
    shared_ptr<IBitonalBitmapData> bitonal_bitmap;
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(bitonal_bitmap, 0, 0, true));

    bitonal_bitmap = CStdBitonalBitmapData::Create(10, 10);
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(bitonal_bitmap, 10, 0, true));
    EXPECT_ANY_THROW(BitonalBitmapOperations::SetPixelValue(bitonal_bitmap, 0, 10, true));
}
