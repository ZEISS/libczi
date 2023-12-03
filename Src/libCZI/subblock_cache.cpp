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

SubBlockCache::SubBlockCache()
{
}

SubBlockCache::~SubBlockCache()
{}

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
    if (options.maxMemoryUsage != numeric_limits<uint64_t>::max())
    {
        this->PruneToMemoryUsage(options.maxMemoryUsage);
               lock_guard<mutex> lck(this->mutex_);
        if (this->cache_size_in_bytes_ > options.maxMemoryUsage)
        {
                       // Sort the cache entries by LRU value
                       //            vector<pair<int, CacheEntry>> sorted_cache_entries;
                       //                       sorted_cache_entries.reserve(this->cache_.size());
                       //                                  for (const auto& element : this->cache_)
                       //                                             {
                       //                                                            sorted_cache_entries.push_back(element);
                       //                                                                       }
                       //                                                                                  sort(sorted_cache_entries.begin(), sorted_cache_entries.end(), [](const auto& a, const auto& b) { return a.second.lru_value < b.second.lru_value; });
                                  // Remove the oldest entries until the memory usage is below the threshold
                                  //            for (const auto& element : sorted_cache_entries)
                                  //                       {
                                  //                                      this->cache_size_in_bytes_ -= SubBlockCache::CalculateSizeInBytes(element.second.bitmap.get());
                                  //                                                     this->cache_.erase(element.first);
                                  //                                                                    --this->cache_subblock_count_;
                                  //                                                                                   if (this->cache_size_in_bytes_ <= options.maxMemoryUsage)
                                  //                                                                                                  {
                                  //                                                                                                                     break;
                                  //                                                                                                                                    }
                                  //                                                                                                                                               }
                                  //                                                                                                                                                      }
                                  //                                                                                                                                                         }
        }
    }
}

void SubBlockCache::PruneByMemoryUsage(std::uint64_t max_memory_usage)
{
    lock_guard<mutex> lck(this->mutex_);
    if (this->cache_size_in_bytes_ > max_memory_usage)
    {
               // Sort the cache entries by LRU value
               //        vector<pair<int, CacheEntry>> sorted_cache_entries;
               //               sorted_cache_entries.reserve(this->cache_.size());
               //                      for (const auto& element : this->cache_)
               //                             {
               //                                        sorted_cache_entries.push_back(element);
               //                                               }
               //                                                      sort(sorted_cache_entries.begin(), sorted_cache_entries.end(), [](const auto& a, const auto& b) { return a.second.lru_value < b.second.lru_value; });
                      // Remove the oldest entries until the memory usage is below the threshold
                      //        for (const auto& element : sorted_cache_entries)
                      //               {
                      //                          this->cache_size_in_bytes_ -= SubBlockCache::CalculateSizeInBytes(element.second.bitmap.get());
                      //                                     this->cache_.erase(element.first);
                      //                                                --this->cache_subblock_count_;
                      //                                                           if (this->cache_size_in_bytes_ <= max_memory_usage)
                      //                                                                      {
                      //                                                                                     break;
                      //                                                                                                }
                      //                                                                                                       }
                      //                                                                                                          }
    }
}

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitmapData* bitmap)
{
    const IntSize size = bitmap->GetSize();
    return static_cast<uint64_t>(size.w) * size.h * Utils::GetBytesPerPixel(bitmap->GetPixelType());
}

