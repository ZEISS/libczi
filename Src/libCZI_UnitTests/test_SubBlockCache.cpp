// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "utils.h"

using namespace libCZI;
using namespace std;

TEST(SubBlockCache, SimpleUseCase1)
{
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Bgr24, 161, 114);
    cache->Add(1, bm2);

    const auto bitmap_from_cache_1 = cache->Get(0);
    EXPECT_TRUE(AreBitmapDataEqual(bm1, bitmap_from_cache_1));

    const auto bitmap_from_cache_2 = cache->Get(1);
    EXPECT_TRUE(AreBitmapDataEqual(bm2, bitmap_from_cache_2));
}

TEST(SubBlockCache, SimpleUseCase2)
{
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Bgr24, 161, 114);
    cache->Add(1, bm2);

    const auto bitmap_from_cache_3 = cache->Get(3);
    EXPECT_TRUE(!bitmap_from_cache_3);
}

TEST(SubBlockCache, OverwriteExisting)
{
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Bgr24, 161, 114);
    cache->Add(1, bm2);
    const auto bm3 = CreateTestBitmap(PixelType::Gray8, 11, 14);
    cache->Add(1, bm3);

    const auto bitmap_from_cache_1 = cache->Get(0);
    EXPECT_TRUE(AreBitmapDataEqual(bm1, bitmap_from_cache_1));

    const auto bitmap_from_cache_2 = cache->Get(1);
    EXPECT_TRUE(AreBitmapDataEqual(bm3, bitmap_from_cache_2));

    const auto statistics_memory_usage = cache->GetStatistics(ISubBlockCacheStatistics::kMemoryUsage);
    EXPECT_EQ(statistics_memory_usage.validityMask, ISubBlockCacheStatistics::kMemoryUsage);
    EXPECT_EQ(statistics_memory_usage.memoryUsage, 163 * 128 *3 + 11 * 14);

    const auto statistics_elements_count = cache->GetStatistics(ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.validityMask, ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.elementsCount, 2);
}

TEST(SubBlockCache, GetStatistics)
{
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Gray8, 4, 2);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Gray16, 2, 2);
    cache->Add(1, bm2);

    const auto statistics_memory_usage = cache->GetStatistics(ISubBlockCacheStatistics::kMemoryUsage);
    EXPECT_EQ(statistics_memory_usage.validityMask, ISubBlockCacheStatistics::kMemoryUsage);
    EXPECT_EQ(statistics_memory_usage.memoryUsage, 4 * 2 + 2 * 2 * 2);

    const auto statistics_elements_count = cache->GetStatistics(ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.validityMask, ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.elementsCount, 2);

    const auto statistics_both = cache->GetStatistics(ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_both.validityMask, ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_both.memoryUsage, 4 * 2 + 2 * 2 * 2);
    EXPECT_EQ(statistics_both.elementsCount, 2);
}
