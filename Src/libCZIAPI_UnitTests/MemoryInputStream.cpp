// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "MemoryInputStream.h"
#include <libCZIApi.h>
#include <cstddef>
#include <cstring>

MemoryInputStream::MemoryInputStream(const void* pv, size_t size)
    : data(size)
{
    if (pv != nullptr)
    {
        memcpy(this->data.data(), pv, size);
    }
}

std::int32_t MemoryInputStream::Read(
    std::uint64_t offset,
    void* pv,
    std::uint64_t size,
    std::uint64_t* ptrBytesRead,
    ExternalStreamErrorInfoInterop* error_info)
{
    memset(error_info, 0, sizeof(ExternalStreamErrorInfoInterop));

    if (offset < this->GetDataSize())
    {
        size_t sizeToCopy = (std::min)((size_t)size, (size_t)(this->GetDataSize() - offset));
        memcpy(pv, this->GetData() + offset, sizeToCopy);
        if (ptrBytesRead != nullptr)
        {
            *ptrBytesRead = sizeToCopy;
        }
    }
    else
    {
        if (ptrBytesRead != nullptr)
        {
            *ptrBytesRead = 0;
        }
    }

    return 0;
}
