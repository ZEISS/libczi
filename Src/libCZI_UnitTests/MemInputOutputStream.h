// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

class CMemInputOutputStream : public libCZI::IInputOutputStream
{
private:
	char* ptr;
	size_t allocatedSize;
	size_t usedSize;
public:
	CMemInputOutputStream(const void* pv, size_t size);
	CMemInputOutputStream(size_t initialSize);
	virtual ~CMemInputOutputStream() override;

	const char* GetDataC() const
	{
		return this->ptr;
	}

	size_t GetDataSize() const
	{
		return this->usedSize;
	}

	std::shared_ptr<void> GetCopy(size_t* pSize) const
	{
		auto spBuffer = std::shared_ptr<void>(malloc(this->usedSize), [](void* p)->void {free(p); });
		memcpy(spBuffer.get(), this->ptr, this->usedSize);
		if (pSize != nullptr)
		{
			*pSize = this->usedSize;
		}

		return spBuffer;
	}

public:	// interface IOutputStream
	virtual void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override;
public:	// interface IStream
	virtual void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
private:
	void EnsureSize(std::uint64_t newSize);
};
