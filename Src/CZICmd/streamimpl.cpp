// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#if false
#include "streamimpl.h"
#include <cerrno>
#include "utils.h"

CStreamImpl::CStreamImpl(const wchar_t* filename)
{
    #if defined(WIN32ENV)
	errno_t err = _wfopen_s(&this->fp, filename, L"rb");
    #endif
    #if defined(LINUXENV)
	int/*error_t*/ err = 0;
	std::wstring wstrFname(filename);
	std::string utf8fname = convertToUtf8(wstrFname);
    this->fp = fopen(utf8fname.c_str(),"rb");
    if (!this->fp)
    {
        err = errno;
    }
    #endif
	if (err != 0)
	{
		throw std::logic_error("couldn't open file");
	}
}

CStreamImpl::CStreamImpl(const std::wstring& filename) : CStreamImpl(filename.c_str())
{
}

CStreamImpl::~CStreamImpl()
{
	if (this->fp != nullptr)
	{
		fclose(this->fp);
	}
}

/*virtual*/void CStreamImpl::Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
	std::uint64_t bytesRead;
    #if defined(WIN32ENV)
	int r = _fseeki64(this->fp, offset, SEEK_SET);
    #endif
    #if defined(LINUXENV)
	int r = fseeko(this->fp, offset, SEEK_SET);
    #endif
	if (r != 0)
	{
		throw std::logic_error("error seeking file");
	}

	bytesRead = fread(pv, 1, (size_t)size, this->fp);
	if (ptrBytesRead != nullptr)
	{
		*ptrBytesRead = bytesRead;
	}
}

#endif