// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "subblock_cache.h"

using namespace libCZI;
using namespace std;

SubBlockCache::SubBlockCache(const libCZI::ISubBlockCache::Options& options)
{

}

SubBlockCache::~SubBlockCache()
{}

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

/*static*/std::uint64_t SubBlockCache::CalculateSizeInBytes(const libCZI::IBitmapData* bitmap)
{
    const IntSize size = bitmap->GetSize();
    return static_cast<uint64_t>(size.w) * size.h * Utils::GetBytesPerPixel(bitmap->GetPixelType());
}
