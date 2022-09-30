// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "CziParse.h"

class CCziMetadataSegment : public  libCZI::IMetadataSegment
{
private:
	std::shared_ptr<const void> spXmlData, spAttachment;
	std::uint64_t	xmlDataSize;
	std::uint32_t	attachmentSize;
public:
	CCziMetadataSegment(const CCZIParse::MetadataSegmentData& data, std::function<void(void*)> deleter);
	~CCziMetadataSegment() override;

	// interface ISubBlock
	void DangerousGetRawData(MemBlkType type, const void*& ptr, size_t& size) const override;
	std::shared_ptr<const void> GetRawData(MemBlkType type, size_t* ptrSize) override;
};