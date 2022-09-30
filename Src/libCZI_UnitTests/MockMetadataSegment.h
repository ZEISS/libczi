// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

class MockMetadataSegment : public libCZI::IMetadataSegment
{
public:
	enum class Type
	{
		Data1,
		Data2,
		Data3,
		Data4,
		Data5,
		Data6,
		InvalidData
	};
private:
	std::string xmlData;
public:
	MockMetadataSegment(Type type = Type::Data1);
	virtual std::shared_ptr<const void> GetRawData(MemBlkType type, size_t* ptrSize) override;
	virtual void DangerousGetRawData(MemBlkType type, const void*& ptr, size_t& size) const override;
};
