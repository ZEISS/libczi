// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <list>
#include <unordered_map>
#include <mutex>

namespace libCZI
{
    namespace detail
    {

        /// A simplistic sub-block cache implementation. It is thread-safe and uses a LRU eviction strategy.
        class SubBlockCache : public libCZI::ISubBlockCache
        {
        private:
            struct CacheEntry
            {
                std::shared_ptr<libCZI::IBitmapData> bitmap;        ///< The cached bitmap.
                std::shared_ptr<libCZI::IBitonalBitmapData> mask;   ///< The cached bitonal mask (if any).
                std::uint64_t sizeInBytes;
                std::list<int>::iterator lruPosition;
            };

            struct InFlightLoad
            {
                bool ready{false};
                CacheItem result;
                std::exception_ptr exception;
                std::condition_variable condition;
            };

            std::unordered_map<int, CacheEntry> cache_;
            std::list<int> lru_;
            std::unordered_map<int, std::shared_ptr<InFlightLoad>> in_flight_loads_;
            mutable std::mutex mutex_;
            std::uint64_t cache_size_in_bytes_{0};
            std::uint32_t cache_subblock_count_{0};
        public:
            SubBlockCache() = default;
            ~SubBlockCache() override = default;

            CacheItem Get(int subblock_index) override;
            void Add(int subblock_index, const CacheItem& cache_item) override;
            CacheItem GetOrCreate(int subblock_index, const std::function<CacheItem()>& loader) override;
            void Prune(const PruneOptions& options) override;
            Statistics GetStatistics(std::uint8_t mask) const override;
        private:
            void PruneByMemoryUsageAndElementCount(std::uint64_t max_memory_usage, std::uint32_t max_element_count);
            CacheItem GetAndTouchLocked(int subblock_index);
            void AddLocked(int subblock_index, const CacheItem& cache_item);
            static std::uint64_t CalculateSizeInBytes(const libCZI::IBitmapData* bitmap);
            static std::uint64_t CalculateSizeInBytes(const libCZI::IBitonalBitmapData* mask);
            static std::uint64_t CalculateSizeInBytes(const libCZI::IBitmapData* bitmap, const libCZI::IBitonalBitmapData* mask);
            static std::uint64_t CalculateSizeInBytes(const CacheEntry& entry);
        };

    } // namespace detail
} // namespace libCZI
