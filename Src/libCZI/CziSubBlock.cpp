// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CziSubBlock.h"
#include "CziUtils.h"

using namespace libCZI;

CCziSubBlock::CCziSubBlock(const libCZI::SubBlockInfo& info, const CCZIParse::SubBlockData& data, std::function<void(void*)> deleter)
	:
	spData(std::shared_ptr<const void>(data.ptrData, deleter)),
	spAttachment(std::shared_ptr<const void>(data.ptrAttachment, deleter)),
	spMetadata(std::shared_ptr<const void>(data.ptrMetadata, deleter)),
	dataSize(data.dataSize),
	attachmentSize(data.attachmentSize),
	metaDataSize(data.metaDataSize),
	info(info)
{
}

CCziSubBlock::~CCziSubBlock()
{
}

/*virtual*/const SubBlockInfo& CCziSubBlock::GetSubBlockInfo() const
{
	return this->info;
}

/*virtual*/void CCziSubBlock::DangerousGetRawData(ISubBlock::MemBlkType type, const void*& ptr, size_t& size) const
{
	switch (type)
	{
	case Metadata:
		ptr = this->spMetadata.get();
		size = (size_t)this->metaDataSize;	// TODO: check the cast
		break;
	case Data:
		ptr = this->spData.get();
		size = (size_t)this->dataSize;
		break;
	case Attachment:
		ptr = this->spAttachment.get();
		size = (size_t)this->attachmentSize;
		break;
	default:
		throw std::logic_error("illegal value for type");
	}
}

/*virtual*/std::shared_ptr<const void> CCziSubBlock::GetRawData(MemBlkType type, size_t* ptrSize)
{
	switch (type)
	{
	case Metadata:
		if (ptrSize != nullptr)
		{
			*ptrSize = this->metaDataSize;
		}

		return this->spMetadata;
	case Data:
		if (ptrSize != nullptr)
		{
			*ptrSize = (size_t)this->dataSize;// TODO: check the cast
		}

		return this->spData;
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

/*virtual*/std::shared_ptr<IBitmapData> CCziSubBlock::CreateBitmap()
{
	return CreateBitmapFromSubBlock(this);
}
