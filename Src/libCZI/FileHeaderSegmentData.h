// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "CziStructs.h"

class CFileHeaderSegmentData
{
private:
	int verMajor, verMinor;
	GUID fileGuid;
	std::uint64_t subBlockDirectoryPosition;
	std::uint64_t attachmentDirectoryPosition;
	std::uint64_t metadataPosition;
public:
	CFileHeaderSegmentData()
		:verMajor(-1),
		verMinor(-1),
		fileGuid({ 0, 0, 0, { 0,0,0,0,0,0,0,0 } }),
		subBlockDirectoryPosition((std::numeric_limits<decltype(subBlockDirectoryPosition)>::max)()),
		attachmentDirectoryPosition((std::numeric_limits<decltype(subBlockDirectoryPosition)>::max)()),
		metadataPosition((std::numeric_limits<decltype(subBlockDirectoryPosition)>::max)())
	{
		
	}

	CFileHeaderSegmentData(const FileHeaderSegmentData* hdrSegmentData) :
		verMajor(hdrSegmentData->Major),
		verMinor(hdrSegmentData->Minor),
		fileGuid(hdrSegmentData->FileGuid),
		subBlockDirectoryPosition(hdrSegmentData->SubBlockDirectoryPosition),
		attachmentDirectoryPosition(hdrSegmentData->AttachmentDirectoryPosition),
		metadataPosition(hdrSegmentData->MetadataPosition)
	{}

	void GetVersion(int* ptrMajor, int* ptrMinor) const
	{
		if (ptrMajor != nullptr) { *ptrMajor = this->verMajor; }
		if (ptrMinor != nullptr) { *ptrMinor = this->verMinor; }
	}

	std::uint64_t GetSubBlockDirectoryPosition() const { return this->subBlockDirectoryPosition; }
	std::uint64_t GetAttachmentDirectoryPosition() const { return this->attachmentDirectoryPosition; }
	std::uint64_t GetMetadataPosition() const { return this->metadataPosition; }
	GUID GetFileGuid()const { return this->fileGuid; }

	bool GetIsSubBlockDirectoryPositionValid()const
	{
		return  this->subBlockDirectoryPosition != (std::numeric_limits<decltype(subBlockDirectoryPosition)>::max)() && this->subBlockDirectoryPosition != 0;
	}

	bool GetIsAttachmentDirectoryPositionValid()const
	{
		return  this->attachmentDirectoryPosition != (std::numeric_limits<decltype(attachmentDirectoryPosition)>::max)() && this->attachmentDirectoryPosition != 0;
	}

	bool GetIsMetadataPositionPositionValid()const
	{
		return  this->metadataPosition != (std::numeric_limits<decltype(metadataPosition)>::max)() && this->metadataPosition != 0;
	}
};
