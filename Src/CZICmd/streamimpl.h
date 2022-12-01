// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
/*
#pragma once

#include "inc_libCZI.h"

class CStreamImpl :public  libCZI::IStream
{
private:
	FILE* fp;
public:
	explicit CStreamImpl(const wchar_t* filename);
	explicit CStreamImpl(const std::wstring& filename);
	~CStreamImpl() override;
public:	// interface libCZI::IStream
	void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};
*/