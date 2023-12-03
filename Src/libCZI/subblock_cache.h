// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include <map>
#include <cstdint>
#include <mutex>
#include <atomic>

class SubBlockCache : public libCZI::ISubBlockCache
{
private:
    struct CacheEntry
    {
        std::shared_ptr<libCZI::IBitmapData> bitmap;
        std::uint64_t lru_value;
    };

    std::map<int, CacheEntry> cache_;
    mutable std::mutex mutex_;
    std::atomic_uint64_t lru_counter_{ 0 };
    std::atomic_uint64_t cache_size_in_bytes_{ 0 };
    std::atomic_uint32_t cache_subblock_count_{ 0 };
public:
    SubBlockCache(const libCZI::ISubBlockCache::PruneOptions& options);
    ~SubBlockCache() override;

    std::shared_ptr<libCZI::IBitmapData> Get(int subblock_index) override;
    void Add(int subblock_index, std::shared_ptr<libCZI::IBitmapData> bitmap) override;
    void Prune(const PruneOptions& options) override;

    Statistics GetStatistics(std::uint8_t mask) const override;

private:
    static std::uint64_t CalculateSizeInBytes(const libCZI::IBitmapData* bitmap);
};
