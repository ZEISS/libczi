// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "MemOutputStream.h"
#include <algorithm>

CMemOutputStream::CMemOutputStream(size_t initialSize) :ptr(nullptr), allocatedSize(initialSize), usedSize(0)
{
	if (initialSize > 0)
	{
		this->ptr = static_cast<char*>(malloc(initialSize));
	}
}

/*virtual*/CMemOutputStream::~CMemOutputStream()
{
	free(this->ptr);
}

/*virtual*/void CMemOutputStream::Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten)
{
	this->EnsureSize(offset + size);
	memcpy(this->ptr + offset, pv, size);
	this->usedSize = (std::max)(static_cast<size_t>(offset + size), this->usedSize);
	if (ptrBytesWritten != nullptr)
	{
		*ptrBytesWritten = size;
	}
}

void CMemOutputStream::EnsureSize(std::uint64_t newSize)
{
	if (newSize > this->allocatedSize)
	{
		newSize = (std::max)(newSize, std::uint64_t(this->allocatedSize + this->allocatedSize / 4));
		this->ptr = static_cast<char*>(realloc(this->ptr, newSize));
		this->allocatedSize = newSize;
	}
}
