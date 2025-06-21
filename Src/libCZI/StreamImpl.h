// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "inc_libCZI_Config.h"
#include <memory>
#if LIBCZI_WINDOWSAPI_AVAILABLE
#include <windows.h>
#endif

/// <summary>   A simplistic output-stream implementation (based on C-runtime fopen). Note that this implementation is NOT thread-safe.</summary>
class CSimpleOutputStreamStreams : public libCZI::IOutputStream
{
private:
    FILE* fp;
public:
    CSimpleOutputStreamStreams() = delete;
    CSimpleOutputStreamStreams(const wchar_t* filename, bool overwriteExisting);
    ~CSimpleOutputStreamStreams() override;
public: // interface libCZI::IOutputStream
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL

/// <summary>   An output-stream implementation (based on pwrite). This implementation is thread-safe.</summary>
class COutputStreamImplPwrite : public libCZI::IOutputStream
{
private:
    int fileDescriptor;
public:
    COutputStreamImplPwrite() = delete;
    COutputStreamImplPwrite(const wchar_t* filename, bool overwriteExisting);
    ~COutputStreamImplPwrite() override;
public: // interface libCZI::IOutputStream
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

#if LIBCZI_WINDOWSAPI_AVAILABLE

class CSimpleOutputStreamImplWindows : public libCZI::IOutputStream
{
private:
    HANDLE handle;
public:
    CSimpleOutputStreamImplWindows() = delete;
    CSimpleOutputStreamImplWindows(const wchar_t* filename, bool overwriteExisting);
    ~CSimpleOutputStreamImplWindows() override;
public: // interface libCZI::IOutputStream
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

/// <summary>   A stream implementation (based on a memory-block). </summary>
class CStreamImplInMemory : public libCZI::IStream
{
private:
    std::shared_ptr<const void> rawData;
    size_t dataBufferSize;
public:
    CStreamImplInMemory() = delete;
    CStreamImplInMemory(std::shared_ptr<const void> ptr, std::size_t dataSize);
    explicit CStreamImplInMemory(libCZI::IAttachment* attachement);
public: // interface libCZI::IStream
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
class CInputOutputStreamImplPreadPwrite : public libCZI::IInputOutputStream
{
private:
    int fileDescriptor;
public:
    CInputOutputStreamImplPreadPwrite() = delete;
    explicit CInputOutputStreamImplPreadPwrite(const wchar_t* filename);
    ~CInputOutputStreamImplPreadPwrite() override;
public:
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

#if LIBCZI_WINDOWSAPI_AVAILABLE
class CSimpleInputOutputStreamImplWindows : public libCZI::IInputOutputStream
{
private:
    HANDLE handle;
public:
    CSimpleInputOutputStreamImplWindows() = delete;
    explicit CSimpleInputOutputStreamImplWindows(const wchar_t* filename);
    ~CSimpleInputOutputStreamImplWindows() override;
public:
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

class CSimpleInputOutputStreamImpl : public libCZI::IInputOutputStream
{
private:
    FILE* fp;
public:
    CSimpleInputOutputStreamImpl() = delete;
    explicit CSimpleInputOutputStreamImpl(const wchar_t* filename);
    ~CSimpleInputOutputStreamImpl() override;
public:
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
    void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
