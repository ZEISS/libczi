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
    EXPECT_EQ(statistics_memory_usage.memoryUsage, 163 * 128 * 3 + 11 * 14);

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

TEST(SubBlockCache, PruneCacheCase1)
{
    // We add two elements to the cache, making the last-added element the least recently used one. When then pruning the cache to 1 element,
    // the first added element (with key=0) should be removed.
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Bgr24, 161, 114);
    cache->Add(1, bm2);

    cache->Prune({ numeric_limits<uint64_t>::max(), 1 });
    const auto statistics_elements_count = cache->GetStatistics(ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.validityMask, ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.elementsCount, 1);

    auto bitmap_from_cache = cache->Get(1);
    EXPECT_TRUE(bitmap_from_cache != nullptr);
    bitmap_from_cache = cache->Get(0);
    EXPECT_TRUE(bitmap_from_cache == nullptr);
}

TEST(SubBlockCache, PruneCacheCase2)
{
    // We add two items to the cache (with key 0, 1). Then, we retrieve element 0 from the cache, which makes it the least recently used one.
    // When then pruning the cache to 1 element, element 1 should be removed.
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Bgr24, 163, 128);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Bgr24, 161, 114);
    cache->Add(1, bm2);

    // Now, element 0 is the oldest one, and 1 is the least recently used one.
    // We now retrieve element 0 from the cache, which makes it the least recently used one.
    auto bitmap_from_cache = cache->Get(0);

    cache->Prune({ numeric_limits<uint64_t>::max(), 1 });
    const auto statistics_elements_count = cache->GetStatistics(ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.validityMask, ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.elementsCount, 1);

    bitmap_from_cache = cache->Get(0);
    EXPECT_TRUE(bitmap_from_cache != nullptr);
    bitmap_from_cache = cache->Get(1);
    EXPECT_TRUE(bitmap_from_cache == nullptr);
}

TEST(SubBlockCache, PruneCacheCase3)
{
    // We add three items to the cache (with key 0, 1, 2), each one byte in size. Then, we request to prune the cache
    // to 1 byte max memory usage. This should remove the first two items from the cache (since they are the oldest entries), 
    // and keep the last one.
    const auto cache = CreateSubBlockCache();
    const auto bm1 = CreateTestBitmap(PixelType::Gray8, 1, 1);
    cache->Add(0, bm1);
    const auto bm2 = CreateTestBitmap(PixelType::Gray8, 1, 1);
    cache->Add(1, bm2);
    const auto bm3 = CreateTestBitmap(PixelType::Gray8, 1, 1);
    cache->Add(2, bm3);

    ISubBlockCache::PruneOptions prune_options;
    prune_options.maxMemoryUsage = 1;
    cache->Prune(prune_options);
    const auto statistics_elements_count = cache->GetStatistics(ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.validityMask, ISubBlockCacheStatistics::kElementsCount);
    EXPECT_EQ(statistics_elements_count.elementsCount, 1);

    auto bitmap_from_cache = cache->Get(0);
    EXPECT_TRUE(bitmap_from_cache == nullptr);
    bitmap_from_cache = cache->Get(1);
    EXPECT_TRUE(bitmap_from_cache == nullptr);
    bitmap_from_cache = cache->Get(2);
    EXPECT_TRUE(bitmap_from_cache != nullptr);
}
