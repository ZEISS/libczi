// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/bitmapData.h"

using namespace libCZI;
using namespace std;

TEST(Pixels, BitonalBitmapOperationsGetPixelValue)
{
}

namespace
{
    template <PixelType tPixelType,typename tPixelDataType>
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
                tPixelDataType* row_ptr = static_cast<tPixelDataType>(static_cast<uint8_t*>(source_locker.ptrDataRoi) + static_cast<size_t>(y) * source_locker.stride);
                for (int x = 0; x < kSourceWidth; ++x)
                {
                    row_ptr[x] = static_cast<tPixelDataType>(y * kSourceWidth + x);
                }
            }
        }

        // Create bitonal mask bitmap
        auto mask_bitmap = CStdBitonalBitmapData::Create(kSourceHeight, kSourceHeight);

        // Fill mask bitmap with test pattern
        for (int y = 0; y < kSourceHeight; ++y)
        {
            for (int x = 0; x < kSourceHeight; ++x)
            {
                const int pixel_number = y * kSourceHeight + x;
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
                tPixelDataType* row_ptr = static_cast<tPixelDataType>(static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride);
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
                const tPixelDataType* row_ptr = static_cast<tPixelDataType>(static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride);
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
                        expected_value = (pixel_number & 1) ? (tPixelDataType)pixel_number : 123;
                    }

                    EXPECT_EQ(row_ptr[x], expected_value);
                }
            }
        }
    }
}

TEST(Pixels, BitonalBitmapOperationsCopyAtScenario1)
{
    // Arrange
    const int kSourceWidth = 5;
    const int kSourceHeight = 5;
    const int kDestinationWidth = 5 + 2;
    const int kDestinationHeight = 5 + 2;

    // Create source bitmap
    auto src_bitmap = CStdBitmapData::Create(PixelType::Gray8, kSourceWidth, kSourceHeight);

    {
        // Fill source bitmap with test pattern (which is 0, 1, 2, ..., 24)
        ScopedBitmapLockerSP source_locker{ src_bitmap };
        for (int y = 0; y < kSourceHeight; ++y)
        {
            uint8_t* row_ptr = static_cast<uint8_t*>(source_locker.ptrDataRoi) + static_cast<size_t>(y) * source_locker.stride;
            for (int x = 0; x < kSourceWidth; ++x)
            {
                row_ptr[x] = static_cast<uint8_t>(y * kSourceWidth + x);
            }
        }
    }

    // Create mask bitmap
    auto mask_bitmap = CStdBitonalBitmapData::Create(kSourceHeight, kSourceHeight);

    // Fill mask bitmap with test pattern
    for (int y = 0; y < kSourceHeight; ++y)
    {
        for (int x = 0; x < kSourceHeight; ++x)
        {
            const int pixel_number = y * kSourceHeight + x;
            BitonalBitmapOperations::SetPixelValue(mask_bitmap, x, y, pixel_number & 1);
        }
    }

    // Create destination bitmap
    auto dst_bitmap = CStdBitmapData::Create(PixelType::Gray8, kDestinationWidth, kDestinationWidth);

    {
        // Fill destination bitmap with constant value (123)
        ScopedBitmapLockerSP destination_locker{ dst_bitmap };
        for (int y = 0; y < kDestinationHeight; ++y)
        {
            uint8_t* row_ptr = static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride;
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
            const uint8_t* row_ptr = static_cast<uint8_t*>(destination_locker.ptrDataRoi) + static_cast<size_t>(y) * destination_locker.stride;
            for (int x = 0; x < kDestinationWidth; ++x)
            {
                uint8_t expected_value;
                if (y == 0 || y == kDestinationHeight - 1 || x == 0 || x == kDestinationWidth - 1)
                {
                    expected_value = 123;
                }
                else
                {
                    const int pixel_number = (y-1) * kSourceHeight + (x-1);
                    expected_value = (pixel_number & 1) ? (uint8_t)pixel_number : 123;
                }

                EXPECT_EQ(row_ptr[x], expected_value);
            }
        }
    }
}
