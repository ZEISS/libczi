// SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_WINDOWSAPI_AVAILABLE
#include <string>
#include "../libCZI.h"
#include <windows.h>

/// Implementation of the IStream-interface for files based on the Win32-API.
/// It leverages the Win32-API ReadFile passing in an offset, thus allowing for concurrent
/// access without locking.
class WindowsFileInputStream : public libCZI::IStream
{
private:
    HANDLE handle;
public:
    WindowsFileInputStream() = delete;
    explicit WindowsFileInputStream(const wchar_t* filename);
    explicit WindowsFileInputStream(const std::string& filename);
    ~WindowsFileInputStream() override;
public: // interface libCZI::IStream
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

#endif
