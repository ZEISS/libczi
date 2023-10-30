// SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
#include "../libCZI.h"

/// <summary>   An input-stream implementation (based on open and pread). This implementation is thread-safe.</summary>
class PreadFileInputStream : public libCZI::IStream
{
private:
    int fileDescriptor;
public:
    PreadFileInputStream() = delete;
    explicit PreadFileInputStream(const wchar_t* filename);
    explicit PreadFileInputStream(const std::string& filename);
    ~PreadFileInputStream() override;
public: // interface libCZI::IStream
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

#endif
