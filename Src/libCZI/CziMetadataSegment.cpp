// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CziMetadataSegment.h"
#include "CziUtils.h"

using namespace libCZI;

CCziMetadataSegment::CCziMetadataSegment(const CCZIParse::MetadataSegmentData& data, std::function<void(void*)> deleter)
	:	spXmlData(std::shared_ptr<const void>(data.ptrXmlData, deleter)),
		spAttachment(std::shared_ptr<const void>(data.ptrAttachment, deleter)),
		xmlDataSize(data.xmlDataSize),
		attachmentSize(data.attachmentSize)
{
}

CCziMetadataSegment::~CCziMetadataSegment()
{}

// interface ISubBlock
/*virtual*/void CCziMetadataSegment::DangerousGetRawData(MemBlkType type, const void*& ptr, size_t& size) const
{
	switch (type)
	{
	case XmlMetadata:
		ptr = this->spXmlData.get();
		size = (size_t)this->xmlDataSize;
		break;
	case Attachment:
		ptr = this->spAttachment.get();
		size = this->attachmentSize;
		break;
	default:
		throw std::logic_error("illegal value for type");
	}
}

/*virtual*/std::shared_ptr<const void> CCziMetadataSegment::GetRawData(MemBlkType type, size_t* ptrSize)
{
	switch (type)
	{
	case XmlMetadata:
		if (ptrSize != nullptr)
		{
			*ptrSize = (size_t)this->xmlDataSize;
		}

		return this->spXmlData;
	case Attachment:
		if (ptrSize != nullptr)
		{
			*ptrSize = this->attachmentSize;
		}

		return this->spAttachment;
	default:
		throw std::logic_error("illegal value for type");
	}
}
