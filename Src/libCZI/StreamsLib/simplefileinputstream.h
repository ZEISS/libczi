// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#include <string>
#include <cstdio>
#include <mutex>

#include "../libCZI.h"

/// Implementation of the IStream-interface for files based on C-API fseek/fread.
/// Since seeking is not thread-safe with this API, we need to have a critical section
/// around the read-method.
class SimpleFileInputStream : public libCZI::IStream
{
private:
    FILE* fp_;
    std::mutex request_mutex_;///< Mutex to serialize the read operations.
public:
    SimpleFileInputStream() = delete;
    explicit SimpleFileInputStream(const wchar_t* filename);
    explicit SimpleFileInputStream(const std::string& filename);
    ~SimpleFileInputStream() override;
public: // interface libCZI::IStream
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

