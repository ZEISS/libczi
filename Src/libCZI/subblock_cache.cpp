// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "subblock_cache.h"

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

std::shared_ptr<ISubBlockCache> libCZI::CreateSubBlockCache()
{
    return make_shared<SubBlockCache>();
}

ISubBlockCacheStatistics::Statistics SubBlockCache::GetStatistics(std::uint8_t mask) const
{
    Statistics result{};
    lock_guard<mutex> lck(this->mutex_);
    if (mask == ISubBlockCacheStatistics::kMemoryUsage)
    {
        result.validityMask = ISubBlockCacheStatistics::kMemoryUsage;
        result.memoryUsage = this->cache_size_in_bytes_;
    }
    else if (mask == ISubBlockCacheStatistics::kElementsCount)
    {
        result.validityMask = ISubBlockCacheStatistics::kElementsCount;
        result.elementsCount = this->cache_subblock_count_;
    }
    else if (mask == (ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount))
    {
        result.validityMask = ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount;

        result.memoryUsage = this->cache_size_in_bytes_;
        result.elementsCount = this->cache_subblock_count_;
    }

    return result;
}

ISubBlockCacheOperation::CacheItem SubBlockCache::Get(int subblock_index)
{
    lock_guard<mutex> lck(this->mutex_);
    return this->GetAndTouchLocked(subblock_index);
}

ISubBlockCacheOperation::CacheItem SubBlockCache::GetAndTouchLocked(int subblock_index)
{
    const auto element = this->cache_.find(subblock_index);
    if (element != this->cache_.end())
    {
        this->lru_.splice(this->lru_.begin(), this->lru_, element->second.lruPosition);
        return { element->second.bitmap, element->second.mask };
    }

    return {};
}

void SubBlockCache::Add(int subblock_index, const ISubBlockCacheOperation::CacheItem& cache_item)
{
    lock_guard<mutex> lck(this->mutex_);
    this->AddLocked(subblock_index, cache_item);
}

void SubBlockCache::AddLocked(int subblock_index, const ISubBlockCacheOperation::CacheItem& cache_item)
{
    if (!cache_item.IsValid())
    {
        return;
    }

    const auto size = SubBlockCache::CalculateSizeInBytes(cache_item.bitmap.get(), cache_item.mask.get());
    const auto existing = this->cache_.find(subblock_index);
    if (existing != this->cache_.end())
    {
        this->cache_size_in_bytes_ -= existing->second.sizeInBytes;
        this->lru_.erase(existing->second.lruPosition);
        this->cache_.erase(existing);
    }

    this->lru_.push_front(subblock_index);
    this->cache_.emplace(subblock_index, CacheEntry{
        cache_item.bitmap, cache_item.mask, size, this->lru_.begin()});
    this->cache_size_in_bytes_ += size;
    this->cache_subblock_count_ = static_cast<std::uint32_t>(this->cache_.size());
}

ISubBlockCacheOperation::CacheItem SubBlockCache::GetOrCreate(
    int subblock_index, const std::function<CacheItem()>& loader)
{
    std::shared_ptr<InFlightLoad> in_flight;
    {
        unique_lock<mutex> lock(this->mutex_);
        auto cached = this->GetAndTouchLocked(subblock_index);
        if (cached.IsValid())
        {
            return cached;
        }

        const auto existing = this->in_flight_loads_.find(subblock_index);
        if (existing != this->in_flight_loads_.end())
        {
            in_flight = existing->second;
            in_flight->condition.wait(lock, [&in_flight] { return in_flight->ready; });
            if (in_flight->exception)
            {
                rethrow_exception(in_flight->exception);
            }

            return in_flight->result;
        }

        in_flight = make_shared<InFlightLoad>();
        this->in_flight_loads_.emplace(subblock_index, in_flight);
    }

    CacheItem loaded;
    exception_ptr exception;
    try
    {
        loaded = loader();
    }
    catch (...)
    {
        exception = current_exception();
    }

    {
        lock_guard<mutex> lock(this->mutex_);
        if (!exception && loaded.IsValid())
        {
            this->AddLocked(subblock_index, loaded);
        }

        in_flight->result = loaded;
        in_flight->exception = exception;
        in_flight->ready = true;
        this->in_flight_loads_.erase(subblock_index);
    }
    in_flight->condition.notify_all();

    if (exception)
    {
        rethrow_exception(exception);
    }

    return loaded;
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
    // Entries are ordered from most to least recently used, so each eviction is O(1).
    while (this->cache_size_in_bytes_ > max_memory_usage || this->cache_subblock_count_ > max_element_count)
    {
        if (this->lru_.empty())
        {
            break;
        }

        const int oldest_key = this->lru_.back();
        const auto oldest_element = this->cache_.find(oldest_key);
        this->cache_size_in_bytes_ -= oldest_element->second.sizeInBytes;
        this->cache_.erase(oldest_element);
        this->lru_.pop_back();
        this->cache_subblock_count_ = static_cast<std::uint32_t>(this->cache_.size());
    }
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitmapData* bitmap)
{
    const IntSize size = bitmap->GetSize();
    return static_cast<uint64_t>(size.w) * size.h * Utils::GetBytesPerPixel(bitmap->GetPixelType());
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitonalBitmapData* mask)
{
    if (mask == nullptr)
    {
        return 0;
    }

    const IntSize size = mask->GetSize();
    return static_cast<uint64_t>((size.w + 7) / 8) * size.h;
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitmapData* bitmap, const libCZI::IBitonalBitmapData* mask)
{
    return SubBlockCache::CalculateSizeInBytes(bitmap) + SubBlockCache::CalculateSizeInBytes(mask);
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const CacheEntry& entry)
{
    return entry.sizeInBytes;
}
