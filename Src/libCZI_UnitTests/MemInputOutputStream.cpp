// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "MemInputOutputStream.h"

CMemInputOutputStream::CMemInputOutputStream(size_t initialSize) :ptr(nullptr), allocatedSize(initialSize), usedSize(0)
{
	if (initialSize > 0)
	{
		this->ptr = (char*)malloc(initialSize);
	}
}

CMemInputOutputStream::CMemInputOutputStream(const void* pv, size_t size)  :CMemInputOutputStream(size)
{
	this->Write(0, pv, size, nullptr);
}

/*virtual*/CMemInputOutputStream::~CMemInputOutputStream()
{
	free(this->ptr);
}

/*virtual*/void CMemInputOutputStream::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
	this->EnsureSize(offset + size);
	memcpy(this->ptr + offset, pv, size);
	this->usedSize = (std::max)((size_t)(offset + size), this->usedSize);
	if (ptrBytesWritten != nullptr)
	{
		*ptrBytesWritten = size;
	}
}

/*virtual*/void CMemInputOutputStream::Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
	if (offset < this->usedSize)
	{
		size_t sizeToCopy = (std::min)((size_t)size, (size_t)(this->usedSize - offset));
		memcpy(pv, this->ptr + offset, sizeToCopy);
		if (ptrBytesRead != nullptr)
		{
			*ptrBytesRead = sizeToCopy;
		}
	}
	else
	{
		if (ptrBytesRead != nullptr)
		{
			*ptrBytesRead = 0;
		}
	}
}

void CMemInputOutputStream::EnsureSize(std::uint64_t newSize)
{
	if (newSize > this->allocatedSize)
	{
		this->ptr = (char*)realloc(this->ptr, newSize);
		this->allocatedSize = newSize;
	}
}
