// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "subblock_cache.h"

using namespace libCZI;
using namespace std;

std::shared_ptr<ISubBlockCache> libCZI::CreateSubBlockCache()
{
    return make_shared<SubBlockCache>();
}

ISubBlockCacheStatistics::Statistics SubBlockCache::GetStatistics(std::uint8_t mask) const
{
    Statistics result{ 0 };
    if (mask == ISubBlockCacheStatistics::kMemoryUsage)
    {
        result.validityMask = ISubBlockCacheStatistics::kMemoryUsage;
        result.memoryUsage = this->cache_size_in_bytes_.load();
    }
    else if (mask == ISubBlockCacheStatistics::kElementsCount)
    {
        result.validityMask = ISubBlockCacheStatistics::kElementsCount;
        result.elementsCount = this->cache_subblock_count_.load();
    }
    else if (mask == (ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount))
    {
        result.validityMask = ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount;

        // We want to ensure that the memory usage and the element count are consistent, therefore we need to lock reading both values.
        lock_guard<mutex> lck(this->mutex_);
        result.memoryUsage = this->cache_size_in_bytes_.load();
        result.elementsCount = this->cache_subblock_count_.load();
    }

    return result;
}

std::shared_ptr<IBitmapData> SubBlockCache::Get(int subblock_index)
{
    lock_guard<mutex> lck(this->mutex_);
    const auto element = this->cache_.find(subblock_index);
    if (element != this->cache_.end())
    {
        element->second.lru_value = this->lru_counter_.fetch_add(1);
        return element->second.bitmap;
    }

    return {};
}

void SubBlockCache::Add(int subblock_index, std::shared_ptr<IBitmapData> bitmap)
{
    const auto size_in_bytes_of_added_bitmap = SubBlockCache::CalculateSizeInBytes(bitmap.get());
    const auto entry_to_be_added = CacheEntry{ bitmap, this->lru_counter_.fetch_add(1) };

    lock_guard<mutex> lck(this->mutex_);
    const auto result = this->cache_.insert({ subblock_index, entry_to_be_added });
    if (result.second)
    {
        // New element inserted
        this->cache_size_in_bytes_ += size_in_bytes_of_added_bitmap;
        ++this->cache_subblock_count_;
    }
    else
    {
        // Element with the same key already existed
        this->cache_size_in_bytes_ -= SubBlockCache::CalculateSizeInBytes(result.first->second.bitmap.get());
        result.first->second = entry_to_be_added;
        this->cache_size_in_bytes_ += size_in_bytes_of_added_bitmap;
    }
}

void SubBlockCache::Prune(const PruneOptions& options)
{
    if (options.maxMemoryUsage != numeric_limits<decltype(options.maxMemoryUsage)>::max() ||
        options.maxSubBlockCount != numeric_limits<decltype(options.maxSubBlockCount)>::max())
    {
        lock_guard<mutex> lck(this->mutex_);
        this->PruneByMemoryUsageAndElementCount(options.maxMemoryUsage, options.maxSubBlockCount);
    }
}

void SubBlockCache::PruneByMemoryUsageAndElementCount(std::uint64_t max_memory_usage, std::uint32_t max_element_count)
{
    // TODO(JBL): This is a very simple implementation of the prune operation. We determine the oldest element and remove it.
    //            This is repeated until the cache size is below the maximum memory usage and the element count is below the maximum element count.
    //            Detrimental is the fact that we have to iterate over all elements in the cache to determine the oldest element, and we might have 
    //            to do this multiple times. If the number of elements in the cache is large, this might be a performance bottleneck.
    while (this->cache_size_in_bytes_.load() > max_memory_usage || this->cache_subblock_count_.load() > max_element_count)
    {
        auto oldest_element = std::min_element(this->cache_.begin(), this->cache_.end(), SubBlockCache::CompareForLruValue);
        if (oldest_element == this->cache_.end())
        {
            break;
        }

        this->cache_size_in_bytes_ -= SubBlockCache::CalculateSizeInBytes(oldest_element->second.bitmap.get());
        --this->cache_subblock_count_;
        this->cache_.erase(oldest_element);
    }
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitmapData* bitmap)
{
    const IntSize size = bitmap->GetSize();
    return static_cast<uint64_t>(size.w) * size.h * Utils::GetBytesPerPixel(bitmap->GetPixelType());
}

/*static*/bool SubBlockCache::CompareForLruValue(const std::pair<int, CacheEntry>& a, const std::pair<int, CacheEntry>& b)
{
    return a.second.lru_value < b.second.lru_value;
}
