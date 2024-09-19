// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "windowsfileinputstream.h"

#if LIBCZI_WINDOWSAPI_AVAILABLE
#include <limits>
#include <iomanip>
#include "../utilities.h"

WindowsFileInputStream::WindowsFileInputStream(const std::string& filename)
    : WindowsFileInputStream(Utilities::convertUtf8ToWchar_t(filename.c_str()).c_str())
{
}

WindowsFileInputStream::WindowsFileInputStream(const wchar_t* filename)
    : handle(INVALID_HANDLE_VALUE)
{
    const HANDLE file_handle = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        std::stringstream ss;
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\"";
        throw std::runtime_error(ss.str());
    }

    this->handle = file_handle;
}

WindowsFileInputStream::~WindowsFileInputStream()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
    }
}

void WindowsFileInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    if (size > (std::numeric_limits<DWORD>::max)())
    {
        throw std::runtime_error("size is too large");
    }

    OVERLAPPED ol = {};
    ol.Offset = static_cast<DWORD>(offset);
    ol.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD bytes_read;
    const BOOL read_file_return_code = ReadFile(this->handle, pv, static_cast<DWORD>(size), &bytes_read, &ol);
    if (!read_file_return_code)
    {
        const DWORD last_error = GetLastError();
        std::stringstream ss;
        ss << "Error reading from file (LastError=" << std::hex << std::setfill('0') << std::setw(8) << std::showbase << last_error << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytes_read;
    }
}

#endif
