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
        std::uint64_t last_used;
    };

    std::map<int, CacheEntry> cache_;
    std::mutex mutex_;
    std::atomic_uint64_t usage_counter_{ 0 };
    std::atomic_uint64_t cache_size_in_bytes_{ 0 };
    std::atomic_uint32_t cache_subblock_count_{ 0 };
public:
    SubBlockCache(const libCZI::ISubBlockCache::Options& options);
    ~SubBlockCache();

    std::shared_ptr<libCZI::IBitmapData> Get(int subblock_index) override;
    void Add(int subblock_index, std::shared_ptr<libCZI::IBitmapData> bitmap) override;

private:
    static std::uint64_t CalculateSizeInBytes(const libCZI::IBitmapData* bitmap);
};
