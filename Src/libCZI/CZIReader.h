// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>
#include "libCZI.h"
#include "CziSubBlockDirectory.h"
#include "CziAttachmentsDirectory.h"
#include "FileHeaderSegmentData.h"

class CCZIReader : public libCZI::ICZIReader, public std::enable_shared_from_this<CCZIReader>
{
private:
	std::shared_ptr<libCZI::IStream> stream;
	CFileHeaderSegmentData hdrSegmentData;
	CCziSubBlockDirectory subBlkDir;
	CCziAttachmentsDirectory attachmentDir;
	bool	isOperational;	///<	If true, then stream, hdrSegmentData and subBlkDir can be considered valid and operational
public:
	CCZIReader();
	~CCZIReader() override;

	// interface ISubBlockRepository
	void EnumerateSubBlocks(const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum) override;
	void EnumSubset(const libCZI::IDimCoordinate* planeCoordinate, const libCZI::IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum) override;
	std::shared_ptr<libCZI::ISubBlock> ReadSubBlock(int index) override;
	bool TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, libCZI::SubBlockInfo& info) override;
	bool TryGetSubBlockInfo(int index, libCZI::SubBlockInfo* info) const override;
	libCZI::SubBlockStatistics GetStatistics() override;
	libCZI::PyramidStatistics GetPyramidStatistics() override;

	// interface ISubBlockRepositoryEx
	void EnumerateSubBlocksEx(const std::function<bool(int index, const libCZI::DirectorySubBlockInfo& info)>& funcEnum) override;
	
	// interface ICZIReader
	void Open(std::shared_ptr<libCZI::IStream> stream) override;
	libCZI::FileHeaderInfo GetFileHeaderInfo() override;
	std::shared_ptr<libCZI::IMetadataSegment> ReadMetadataSegment() override;
	std::shared_ptr<libCZI::IAccessor> CreateAccessor(libCZI::AccessorType accessorType) override;
	void Close() override;

	// interface IAttachmentRepository
	void EnumerateAttachments(const std::function<bool(int index, const libCZI::AttachmentInfo& infi)>& funcEnum) override;
	void EnumerateSubset(const char* contentFileType, const char* name, const std::function<bool(int index, const libCZI::AttachmentInfo& infi)>& funcEnum) override;
	std::shared_ptr<libCZI::IAttachment> ReadAttachment(int index) override;

private:
	std::shared_ptr<libCZI::ISubBlock> ReadSubBlock(const CCziSubBlockDirectory::SubBlkEntry& entry);
	std::shared_ptr<libCZI::IAttachment> ReadAttachment(const CCziAttachmentsDirectory::AttachmentEntry& entry);
	std::shared_ptr<libCZI::IMetadataSegment> ReadMetadataSegment(std::uint64_t position);

	void ThrowIfNotOperational();
	void SetOperationalState(bool operational);
};
