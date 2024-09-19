// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "StreamImpl.h"
#include <cerrno>
#include <sstream>
#include <codecvt>
#include <iomanip>
#include "utilities.h"

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>   // required on BSD
#endif

using namespace std;

//----------------------------------------------------------------------------

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL

COutputStreamImplPwrite::COutputStreamImplPwrite(const wchar_t* filename, bool overwriteExisting)
    : fileDescriptor(0)
{
    auto filename_utf8 = Utilities::convertWchar_tToUtf8(filename);

    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    if (!overwriteExisting)
    {
        // If filename already exists, open will fail with EEXIST
        flags |= O_EXCL;
    }

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    this->fileDescriptor = open(filename_utf8.c_str(), flags, mode);
    if (this->fileDescriptor < 0)
    {
        auto err = errno;
        std::stringstream ss;
        ss << "Error opening the file \"" << filename_utf8 << "\" -> errno=" << err << " (" << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }
}

COutputStreamImplPwrite::~COutputStreamImplPwrite()
{
    if (this->fileDescriptor != 0)
    {
        close(this->fileDescriptor);
    }
}

/*virtual*/void COutputStreamImplPwrite::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
    ssize_t bytesWritten = pwrite(this->fileDescriptor, pv, size, offset);
    if (bytesWritten < 0)
    {
        auto err = errno;
        std::stringstream ss;
        ss << "Error reading from file (errno=" << err << " -> " << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesWritten;
    }
}
#endif

//----------------------------------------------------------------------------

#if LIBCZI_WINDOWSAPI_AVAILABLE

CSimpleOutputStreamImplWindows::CSimpleOutputStreamImplWindows(const wchar_t* filename, bool overwriteExisting)
    : handle(INVALID_HANDLE_VALUE)
{
    DWORD dwCreationDisposition = overwriteExisting ? CREATE_ALWAYS : CREATE_NEW;
    HANDLE h = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        std::stringstream ss;
        wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
        ss << "Error opening the file \"" << utf8_conv.to_bytes(filename) << "\" for output.";
        throw std::runtime_error(ss.str());
    }

    this->handle = h;
}

CSimpleOutputStreamImplWindows::~CSimpleOutputStreamImplWindows()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
        this->handle = INVALID_HANDLE_VALUE;
    }
}

/*virtual*/void CSimpleOutputStreamImplWindows::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
    if (size > (std::numeric_limits<DWORD>::max)())
    {
        throw std::runtime_error("size is too large");
    }

    OVERLAPPED ol = {};
    ol.Offset = static_cast<DWORD>(offset);
    ol.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD bytes_written;
    const BOOL B = WriteFile(this->handle, pv, static_cast<DWORD>(size), &bytes_written, &ol);
    if (!B)
    {
        const DWORD last_error = GetLastError();
        std::stringstream ss;
        ss << "Error writing to file (LastError=" << std::hex << std::setfill('0') << std::setw(8) << std::showbase << last_error << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytes_written;
    }
}
#endif

//----------------------------------------------------------------------------
CStreamImplInMemory::CStreamImplInMemory(std::shared_ptr<const void> ptr, std::size_t dataSize)
    : rawData(std::move(ptr)), dataBufferSize(dataSize)
{
}

CStreamImplInMemory::CStreamImplInMemory(libCZI::IAttachment* attachement)
{
    this->rawData = attachement->GetRawData(&this->dataBufferSize);
}

/*virtual*/void CStreamImplInMemory::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    if (pv == nullptr)
    {
        throw std::invalid_argument("Pointer cannot be null");
    }

    if (offset >= this->dataBufferSize)
    {
        std::stringstream ss;
        ss << "Error reading from memory at offset " << offset << " -> requested size: " << size << " bytes, which exceeds actual data size " << this->dataBufferSize << " bytes.";
        throw std::runtime_error(ss.str());
    }

    // Read only to the end of buffer size
    const size_t sizeToCopy = (std::min)((size_t)size, (size_t)(this->dataBufferSize - offset));

    std::memcpy(pv, static_cast<const char*>(rawData.get()) + offset, sizeToCopy);
    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = sizeToCopy;
    }
}

//----------------------------------------------------------------------------
CSimpleOutputStreamStreams::CSimpleOutputStreamStreams(const wchar_t* filename, bool overwriteExisting) : fp(nullptr)
{
    // TODO: check if file exists -> x subspecifier? http://www.cplusplus.com/reference/cstdio/fopen/ or stat -> https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform/230070#230070
#if defined(_WIN32)
    if (overwriteExisting == false)
    {
        FILE* testFp = nullptr;
        errno_t err = _wfopen_s(&testFp, filename, L"rb");
        if (err == 0)
        {
            if (testFp != nullptr)
            {
                fclose(testFp);
            }

            std::stringstream ss;
            wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
            ss << "Error opening the file \"" << utf8_conv.to_bytes(filename) << "\" for writing because it already exists.";
            throw std::runtime_error(ss.str());
        }
    }

    errno_t err = _wfopen_s(&this->fp, filename, L"wb");
#else
    int /*error_t*/ err = 0;

    // convert the wchar_t to an UTF8-string
    auto filename_utf8 = Utilities::convertWchar_tToUtf8(filename);

    if (overwriteExisting == false)
    {
        FILE* testFp = fopen(filename_utf8.c_str(), "rb");
        if (testFp != nullptr)
        {
            fclose(testFp);
            std::stringstream ss;
            ss << "Error opening the file \"" << filename_utf8 << "\" for writing because it already exists.";
            throw std::runtime_error(ss.str());
        }
    }

    this->fp = fopen(filename_utf8.c_str(), "wb");
    if (!this->fp)
    {
        err = errno;
    }
#endif
    if (err != 0)
    {
        std::stringstream ss;
#if (_WIN32)
        char errMsg[100];
        strerror_s(errMsg, sizeof(char), err);
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\" -> errno=" << err << " (" << errMsg << ")";
#else
        ss << "Error opening the file \"" << filename_utf8 << "\" -> errno=" << err << " (" << strerror(err) << ")";
#endif
        throw std::runtime_error(ss.str());
    }
}

CSimpleOutputStreamStreams::~CSimpleOutputStreamStreams()
{
    fclose(this->fp);
}

void CSimpleOutputStreamStreams::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
#if defined(_WIN32)
    int r = _fseeki64(this->fp, offset, SEEK_SET);
#else
    int r = fseeko(this->fp, offset, SEEK_SET);
#endif

    if (r != 0)
    {
        const auto err = errno;
        ostringstream ss;
        ss << "Seek to file-position " << offset << " failed, errno=<<" << err << ".";
        throw std::runtime_error(ss.str());
    }

    const std::uint64_t bytesRead = fwrite(pv, 1, (size_t)size, this->fp);
    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesRead;
    }
}

//-----------------------------------------------------------------------------

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
CInputOutputStreamImplPreadPwrite::CInputOutputStreamImplPreadPwrite(const wchar_t* filename)
    : fileDescriptor(0)
{
    auto filename_utf8 = Utilities::convertWchar_tToUtf8(filename);

    int flags = O_RDWR | O_CREAT;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    this->fileDescriptor = open(filename_utf8.c_str(), flags, mode);
    if (this->fileDescriptor < 0)
    {
        auto err = errno;
        ostringstream ss;
        ss << "Error opening the file \"" << filename_utf8 << "\" -> errno=" << err << " (" << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }
}

CInputOutputStreamImplPreadPwrite::~CInputOutputStreamImplPreadPwrite()
{
    if (this->fileDescriptor != 0)
    {
        close(this->fileDescriptor);
    }
}

/*virtual*/void CInputOutputStreamImplPreadPwrite::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    ssize_t bytesRead = pread(this->fileDescriptor, pv, size, offset);
    if (bytesRead < 0)
    {
        auto err = errno;
        ostringstream ss;
        ss << "Error reading from file (errno=" << err << " -> " << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytesRead;
    }
}

/*virtual*/void CInputOutputStreamImplPreadPwrite::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
    ssize_t bytesWritten = pwrite(this->fileDescriptor, pv, size, offset);
    if (bytesWritten < 0)
    {
        auto err = errno;
        ostringstream ss;
        ss << "Error reading from file (errno=" << err << " -> " << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesWritten;
    }
}
#endif

//-----------------------------------------------------------------------------

#if LIBCZI_WINDOWSAPI_AVAILABLE
CSimpleInputOutputStreamImplWindows::CSimpleInputOutputStreamImplWindows(const wchar_t* filename)
    : handle(INVALID_HANDLE_VALUE)
{
    HANDLE h = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        ostringstream ss;
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\"";
        throw std::runtime_error(ss.str());
    }

    this->handle = h;
}

/*virtual*/ CSimpleInputOutputStreamImplWindows::~CSimpleInputOutputStreamImplWindows()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
    }
}

/*virtual*/void CSimpleInputOutputStreamImplWindows::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    if (size > (std::numeric_limits<DWORD>::max)())
    {
        throw std::runtime_error("size is too large");
    }

    OVERLAPPED ol = {};
    ol.Offset = static_cast<DWORD>(offset);
    ol.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD bytes_read;
    const BOOL B = ReadFile(this->handle, pv, static_cast<DWORD>(size), &bytes_read, &ol);
    if (!B)
    {
        const DWORD last_error = GetLastError();
        ostringstream ss;
        ss << "Error reading from file (LastError=" << std::hex << std::setfill('0') << std::setw(8) << std::showbase << last_error << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytes_read;
    }
}

/*virtual*/void CSimpleInputOutputStreamImplWindows::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
    if (size > (std::numeric_limits<DWORD>::max)())
    {
        throw std::runtime_error("size is too large");
    }

    OVERLAPPED ol = {};
    ol.Offset = static_cast<DWORD>(offset);
    ol.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD bytes_written;
    const BOOL B = WriteFile(this->handle, pv, static_cast<DWORD>(size), &bytes_written, &ol);
    if (!B)
    {
        const DWORD last_error = GetLastError();
        ostringstream ss;
        ss << "Error writing to file (LastError=" << std::hex << std::setfill('0') << std::setw(8) << std::showbase << last_error << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytes_written;
    }
}
#endif

//-----------------------------------------------------------------------------

CSimpleInputOutputStreamImpl::CSimpleInputOutputStreamImpl(const wchar_t* filename)
{
    // TODO: check if file exists -> x subspecifier? http://www.cplusplus.com/reference/cstdio/fopen/ or stat -> https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform/230070#230070
#if defined(_WIN32)
    errno_t err = _wfopen_s(&this->fp, filename, L"rwb");
#else
    int /*error_t*/ err = 0;

    // convert the wchar_t to an UTF8-string
    auto filename_utf8 = Utilities::convertWchar_tToUtf8(filename);

    this->fp = fopen(filename_utf8.c_str(), "rwb");
    if (!this->fp)
    {
        err = errno;
    }
#endif
    if (err != 0)
    {
        std::stringstream ss;
#if (_WIN32)
        char errMsg[100];
        strerror_s(errMsg, sizeof(errMsg), err);
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\" -> errno=" << err << " (" << errMsg << ")";
#else
        ss << "Error opening the file \"" << filename_utf8 << "\" -> errno=" << err << " (" << strerror(err) << ")";
#endif
        throw std::runtime_error(ss.str());
    }
}

CSimpleInputOutputStreamImpl::~CSimpleInputOutputStreamImpl()
{
    fclose(this->fp);
}

void CSimpleInputOutputStreamImpl::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
#if defined(_WIN32)
    int r = _fseeki64(this->fp, offset, SEEK_SET);
#else
    int r = fseeko(this->fp, offset, SEEK_SET);
#endif

    if (r != 0)
    {
        const auto err = errno;
        ostringstream ss;
        ss << "Seek to file-position " << offset << " failed, errno=<<" << err << ".";
        throw std::runtime_error(ss.str());
    }

    const std::uint64_t bytesRead = fwrite(pv, 1, (size_t)size, this->fp);
    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesRead;
    }
}

/*virtual*/void CSimpleInputOutputStreamImpl::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
#if defined(_WIN32)
    int r = _fseeki64(this->fp, offset, SEEK_SET);
#else
    int r = fseeko(this->fp, offset, SEEK_SET);
#endif

    if (r != 0)
    {
        const auto err = errno;
        ostringstream ss;
        ss << "Seek to file-position " << offset << " failed, errno=<<" << err << ".";
        throw std::runtime_error(ss.str());
    }

    const std::uint64_t bytesRead = fread(pv, 1, (size_t)size, this->fp);
    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytesRead;
    }
}
