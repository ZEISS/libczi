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
    auto element = this->cache_.find(subblock_index);
    if (element != this->cache_.end())
    {
        element->second.last_used = this->usage_counter_.fetch_add(1);
        return element->second.bitmap;
    }
    else
    {
        return {};
    }
}

void SubBlockCache::Add(int subblock_index, std::shared_ptr<IBitmapData> bitmap)
{
    lock_guard<mutex> lck(this->mutex_);
    this->cache_[subblock_index] = { bitmap, this->usage_counter_.fetch_add(1) };
}
