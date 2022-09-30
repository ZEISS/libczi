// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CziAttachment.h"
#include "CziUtils.h"

using namespace libCZI;

CCziAttachment::CCziAttachment(const libCZI::AttachmentInfo& info, const CCZIParse::AttachmentData& data, std::function<void(void*)> deleter)
	:	
	spData(std::shared_ptr<const void>(data.ptrData, deleter)),
	dataSize(data.dataSize),
	info(info)
{
}

CCziAttachment::~CCziAttachment()
{
}

/*virtual*/const libCZI::AttachmentInfo& CCziAttachment::GetAttachmentInfo() const 
{
	return this->info;
}

/*virtual*/void CCziAttachment::DangerousGetRawData(const void*& ptr, size_t& size) const
{
	ptr = this->spData.get();
	size = (size_t)this->dataSize;	// TODO: check the cast
}

/*virtual*/std::shared_ptr<const void> CCziAttachment::GetRawData(size_t* ptrSize)
{
	if (ptrSize!=nullptr)
	{
		*ptrSize = (size_t)this->dataSize;
	}

	return this->spData;
}