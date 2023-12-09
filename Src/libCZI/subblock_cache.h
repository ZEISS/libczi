// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include <map>
#include <cstdint>
#include <mutex>
#include <atomic>

/// A simplistic sub-block cache implementation. It is thread-safe and uses a LRU eviction strategy.
class SubBlockCache : public libCZI::ISubBlockCache
{
private:
    struct CacheEntry
    {
        std::shared_ptr<libCZI::IBitmapData> bitmap;    ////< The cached bitmap.
        std::uint64_t lru_value;                        ////< The "LRU value" - when marking a cache entry as "used", this value is set to the current value of the "LRU counter".
    };

    std::map<int, CacheEntry> cache_;
    mutable std::mutex mutex_;
    std::atomic_uint64_t lru_counter_{ 0 }; ///< The "LRU counter" - when marking a cache entry as "used", this counter is incremented and the new value is stored in the cache entry.
    std::atomic_uint64_t cache_size_in_bytes_{ 0 }; ////< The current size of the cache in bytes.
    std::atomic_uint32_t cache_subblock_count_{ 0 }; ///< The current number of sub-blocks in the cache.
public:
    SubBlockCache() = default;
    ~SubBlockCache() override = default;

    std::shared_ptr<libCZI::IBitmapData> Get(int subblock_index) override;
    void Add(int subblock_index, std::shared_ptr<libCZI::IBitmapData> bitmap) override;
    void Prune(const PruneOptions& options) override;
    Statistics GetStatistics(std::uint8_t mask) const override;
private:
    void PruneByMemoryUsageAndElementCount(std::uint64_t max_memory_usage, std::uint32_t max_element_count);
    static std::uint64_t CalculateSizeInBytes(const libCZI::IBitmapData* bitmap);
    static bool CompareForLruValue(const std::pair<int, CacheEntry>& a, const std::pair<int, CacheEntry>& b);
};
