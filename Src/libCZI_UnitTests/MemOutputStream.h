// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

class CMemOutputStream : public libCZI::IOutputStream
{
private:
	char* ptr;
	size_t allocatedSize;
	size_t usedSize;
public:
	CMemOutputStream(size_t initialSize);
	virtual ~CMemOutputStream() override;

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
private:
	void EnsureSize(std::uint64_t newSize);
};
