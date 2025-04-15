// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <libCZI.h>

class MemoryOutputStream : public libCZI::IOutputStream
{
private:
    char* ptr;
    size_t allocatedSize;
    size_t usedSize;
public:
    MemoryOutputStream(size_t initialSize);
    virtual ~MemoryOutputStream() override;

    const char* GetDataC() const
    {
        return this->ptr;
    }

    size_t GetDataSize() const
    {
        return this->usedSize;
    }

    std::shared_ptr<void> GetCopy(size_t* pSize) const
    {
        auto spBuffer = std::shared_ptr<void>(malloc(this->usedSize), [](void* p)->void {free(p); });
        memcpy(spBuffer.get(), this->ptr, this->usedSize);
        if (pSize != nullptr)
        {
            *pSize = this->usedSize;
        }

        return spBuffer;
    }
public:	// interface IOutputStream
    virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
private:
    void EnsureSize(std::uint64_t newSize);
};
