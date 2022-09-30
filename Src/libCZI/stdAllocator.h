// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstdint>

class CHeapAllocator
{
public:
	void*	Allocate(std::uint64_t size);
	void	Free(void* ptr);
};

class CSharedPtrAllocator
{
private:
	std::shared_ptr<const void> shp;
public:
	explicit CSharedPtrAllocator(std::shared_ptr<const void> shp) :shp(shp) {}

	void*	Allocate(std::uint64_t size) { return const_cast<void*>(this->shp.get()); }
	void	Free(void* ptr) { this->shp.reset(); }
};
