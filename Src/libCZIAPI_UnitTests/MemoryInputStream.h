// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

struct ExternalStreamErrorInfoInterop;

class MemoryInputStream
{
private:
    std::vector<std::uint8_t> data;
public:
    MemoryInputStream(const void* pv, size_t size);
    MemoryInputStream(size_t initialSize) : MemoryInputStream(nullptr, initialSize) {}

    const std::uint8_t* GetDataC() const
    {
        return this->data.data();
    }

    std::uint8_t* GetData()
    {
        return this->data.data();
    }

    size_t GetDataSize() const
    {
        return this->data.size();
    }

    std::int32_t Read(
        std::uint64_t offset,
        void* pv,
        std::uint64_t size,
        std::uint64_t* ptrBytesRead,
        ExternalStreamErrorInfoInterop* error_info);
};
