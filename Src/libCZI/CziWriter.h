// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "CziSubBlockDirectory.h"
#include "CziAttachmentsDirectory.h"
#include "CziStructs.h"
#include "CziUtils.h"

/// This utility class is used to implement the "AddSubBlock"-method with arguments of type "AddSubBlockInfoMemPtr",
/// "AddSubBlockInfoLinewiseBitmap" or "AddSubBlockInfoStridedBitmap" on top of an operation operating with "AddSubBlockInfo".
class AddSubBlockHelper
{
private:
	static bool SetIfCallCountZero(int callCnt, const void* ptr, size_t size, const void*& dstPtr, size_t& dstSize)
	{
		if (callCnt == 0)
		{
			dstPtr = ptr;
			dstSize = size;
			return true;
		}

		return false;
	}
public:
	template <class T>
	static void SyncAddSubBlock(T& t, const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfo)
	{
		libCZI::AddSubBlockInfo addSbInfo(addSbBlkInfo);
		addSbInfo.sizeData = addSbBlkInfo.dataSize;
		addSbInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbBlkInfo.ptrData, addSbBlkInfo.dataSize, ptr, size);
		};

		addSbInfo.sizeAttachment = addSbBlkInfo.sbBlkAttachmentSize;
		addSbInfo.getAttachment = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbBlkInfo.ptrSbBlkAttachment, addSbBlkInfo.sbBlkAttachmentSize, ptr, size);
		};

		addSbInfo.sizeMetadata = addSbBlkInfo.sbBlkMetadataSize;
		addSbInfo.getMetaData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbBlkInfo.ptrSbBlkMetadata, addSbBlkInfo.sbBlkMetadataSize, ptr, size);
		};

		return t.operator()(addSbInfo);
	}

	template <class T>
	static void SyncAddSubBlock(T& t, const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise)
	{
		libCZI::AddSubBlockInfo addSbInfo(addSbInfoLinewise);

		size_t stride = addSbInfoLinewise.physicalWidth * CziUtils::GetBytesPerPel(addSbInfoLinewise.PixelType);
		addSbInfo.sizeData = addSbInfoLinewise.physicalHeight*stride;
		auto linesCnt = addSbInfoLinewise.physicalHeight;
		addSbInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			if (callCnt < linesCnt)
			{
				ptr = addSbInfoLinewise.getBitmapLine(callCnt);
				size = stride;
				return true;
			}

			return false;
		};

		addSbInfo.sizeAttachment = addSbInfoLinewise.sbBlkAttachmentSize;
		addSbInfo.getAttachment = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbInfoLinewise.ptrSbBlkAttachment, addSbInfoLinewise.sbBlkAttachmentSize, ptr, size);
		};

		addSbInfo.sizeMetadata = addSbInfoLinewise.sbBlkMetadataSize;
		addSbInfo.getMetaData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbInfoLinewise.ptrSbBlkMetadata, addSbInfoLinewise.sbBlkMetadataSize, ptr, size);
		};

		return t.operator()(addSbInfo);
	}

	template <class T>
	static void SyncAddSubBlock(T& t, const libCZI::AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap)
	{
		libCZI::AddSubBlockInfo addSbInfo(addSbBlkInfoStrideBitmap);

		addSbInfo.sizeData = addSbBlkInfoStrideBitmap.physicalHeight * addSbBlkInfoStrideBitmap.strideBitmap;
		addSbInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			if (callCnt < addSbBlkInfoStrideBitmap.physicalHeight)
			{
				ptr = static_cast<const char*>(addSbBlkInfoStrideBitmap.ptrBitmap) + callCnt * addSbBlkInfoStrideBitmap.strideBitmap;
				size = addSbBlkInfoStrideBitmap.strideBitmap;
				return true;
			}

			return false;
		};

		addSbInfo.sizeAttachment = addSbBlkInfoStrideBitmap.sbBlkAttachmentSize;
		addSbInfo.getAttachment = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbBlkInfoStrideBitmap.ptrSbBlkAttachment, addSbBlkInfoStrideBitmap.sbBlkAttachmentSize, ptr, size);
		};

		addSbInfo.sizeMetadata = addSbBlkInfoStrideBitmap.sbBlkMetadataSize;
		addSbInfo.getMetaData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
		{
			return SetIfCallCountZero(callCnt, addSbBlkInfoStrideBitmap.ptrSbBlkMetadata, addSbBlkInfoStrideBitmap.sbBlkMetadataSize, ptr, size);
		};

		return t.operator()(addSbInfo);
	}
};

/// Utility functions for writing the parts of a CZI-file. The functions are used by the writer- and the reader/writer-implementation commonly.
class CWriterUtils
{
public:
	struct WriteInfo
	{
		std::uint64_t			segmentPos;
		std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)> writeFunc;
		bool					useSpecifiedAllocatedSize;
		std::uint64_t			specifiedAllocatedSize;
	};
	static std::uint64_t WriteSubBlock(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo);
	static std::uint64_t WriteAttachment(const WriteInfo& info, const libCZI::AddAttachmentInfo& addAttchmntInfo);

	struct MetadataWriteInfo
	{
		bool					markAsDeletedIfExistingSegmentIsNotUsed;
		std::uint64_t			existingSegmentPos;
		size_t                  sizeExistingSegmentPos;

		std::uint64_t           segmentPosForNewSegment;

		std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)> writeFunc;
	};
	static std::tuple<std::uint64_t, std::uint64_t> WriteMetadata(const MetadataWriteInfo& info, const libCZI::WriteMetadataInfo& metadataInfo);

	struct SubBlkDirWriteInfo
	{
		bool					markAsDeletedIfExistingSegmentIsNotUsed;
		std::uint64_t			existingSegmentPos;
		size_t                  sizeExistingSegmentPos;

		std::uint64_t           segmentPosForNewSegment;

		std::function<void(const std::function<void(size_t , const CCziSubBlockDirectoryBase::SubBlkEntry&)>&)> enumEntriesFunc;

		std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)> writeFunc;
	};
	static std::tuple<std::uint64_t, std::uint64_t> WriteSubBlkDirectory(const SubBlkDirWriteInfo& info);
	
	struct AttachmentDirWriteInfo
	{
		bool					markAsDeletedIfExistingSegmentIsNotUsed;
		std::uint64_t			existingSegmentPos;
		size_t                  sizeExistingSegmentPos;

		std::uint64_t           segmentPosForNewSegment;

		size_t					entryCnt;
		std::function<void(const std::function<void(size_t, const CCziAttachmentsDirectoryBase::AttachmentEntry&)>&)> enumEntriesFunc;

		std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)> writeFunc;
	};
	static std::tuple<std::uint64_t, std::uint64_t> WriteAttachmentDirectory(const AttachmentDirWriteInfo& info);

	struct MarkDeletedInfo
	{
		std::uint64_t	segmentPos;
		std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)> writeFunc;
	};
	static void WriteDeletedSegment(const MarkDeletedInfo& info);

	static void CheckAddSubBlockArguments(const libCZI::AddSubBlockInfo& addSbBlkInfo);
	static CCziSubBlockDirectoryBase::SubBlkEntry SubBlkEntryFromAddSubBlockInfo(const libCZI::AddSubBlockInfo& addSbBlkInfo);
	static void CheckAddAttachmentArguments(const libCZI::AddAttachmentInfo& addAttachmentInfo);
	static CCziAttachmentsDirectoryBase::AttachmentEntry AttchmntEntryFromAddAttachmentInfo(const libCZI::AddAttachmentInfo& addAttachmentInfo);
	static void CheckWriteMetadataArguments(const libCZI::WriteMetadataInfo& metadataInfo);

	static bool CalculateSegmentDataSize(const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t* pAllocatedSize, std::uint64_t* pUsedSize);
	static bool CalculateSegmentDataSize(const libCZI::AddAttachmentInfo& addAttchmntInfo, std::uint64_t* pAllocatedSize, std::uint64_t* pUsedSize);

	static std::uint64_t AlignSegmentSize(std::uint64_t usedSize);
private:
	static size_t CalcSubBlockSegmentDataSize(const libCZI::AddSubBlockInfo& addSbBlkInfo);
	static size_t CalcSubBlockDirectoryEntryDVSize(const libCZI::AddSubBlockInfo& addSbBlkInfo);
	static int CalcCountOfDimensionsEntriesInDirectoryEntryDV(const libCZI::AddSubBlockInfo& addSbBlkInfo);

	static void FillSubBlockSegment(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegment* sbBlkSegment);
	static bool SetAllocatedAndUsedSize(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegment* pSbBlkSegment);
	static void FillSubBlockSegmentData(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegmentData* ptr);
	static void FillSubBlockDirectoryEntryDv(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockDirectoryEntryDV* ptr);
	static size_t FillSubBlockDirectoryEntryDV(SubBlockDirectoryEntryDV* ptr, const CCziSubBlockDirectoryBase::SubBlkEntry& entry);
	static void SetDimensionInDimensionEntry(DimensionEntry* de, char c);

	static std::uint64_t WriteZeroes(const WriteInfo& info, std::uint64_t filePos, std::uint64_t count);
	static std::uint64_t WriteZeroes(const std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)>& writeFunc, std::uint64_t filePos, std::uint64_t count);
	static size_t WriteSubBlockSegment(const WriteInfo& info, const void* ptrData, size_t dataSize, std::uint64_t filePos);
	static size_t WriteSubBlkMetaData(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos);
	static size_t WriteSubBlkDataGeneric(const WriteInfo& info, std::uint64_t filePos, size_t size, std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& sizePtr)> getFunc, const char* nameOfPartToWrite);
	static size_t WriteSubBlkData(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos);
	static size_t WriteSubBlkAttachment(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos);

	static size_t CalcSizeOfSubBlockDirectoryEntryDV(const CCziSubBlockDirectoryBase::SubBlkEntry& entry);
	static int CalcCountOfDimensionsEntriesInDirectoryEntryDV(const CCziSubBlockDirectoryBase::SubBlkEntry& entry);
};

class CCziWriter : public libCZI::ICziWriter
{
private:
	CWriterCziSubBlockDirectory sbBlkDirectory;
	CWriterCziAttachmentsDirectory attachmentDirectory;
	std::shared_ptr<libCZI::IOutputStream> stream;
	std::shared_ptr<libCZI::ICziWriterInfo> info;

	std::uint64_t nextSegmentPos;

	class CziWriterInfoWrapper : public libCZI::ICziWriterInfo
	{
	private:
		std::shared_ptr<libCZI::ICziWriterInfo> writerInfo;
		GUID fileGuid;
	public:
		CziWriterInfoWrapper(std::shared_ptr<libCZI::ICziWriterInfo> writerInfo);

		const libCZI::IDimBounds* GetDimBounds() const override { return this->writerInfo->GetDimBounds(); }
		const GUID& GetFileGuid() const override { return this->fileGuid; }
		bool TryGetMIndexMinMax(int* min, int* max) const override { return this->writerInfo->TryGetMIndexMinMax(min, max); }
		bool TryGetReservedSizeForAttachmentDirectory(size_t* size) const override { return this->writerInfo->TryGetReservedSizeForAttachmentDirectory(size); }
		bool TryGetReservedSizeForSubBlockDirectory(size_t* size) const override { return this->writerInfo->TryGetReservedSizeForSubBlockDirectory(size); }
		bool TryGetReservedSizeForMetadataSegment(size_t* size) const override { return this->writerInfo->TryGetReservedSizeForMetadataSegment(size); }
	};
public:
	CCziWriter();

	void Create(std::shared_ptr<libCZI::IOutputStream> stream, std::shared_ptr<libCZI::ICziWriterInfo> info) override;
	virtual ~CCziWriter();

	virtual void SyncAddSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo) override;
	virtual void SyncAddAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo) override;
	virtual void SyncWriteMetadata(const libCZI::WriteMetadataInfo& metadataInfo) override;
	virtual std::shared_ptr<libCZI::ICziMetadataBuilder> GetPreparedMetadata(const libCZI::PrepareMetadataInfo& info) override;
	virtual void Close() override;

private:
	void WriteSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo);

	void WriteAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo);

	// tuple: first item is the filepos, second is the allocatedSize (excluding SegmentHeader)
	std::tuple<std::uint64_t, std::uint64_t > WriteMetadata(const libCZI::WriteMetadataInfo& metadataInfo);
	std::tuple<std::uint64_t, std::uint64_t>  WriteCurrentSubBlkDirectory();

	void WriteSubBlkDirectory();
	void WriteAttachmentDirectory();
	std::tuple<std::uint64_t, std::uint64_t>  WriteCurrentAttachmentsDirectory();

	void WriteToOutputStream(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite = nullptr);
	void ThrowNotEnoughDataWritten(std::uint64_t offset, std::uint64_t bytesToWrite, std::uint64_t bytesActuallyWritten);
	void ThrowIfCoordinateIsOutOfBounds(const libCZI::AddSubBlockInfo& addSbBlkInfo) const;

	enum class SbBlkCoordinateCheckResult
	{
		Ok,
		OutOfBounds,
		InsufficientCoordinate,
		UnexpectedCoordinate
	};

	SbBlkCoordinateCheckResult CheckCoordinate(const libCZI::AddSubBlockInfo& addSbBlkInfo) const;
	static std::tuple<std::string, std::tuple<bool, std::string>> DefaultGenerateChannelIdAndName(int chIdx);

private:
	struct FileHeaderData
	{
		GUID primaryFileGuid;
		std::uint64_t subBlockDirectoryPosition;
		std::uint64_t metadataPosition;
		std::uint64_t attachmentDirectoryPosition;

		void Clear() { memset(this, 0, sizeof(*this)); }
	};

	void WriteFileHeader(const FileHeaderData& fhd);

	void Finish();

	void ThrowIfNotOperational();
	void ThrowIfAlreadyInitialized();

	void ReserveMetadataSegment(size_t s);
	void ReserveSubBlockDirectory(size_t s);
	void ReserveAttachmentDirectory(size_t s);

private:
	class WrittenSegmentInfo
	{
		bool isValid;
		std::uint64_t filePos;
		std::uint64_t allocatedSize;
		bool isMarkedAsDeleted;
	public:
		WrittenSegmentInfo() : isValid(false) {}
		void Invalidate() { this->isValid = false; }
		bool IsValid() const { return this->isValid; }
		void SetPositionAndAllocatedSize(std::uint64_t filePos, std::uint64_t allocatedSize, bool isMarkedAsDeleted)
		{
			this->filePos = filePos;
			this->allocatedSize = allocatedSize;
			this->isMarkedAsDeleted = isMarkedAsDeleted;
			this->isValid = true;
		}
		std::uint64_t GetFilePos()const { return this->filePos; }
		std::uint64_t GetAllocatedSize()const { return this->allocatedSize; }
		bool GetIsMarkedAsDeleted()const { return this->isMarkedAsDeleted; }
	};

	WrittenSegmentInfo	metadataSegment;
	WrittenSegmentInfo	subBlockDirectorySegment;
	WrittenSegmentInfo	attachmentDirectorySegment;
};
