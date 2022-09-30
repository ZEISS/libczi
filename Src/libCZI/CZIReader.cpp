// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CZIReader.h"
#include "CziParse.h"
#include "CziSubBlock.h"
#include "CziMetadataSegment.h"
#include "CziUtils.h"
#include "utilities.h"
#include "CziAttachment.h"

using namespace std;
using namespace libCZI;

CCZIReader::CCZIReader() : isOperational(false)
{
}

CCZIReader::~CCZIReader()
{
}

/*virtual */void CCZIReader::Open(std::shared_ptr<libCZI::IStream> stream)
{
	if (this->isOperational == true)
	{
		throw logic_error("CZIReader is already operational.");
	}

	this->hdrSegmentData = CCZIParse::ReadFileHeaderSegmentData(stream.get());
	this->subBlkDir = CCZIParse::ReadSubBlockDirectory(stream.get(), this->hdrSegmentData.GetSubBlockDirectoryPosition());
	auto attachmentPos = this->hdrSegmentData.GetAttachmentDirectoryPosition();
	if (attachmentPos != 0)
	{
		// we should be operational without an attachment-directory as well I suppose.
		// TODO: how to determine whether there is "no attachment-directory" - is the check for 0 sufficient?
		this->attachmentDir = std::move(CCZIParse::ReadAttachmentsDirectory(stream.get(), attachmentPos));
	}

	this->stream = stream;
	this->SetOperationalState(true);
}

/*virtual*/FileHeaderInfo CCZIReader::GetFileHeaderInfo()
{
	this->ThrowIfNotOperational();
	FileHeaderInfo fhi;
	fhi.fileGuid = this->hdrSegmentData.GetFileGuid();
	this->hdrSegmentData.GetVersion(&fhi.majorVersion, &fhi.minorVersion);
	return fhi;
}

/*virtual*/std::shared_ptr<libCZI::IMetadataSegment> CCZIReader::ReadMetadataSegment()
{
	this->ThrowIfNotOperational();
	if (!this->hdrSegmentData.GetIsMetadataPositionPositionValid())
	{
		throw LibCZISegmentNotPresent("No metadata-segment available.");
	}

	return this->ReadMetadataSegment(this->hdrSegmentData.GetMetadataPosition());
}

/*virtual*/SubBlockStatistics CCZIReader::GetStatistics()
{
	this->ThrowIfNotOperational();
	SubBlockStatistics s = this->subBlkDir.GetStatistics();
	return s;
}

/*virtual*/libCZI::PyramidStatistics CCZIReader::GetPyramidStatistics()
{
	this->ThrowIfNotOperational();
	return this->subBlkDir.GetPyramidStatistics();
}

/*virtual*/void CCZIReader::EnumerateSubBlocks(const std::function<bool(int index, const SubBlockInfo& info)>& funcEnum)
{
	this->ThrowIfNotOperational();
	this->subBlkDir.EnumSubBlocks(
		[&](int index, const CCziSubBlockDirectory::SubBlkEntry& entry)->bool
	{
		SubBlockInfo info;
		info.compressionModeRaw = entry.Compression;
		info.pixelType = CziUtils::PixelTypeFromInt(entry.PixelType);
		info.coordinate = entry.coordinate;
		info.logicalRect = IntRect{ entry.x,entry.y,entry.width,entry.height };
		info.physicalSize = IntSize{ (std::uint32_t)entry.storedWidth, (std::uint32_t)entry.storedHeight };
		info.mIndex = entry.mIndex;
		return funcEnum(index, info);
	});
}

/*virtual*/void CCZIReader::EnumerateSubBlocksEx(const std::function<bool(int index, const DirectorySubBlockInfo& info)>& funcEnum)
{
	this->ThrowIfNotOperational();
	this->subBlkDir.EnumSubBlocks(
		[&](int index, const CCziSubBlockDirectory::SubBlkEntry& entry)->bool
		{
			DirectorySubBlockInfo info;
			info.compressionModeRaw = entry.Compression;
			info.pixelType = CziUtils::PixelTypeFromInt(entry.PixelType);
			info.coordinate = entry.coordinate;
			info.logicalRect = IntRect{ entry.x,entry.y,entry.width,entry.height };
			info.physicalSize = IntSize{ (std::uint32_t)entry.storedWidth, (std::uint32_t)entry.storedHeight };
			info.mIndex = entry.mIndex;
			info.filePosition = entry.FilePosition;
			return funcEnum(index, info);
		});
}

/*virtual*/void CCZIReader::EnumSubset(const IDimCoordinate* planeCoordinate, const IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const SubBlockInfo& info)>& funcEnum)
{
	this->ThrowIfNotOperational();

	// TODO:
	// Ok... for a first tentative, experimental and quick-n-dirty implementation, simply
	//      walk through all the subblocks. We surely want to have something more elaborated
	//      here.
	this->EnumerateSubBlocks(
		[&](int index, const SubBlockInfo& info)->bool
	{
		// TODO: we only deal with layer 0 currently... or, more precisely, we do not take "zoom" into account at all
		//        -> well... added that boolean "onlyLayer0" - is this sufficient...?
		if (onlyLayer0 == false || (info.physicalSize.w == info.logicalRect.w && info.physicalSize.h == info.logicalRect.h))
		{
			if (planeCoordinate == nullptr || CziUtils::CompareCoordinate(planeCoordinate, &info.coordinate) == true)
			{
				if (roi == nullptr || Utilities::DoIntersect(*roi, info.logicalRect))
				{
					bool b = funcEnum(index, info);
					return b;
				}
			}
		}

		return true;
	});
}

/*virtual*/std::shared_ptr<ISubBlock> CCZIReader::ReadSubBlock(int index)
{
	this->ThrowIfNotOperational();
	CCziSubBlockDirectory::SubBlkEntry entry;
	if (this->subBlkDir.TryGetSubBlock(index, entry) == false)
	{
		return std::shared_ptr<ISubBlock>();
	}

	return this->ReadSubBlock(entry);
}

/*virtual*/bool CCZIReader::TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, SubBlockInfo& info)
{
	this->ThrowIfNotOperational();

	// TODO: we should be able to gather this information when constructing the subblock-list
	//  for the time being... just walk through the whole list
	//  
	bool foundASubBlock = false;
	SubBlockStatistics s = this->subBlkDir.GetStatistics();
	if (!s.dimBounds.IsValid(DimensionIndex::C))
	{
		// in this case -> just take the first subblock...
		this->EnumerateSubBlocks(
			[&](int index, const SubBlockInfo& sbinfo)->bool
		{
			info = sbinfo;
			foundASubBlock = true;
			return false;
		});
	}
	else
	{
		this->EnumerateSubBlocks(
			[&](int index, const SubBlockInfo& sbinfo)->bool
		{
			int c;
			if (sbinfo.coordinate.TryGetPosition(DimensionIndex::C, &c) == true && c == channelIndex)
			{
				info = sbinfo;
				foundASubBlock = true;
				return false;
			}

			return true;
		});
	}

	return foundASubBlock;
}

/*virtual*/bool CCZIReader::TryGetSubBlockInfo(int index, SubBlockInfo* info) const
{
	CCziSubBlockDirectory::SubBlkEntry entry;
	if (this->subBlkDir.TryGetSubBlock(index, entry) == false)
	{
		return false;
	}

	if (info != nullptr)
	{
		info->compressionModeRaw = entry.Compression;
		info->pixelType = CziUtils::PixelTypeFromInt(entry.PixelType);
		info->coordinate = entry.coordinate;
		info->logicalRect = IntRect{ entry.x,entry.y,entry.width,entry.height };
		info->physicalSize = IntSize{ static_cast<std::uint32_t>(entry.storedWidth), static_cast<std::uint32_t>(entry.storedHeight) };
		info->mIndex = entry.mIndex;
	}

	return true;
}

/*virtual*/std::shared_ptr<libCZI::IAccessor> CCZIReader::CreateAccessor(libCZI::AccessorType accessorType)
{
	this->ThrowIfNotOperational();
	return CreateAccesor(this->shared_from_this(), accessorType);
}

/*virtual*/void CCZIReader::Close()
{
	this->ThrowIfNotOperational();
	this->SetOperationalState(false);
	this->stream.reset();
}

/*virtual*/void CCZIReader::EnumerateAttachments(const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum)
{
	this->ThrowIfNotOperational();
	libCZI::AttachmentInfo ai;
	ai.contentFileType[sizeof(ai.contentFileType) - 1] = '\0';
	this->attachmentDir.EnumAttachments(
		[&](int index, const CCziAttachmentsDirectory::AttachmentEntry& ae)
	{
		ai.contentGuid = ae.ContentGuid;
		memcpy(ai.contentFileType, ae.ContentFileType, sizeof(ae.ContentFileType));
		ai.name = ae.Name;
		bool b = funcEnum(index, ai);
		return b;
	});
}

/*virtual*/void CCZIReader::EnumerateSubset(const char* contentFileType, const char* name, const std::function<bool(int index, const libCZI::AttachmentInfo& infi)>& funcEnum)
{
	this->ThrowIfNotOperational();
	libCZI::AttachmentInfo ai;
	ai.contentFileType[sizeof(ai.contentFileType) - 1] = '\0';
	this->attachmentDir.EnumAttachments(
		[&](int index, const CCziAttachmentsDirectory::AttachmentEntry& ae)
	{
		if (contentFileType == nullptr || strcmp(contentFileType, ae.ContentFileType) == 0)
		{
			if (name == nullptr || strcmp(name, ae.Name) == 0)
			{
				ai.contentGuid = ae.ContentGuid;
				memcpy(ai.contentFileType, ae.ContentFileType, sizeof(ae.ContentFileType));
				ai.name = ae.Name;
				bool b = funcEnum(index, ai);
				return b;
			}
		}

		return true;
	});
}

/*virtual*/std::shared_ptr<libCZI::IAttachment> CCZIReader::ReadAttachment(int index)
{
	this->ThrowIfNotOperational();
	CCziAttachmentsDirectory::AttachmentEntry entry;
	if (this->attachmentDir.TryGetAttachment(index, entry) == false)
	{
		return std::shared_ptr<IAttachment>();
	}

	return this->ReadAttachment(entry);
}

std::shared_ptr<ISubBlock> CCZIReader::ReadSubBlock(const CCziSubBlockDirectory::SubBlkEntry& entry)
{
	CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

	auto subBlkData = CCZIParse::ReadSubBlock(this->stream.get(), entry.FilePosition, allocateInfo);

	libCZI::SubBlockInfo info;
	info.pixelType = CziUtils::PixelTypeFromInt(subBlkData.pixelType);
	info.compressionModeRaw = subBlkData.compression;
	info.coordinate = subBlkData.coordinate;
	info.mIndex = subBlkData.mIndex;
	info.logicalRect = subBlkData.logicalRect;
	info.physicalSize = subBlkData.physicalSize;

	return std::make_shared<CCziSubBlock>(info, subBlkData, free);
}

std::shared_ptr<libCZI::IAttachment> CCZIReader::ReadAttachment(const CCziAttachmentsDirectory::AttachmentEntry& entry)
{
	CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

	auto attchmnt = CCZIParse::ReadAttachment(this->stream.get(), entry.FilePosition, allocateInfo);
	libCZI::AttachmentInfo attchmentInfo;
	attchmentInfo.contentGuid = entry.ContentGuid;
	static_assert(sizeof(attchmentInfo.contentFileType) > sizeof(entry.ContentFileType), "sizeof(attchmentInfo.contentFileType) must be greater than sizeof(entry.ContentFileType)");
	memcpy(attchmentInfo.contentFileType, entry.ContentFileType, sizeof(entry.ContentFileType));
	attchmentInfo.contentFileType[sizeof(entry.ContentFileType)] = '\0';
	attchmentInfo.name = entry.Name;

	return std::make_shared<CCziAttachment>(attchmentInfo, attchmnt, allocateInfo.free);
}

std::shared_ptr<libCZI::IMetadataSegment> CCZIReader::ReadMetadataSegment(std::uint64_t position)
{
	CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

	auto metaDataSegmentData = CCZIParse::ReadMetadataSegment(this->stream.get(), position, allocateInfo);
	return std::make_shared<CCziMetadataSegment>(metaDataSegmentData, free);
}

void CCZIReader::ThrowIfNotOperational()
{
	if (this->isOperational == false)
	{
		throw logic_error("CZIReader is not operational (must call 'Open' first)");
	}
}

void CCZIReader::SetOperationalState(bool operational)
{
	this->isOperational = operational;
}