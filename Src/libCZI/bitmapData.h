// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <atomic>
#include "libCZI_Pixels.h"
#include "CziUtils.h"
#include "stdAllocator.h"
#include "Site.h"
#if defined(_DEBUG)
#include <assert.h>
#include "Site.h"
#endif

template  <typename tAllocator = CHeapAllocator>
class CBitmapData : public libCZI::IBitmapData
{
private:
	tAllocator	allocator;
	libCZI::PixelType	pixelType;
	std::uint32_t		width;
	std::uint32_t		height;
	std::uint32_t		pitch;

	std::uint32_t		extraRows;
	std::uint32_t		extraColumns;

	void*	pData;
	std::uint64_t		dataSize;

	std::atomic<int> lockCnt = ATOMIC_VAR_INIT(0);
public:
	static std::shared_ptr<libCZI::IBitmapData> Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch = 0, std::uint32_t extraRows = 0, std::uint32_t extraColumns = 0);
	static std::shared_ptr<libCZI::IBitmapData> Create(tAllocator allocator, libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch);

	libCZI::PixelType		GetPixelType() const override { return this->pixelType; }
	libCZI::IntSize			GetSize() const override { return libCZI::IntSize{ this->width,this->height }; }

	libCZI::BitmapLockInfo	Lock() override
	{
		std::atomic_fetch_add(&this->lockCnt, 1);
		libCZI::BitmapLockInfo bli;
		bli.ptrData = this->pData;
		bli.ptrDataRoi = static_cast<char*>(this->pData) + static_cast<size_t>(this->extraRows)*this->pitch;
		bli.stride = this->pitch;
		bli.size = this->dataSize;
		return bli;
	}

	void Unlock() override
	{
        const int lckCnt = std::atomic_fetch_sub(&this->lockCnt, 1);
		if (lckCnt < 1)
		{
			throw std::logic_error("Lock/Unlock-semantic was violated.");
		}
	}

	virtual ~CBitmapData() override
	{
#if defined(_DEBUG)
		const int lckCnt = std::atomic_load(&this->lockCnt);
		if (lckCnt != 0)
		{
			if (GetSite()->IsEnabled(libCZI::LOGLEVEL_CATASTROPHICERROR))
			{
				std::stringstream ss;
				ss << "FATAL ERROR : Bitmap is being destroyed with a lockCnt <> 0 (lockCnt is: " << lockCnt << ")";
				GetSite()->Log(libCZI::LOGLEVEL_CATASTROPHICERROR, ss);
			}

			assert(lckCnt == 0);
		}
#endif
		this->allocator.Free(this->pData);
	}

	// need to make these two c'tors public (-> make_shared does not work with private/protected c'tor, cf. http://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const )
public:
	CBitmapData(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, std::uint32_t extraRows, std::uint32_t extraColumns)
	{
		uint64_t size = (height + extraRows * 2ULL) * pitch;
		ThrowIfPointerIsNull(this->pData = this->allocator.Allocate(size), size);
		this->dataSize = size;
		this->width = width;
		this->height = height;
		this->pitch = pitch;
		this->extraColumns = extraColumns;
		this->extraRows = extraRows;
		this->pixelType = pixeltype;
	}

	CBitmapData(tAllocator allocator, libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, std::uint32_t extraRows, std::uint32_t extraColumns)
		: allocator(allocator)
	{
		uint64_t size = (height + extraRows * 2ULL) * pitch;
		ThrowIfPointerIsNull(this->pData = this->allocator.Allocate(size), size);
		this->dataSize = size;
		this->width = width;
		this->height = height;
		this->pitch = pitch;
		this->extraColumns = extraColumns;
		this->extraRows = extraRows;
		this->pixelType = pixeltype;
	}

private:
	static std::uint32_t CalcDefaultPitch(libCZI::PixelType pixeltype, int width)
	{
		auto stride = CziUtils::GetBytesPerPel(pixeltype)*width;
		stride = ((stride + 3) / 4) * 4;
		return stride;
	}

	static void ThrowIfPointerIsNull(const void* ptr, uint64_t size)
	{
		if (ptr == nullptr)
		{
			if (GetSite()->IsEnabled(libCZI::LOGLEVEL_ERROR))
			{
				std::stringstream ss;
				ss << "Allocation request (" << size << " bytes) failed";
				GetSite()->Log(libCZI::LOGLEVEL_ERROR, ss.str());
			}
			
			throw std::bad_alloc();
		}
	}
};



template <typename tAllocator>
/*static*/inline std::shared_ptr<libCZI::IBitmapData> CBitmapData<tAllocator>::Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch /*= 0*/, std::uint32_t extraRows /*= 0*/, std::uint32_t extraColumns /*= 0*/)
{
	if (pitch == 0)
	{
		pitch = CalcDefaultPitch(pixeltype, width + 2 * extraColumns);
	}

	auto s = std::make_shared<CBitmapData<tAllocator>>(pixeltype, width, height, pitch, extraRows, extraColumns);

	return s;
}

template <typename tAllocator>
/*static*/inline std::shared_ptr<libCZI::IBitmapData> CBitmapData<tAllocator>::Create(tAllocator allocator, libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t pitch)
{
	auto s = std::make_shared<CBitmapData<tAllocator>>(allocator, pixeltype, width, height, pitch, 0, 0);
	return s;
}

typedef CBitmapData<CHeapAllocator> CStdBitmapData;