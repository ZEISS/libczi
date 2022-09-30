// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "CziParse.h"

class CCziSubBlock : public  libCZI::ISubBlock
{
private:
	std::shared_ptr<const void> spData, spAttachment, spMetadata;
	std::uint64_t	dataSize;
	std::uint32_t	attachmentSize;
	std::uint32_t	metaDataSize;
	libCZI::SubBlockInfo	info;
public:
	CCziSubBlock(const libCZI::SubBlockInfo& info,const CCZIParse::SubBlockData& data, std::function<void(void*)> deleter);
	~CCziSubBlock() override;

	// interface ISubBlock
	const libCZI::SubBlockInfo& GetSubBlockInfo() const override;
	void DangerousGetRawData(libCZI::ISubBlock::MemBlkType type, const void*& ptr, size_t& size) const override;
	std::shared_ptr<const void> GetRawData(MemBlkType type, size_t* ptrSize) override;
	std::shared_ptr<libCZI::IBitmapData> CreateBitmap() override;
};
