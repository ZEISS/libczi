// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <functional>
#include <set>
#include <map>

class CCziAttachmentsDirectoryBase
{
public:
	struct AttachmentEntry
	{
		std::int64_t FilePosition;
		GUID ContentGuid;
		char ContentFileType[8];
		char Name[80];
	};

	struct AttachmentEntryEx : AttachmentEntry
	{
		std::int64_t allocatedSize;
	};

	static bool CompareForEquality_Id(const AttachmentEntry&a, const AttachmentEntry&b);
};

class CCziAttachmentsDirectory :public CCziAttachmentsDirectoryBase
{
private:
	std::vector<AttachmentEntry> attachmentEntries;
public:
	CCziAttachmentsDirectory() = default;
	explicit CCziAttachmentsDirectory(size_t initialCnt)
	{
		this->attachmentEntries.reserve(initialCnt);
	}

	void AddAttachmentEntry(const AttachmentEntry& entry);
	void EnumAttachments(std::function<bool(int index, const CCziAttachmentsDirectory::AttachmentEntry&)> func);
	bool TryGetAttachment(int index, AttachmentEntry& entry);
};

class CWriterCziAttachmentsDirectory : public CCziAttachmentsDirectoryBase
{
public:
	bool TryAddAttachment(const AttachmentEntry& entry);
	int GetAttachmentCount() const;
	bool EnumEntries(const std::function<bool(int index, const AttachmentEntry&)>& func) const;
private:
	struct AttachmentEntriesCompare {
		bool operator() (const AttachmentEntry& a, const AttachmentEntry& b) const;
	};

	std::set<AttachmentEntry, AttachmentEntriesCompare> attachments;
};

class CReaderWriterCziAttachmentsDirectory : public CCziAttachmentsDirectoryBase
{
private:
	int nextAttchmntIndex;
	std::map<int, AttachmentEntry> attchmnts;
	bool isModified;
public:
	CReaderWriterCziAttachmentsDirectory() : nextAttchmntIndex(0), isModified(false) {}

	bool IsModified()const { return this->isModified; }
	void SetModified(bool modified) { this->isModified = modified; }

	size_t GetEntryCnt() const;
	bool EnumEntries(const std::function<bool(int index, const AttachmentEntry&)>& func) const;

	void AddAttachment(const AttachmentEntry& entry, int* key = nullptr);

	bool TryGetAttachment(int key, AttachmentEntry* attchmntEntry);

	bool TryModifyAttachment(int key, const AttachmentEntry& attchmntEntry);

	bool TryRemoveAttachment(int key, AttachmentEntry* attchmntEntry);

	bool TryAddAttachment(const AttachmentEntry& entry, int* key);
};