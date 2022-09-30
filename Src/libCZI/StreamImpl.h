// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "inc_libCZI_Config.h"
#include <fstream>

/// <summary>	A simplistic stream implementation (based on C-runtime fopen). Note that this implementation is NOT thread-safe.</summary>
class CSimpleStreamImpl : public libCZI::IStream
{
private:
	FILE* fp;
public:
	CSimpleStreamImpl() = delete;
	CSimpleStreamImpl(const wchar_t* filename);
	virtual ~CSimpleStreamImpl() override;
public:	// interface libCZI::IStream
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

/// <summary>	A simplistic output-stream implementation (based on C-runtime fopen). Note that this implementation is NOT thread-safe.</summary>
class CSimpleOutputStreamStreams : public libCZI::IOutputStream
{
private:
	FILE * fp;
public:
	CSimpleOutputStreamStreams() = delete;
	CSimpleOutputStreamStreams(const wchar_t* filename, bool overwriteExisting);
	virtual ~CSimpleOutputStreamStreams() override;
public:	// interface libCZI::IOutputStream
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};

/// <summary>	A simplistic stream implementation (based on C++ streams). Note that this implementation is NOT thread-safe.</summary>
class CSimpleStreamImplCppStreams : public libCZI::IStream
{
private:
	std::ifstream infile;
public:
	CSimpleStreamImplCppStreams() = delete;
	CSimpleStreamImplCppStreams(const wchar_t* filename);
	virtual ~CSimpleStreamImplCppStreams() override;
public:	// interface libCZI::IStream
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
/// <summary>	An input-stream implementation (based on open and pread). This implementation is thread-safe.</summary>
class CStreamImplPread : public libCZI::IStream
{
private:
	int fileDescriptor;
public:
	CStreamImplPread() = delete;
	CStreamImplPread(const wchar_t* filename);
	virtual ~CStreamImplPread() override;
public:	// interface libCZI::IStream
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

/// <summary>	An output-stream implementation (based on open and pwrite). This implementation is thread-safe.</summary>
class COutputStreamImplPwrite : public libCZI::IOutputStream
{
private:
	int fileDescriptor;
public:
	COutputStreamImplPwrite() = delete;
	COutputStreamImplPwrite(const wchar_t* filename, bool overwriteExisting);
	virtual ~COutputStreamImplPwrite() override;
public:	// interface libCZI::IOutputStream
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

#if defined(_WIN32)
class CSimpleStreamImplWindows : public libCZI::IStream
{
private:
	HANDLE handle;
public:
	CSimpleStreamImplWindows() = delete;
	CSimpleStreamImplWindows(const wchar_t* filename);
	virtual ~CSimpleStreamImplWindows() override;
public:	// interface libCZI::IStream
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

class CSimpleOutputStreamImplWindows : public libCZI::IOutputStream
{
private:
	HANDLE handle;
public:
	CSimpleOutputStreamImplWindows() = delete;
	CSimpleOutputStreamImplWindows(const wchar_t* filename, bool overwriteExisting);
	virtual ~CSimpleOutputStreamImplWindows() override;
public:	// interface libCZI::IOutputStream
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

/// <summary>	A stream implementation (based on a memory-block). </summary>
class CStreamImplInMemory : public libCZI::IStream
{
private:
	std::shared_ptr<const void> rawData;
	size_t dataBufferSize;
public:
	CStreamImplInMemory() = delete;
	CStreamImplInMemory(std::shared_ptr<const void> ptr, std::size_t dataSize);
	CStreamImplInMemory(libCZI::IAttachment* attachement);
public:	// interface libCZI::IStream
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
class CInputOutputStreamImplPreadPwrite : public libCZI::IInputOutputStream
{
private:
	int fileDescriptor;
public:
	CInputOutputStreamImplPreadPwrite() = delete;
	CInputOutputStreamImplPreadPwrite(const wchar_t* filename);
	virtual ~CInputOutputStreamImplPreadPwrite() override;
public:
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

#if defined(_WIN32)
class CSimpleInputOutputStreamImplWindows : public libCZI::IInputOutputStream
{
private:
	HANDLE handle;
public:
	CSimpleInputOutputStreamImplWindows() = delete;
	CSimpleInputOutputStreamImplWindows(const wchar_t* filename);
	virtual ~CSimpleInputOutputStreamImplWindows() override;
public:
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
#endif

class CSimpleInputOutputStreamImpl : public libCZI::IInputOutputStream
{
private:
	FILE * fp;
public:
	CSimpleInputOutputStreamImpl() = delete;
	CSimpleInputOutputStreamImpl(const wchar_t* filename);
	virtual ~CSimpleInputOutputStreamImpl() override;
public:
	virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
};
