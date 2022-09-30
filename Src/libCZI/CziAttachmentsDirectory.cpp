// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CziAttachmentsDirectory.h"

/*static*/bool CCziAttachmentsDirectoryBase::CompareForEquality_Id(const AttachmentEntry&a, const AttachmentEntry&b)
{
	int r = memcmp(&a.ContentGuid, &b.ContentGuid, sizeof(GUID));
	if (r!=0)
	{
		return false;
	}

	r = strncmp(a.Name, b.Name, sizeof(a.Name));
	if (r != 0)
	{
		return false;
	}

	r = strncmp(a.ContentFileType, b.ContentFileType, sizeof(a.ContentFileType));
	if (r != 0)
	{
		return false;
	}

	return true;
}

void CCziAttachmentsDirectory::AddAttachmentEntry(const AttachmentEntry& entry)
{
	this->attachmentEntries.emplace_back(entry);
}

void CCziAttachmentsDirectory::EnumAttachments(std::function<bool(int index, const CCziAttachmentsDirectory::AttachmentEntry&)> func)
{
	int i = 0;
	for (const auto& ae : this->attachmentEntries)
	{
		const bool b = func(i, ae);
		if (b != true)
		{
			break;
		}

		++i;
	}
}

bool CCziAttachmentsDirectory::TryGetAttachment(int index, AttachmentEntry& entry)
{
	if (index < (int)this->attachmentEntries.size())
	{
		entry = this->attachmentEntries.at(index);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CWriterCziAttachmentsDirectory::TryAddAttachment(const AttachmentEntry& entry)
{
	auto insert = this->attachments.insert(entry);
	return insert.second;
}

bool CWriterCziAttachmentsDirectory::EnumEntries(const std::function<bool(int index, const AttachmentEntry&)>& func) const
{
	int index = 0;
	for (auto it = this->attachments.cbegin(); it != this->attachments.cend(); ++it)
	{
		if (!func(index++, *it))
		{
			return false;
		}
	}

	return true;
}

int CWriterCziAttachmentsDirectory::GetAttachmentCount() const
{
	return (int)this->attachments.size();
}

bool CWriterCziAttachmentsDirectory::AttachmentEntriesCompare::operator()(const AttachmentEntry & a, const AttachmentEntry & b) const
{
	// we shall return true if a is considered to go before b in the strict weak ordering the function defines
	int r = memcmp(&a.ContentGuid, &b.ContentGuid, sizeof(GUID));
	if (r < 0)
	{
		return true;
	}
	else if (r > 0)
	{
		return false;
	}

	r = memcmp(&a.ContentFileType, &b.ContentFileType, sizeof(a.ContentFileType));
	if (r < 0)
	{
		return true;
	}
	else if (r > 0)
	{
		return false;
	}

	r = memcmp(&a.Name, &b.Name, sizeof(a.Name));
	if (r < 0)
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

void CReaderWriterCziAttachmentsDirectory::AddAttachment(const AttachmentEntry& entry, int* key)
{
	this->attchmnts.insert(std::pair<int, AttachmentEntry>(this->nextAttchmntIndex, entry));
	if (key != nullptr)
	{
		*key = this->nextAttchmntIndex;
	}

	this->SetModified(true);
	this->nextAttchmntIndex++;
}

bool CReaderWriterCziAttachmentsDirectory::EnumEntries(const std::function<bool(int index, const AttachmentEntry&)>& func) const
{
	for (auto it = this->attchmnts.cbegin(); it != this->attchmnts.cend(); ++it)
	{
		const bool b = func(it->first, it->second);
		if (!b)
		{
			return false;
		}
	}

	return true;
}

size_t CReaderWriterCziAttachmentsDirectory::GetEntryCnt() const
{
	return this->attchmnts.size();
}

bool CReaderWriterCziAttachmentsDirectory::TryGetAttachment(int key, AttachmentEntry* attchmntEntry)
{
	const auto it = this->attchmnts.find(key);
	if (it == this->attchmnts.cend())
	{
		return false;
	}

	if (attchmntEntry != nullptr)
	{
		*attchmntEntry = it->second;
	}

	return true;
}

bool CReaderWriterCziAttachmentsDirectory::TryModifyAttachment(int key, const AttachmentEntry& attchmntEntry)
{
	auto it = this->attchmnts.find(key);
	if (it == this->attchmnts.end())
	{
		return false;
	}

	it->second = attchmntEntry;
	this->SetModified(true);
	return true;
}

bool CReaderWriterCziAttachmentsDirectory::TryRemoveAttachment(int key, AttachmentEntry* attchmntEntry)
{
	auto it = this->attchmnts.find(key);
	if (it == this->attchmnts.end())
	{
		return false;
	}

	if (attchmntEntry != nullptr)
	{
		*attchmntEntry = it->second;
	}

	this->attchmnts.erase(it);
	this->SetModified(true);
	return true;
}

bool CReaderWriterCziAttachmentsDirectory::TryAddAttachment(const AttachmentEntry& entry, int* key)
{
	for (auto it = this->attchmnts.cbegin(); it != this->attchmnts.cend(); ++it)
	{
		if (CCziAttachmentsDirectoryBase::CompareForEquality_Id(it->second, entry))
		{
			return false;
		}
	}

	this->AddAttachment(entry, key);
	return true;
}
