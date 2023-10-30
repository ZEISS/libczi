// SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if defined(_WIN32)
#include <string>
#include "../libCZI.h"
#include <Windows.h>

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
