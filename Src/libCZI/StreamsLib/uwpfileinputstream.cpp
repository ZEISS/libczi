// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "uwpfileinputstream.h"

#if LIBCZI_WINDOWS_UWPAPI_AVAILABLE
#include <limits>
#include <iomanip>
#include "../utilities.h"

UwpFileInputStream::UwpFileInputStream(const std::string& filename)
    : UwpFileInputStream(Utilities::convertUtf8ToWchar_t(filename.c_str()).c_str())
{
}

UwpFileInputStream::UwpFileInputStream(const wchar_t* filename)
    : handle(INVALID_HANDLE_VALUE)
{
    CREATEFILE2_EXTENDED_PARAMETERS params = {};
    params.dwSize = sizeof(params);
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = FILE_FLAG_RANDOM_ACCESS;
    params.dwSecurityQosFlags = 0;
    params.lpSecurityAttributes = nullptr;
    params.hTemplateFile = nullptr;

    const HANDLE file_handle = CreateFile2(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        OPEN_EXISTING,
        &params
    );

    if (file_handle == INVALID_HANDLE_VALUE)
    {
        std::stringstream ss;
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\"";
        throw std::runtime_error(ss.str());
    }

    this->handle = file_handle;
}

UwpFileInputStream::~UwpFileInputStream()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
    }
}

void UwpFileInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
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
