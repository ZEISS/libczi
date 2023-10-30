// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include <tuple>
#include "libCZI.h"
#include "CziSubBlockDirectory.h"
#include "CziAttachmentsDirectory.h"
#include "CziStructs.h"
#include "CziWriter.h"
#include "libCZI_ReadWrite.h"
#include "FileHeaderSegmentData.h"
#include "CziParse.h"

class CCziReaderWriter : public libCZI::ICziReaderWriter
{
private:
    std::shared_ptr<libCZI::IInputOutputStream> stream;
    std::shared_ptr<libCZI::ICziReaderWriterInfo> info;

    CFileHeaderSegmentData hdrSegmentData;
    CReaderWriterCziSubBlockDirectory sbBlkDirectory;
    CReaderWriterCziAttachmentsDirectory attachmentDirectory;

public:
    void Create(std::shared_ptr<libCZI::IInputOutputStream> stream, std::shared_ptr<libCZI::ICziReaderWriterInfo> info) override;
    void ReplaceSubBlock(int key, const libCZI::AddSubBlockInfo& addSbBlkInfo) override;
    void RemoveSubBlock(int key) override;
    void ReplaceAttachment(int attchmntId, const libCZI::AddAttachmentInfo& addAttachmentInfo) override;
    void RemoveAttachment(int attchmntId) override;
    void SyncAddSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo) override;
    void SyncAddAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo) override;
    void SyncWriteMetadata(const libCZI::WriteMetadataInfo& metadataInfo) override;
    std::shared_ptr<libCZI::IMetadataSegment> ReadMetadataSegment() override;
    libCZI::FileHeaderInfo GetFileHeaderInfo() override;

    void Close() override;

    // interface ISubBlockRepository
    void EnumerateSubBlocks(const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum) override;
    void EnumSubset(const libCZI::IDimCoordinate* planeCoordinate, const libCZI::IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum) override;
    std::shared_ptr<libCZI::ISubBlock> ReadSubBlock(int index) override;
    bool TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, libCZI::SubBlockInfo& info) override;
    bool TryGetSubBlockInfo(int index, libCZI::SubBlockInfo* info) const override;
    libCZI::SubBlockStatistics GetStatistics() override;
    libCZI::PyramidStatistics GetPyramidStatistics() override;

    // interface IAttachmentRepository
    void EnumerateAttachments(const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum) override;
    void EnumerateSubset(const char* contentFileType, const char* name, const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum) override;
    std::shared_ptr<libCZI::IAttachment> ReadAttachment(int index) override;

private:
    void Finish();

    void ReadCziStructure();
    libCZI::GUID UpdateFileHeaderGuid();
    void DetermineNextSubBlockOffset();

    std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> ReplaceSubBlock(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry);
    std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> ReplaceSubBlockAddNewAtEnd(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry);
    std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> ReplaceSubBlockInplace(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry, std::uint64_t existingSegmentAllocatedSize);

    std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> ReplaceAttachment(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo);
    std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> ReplaceAttachmentAddNewAtEnd(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo);
    std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> ReplaceAttachmentInplace(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo, std::uint64_t existingSegmentAllocatedSize);

    std::shared_ptr<libCZI::IAttachment> ReadAttachment(const CCziAttachmentsDirectoryBase::AttachmentEntry& entry);

    CCZIParse::SegmentSizes ReadSegmentHdrOfSubBlock(std::uint64_t pos);
    CCZIParse::SegmentSizes ReadSegmentHdrOfAttachment(std::uint64_t pos);
    void EnsureNextSegmentInfo();

    void WriteToOutputStream(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite);
    static void ThrowNotEnoughDataWritten(std::uint64_t offset, std::uint64_t bytesToWrite, std::uint64_t bytesActuallyWritten);

    void UpdateFileHeader();
    void ThrowIfNotOperational() const;
    void ThrowIfAlreadyInitialized() const;
private:
    class CNextSegment
    {
    private:
        bool lastSegmentPosValid;
        std::uint64_t lastSegmentPos;   ///< file-position of the _start_ of the last segment

        bool nextSegmentPosValid;
        std::uint64_t nextSegmentPos;   ///< file-position of the _next_ segment
    public:
        CNextSegment() :lastSegmentPosValid(false), nextSegmentPosValid(false)
        {}

        /// Sets the position of the last segment (we store the _start_ of the last segment).
        ///
        /// \param lastSegmentPos The _start_ of the last segment in the CZI.
        void SetLastSegmentPos(std::uint64_t lastSegmentPos)
        {
            this->lastSegmentPos = lastSegmentPos;
            this->lastSegmentPosValid = true;
        }

        /// Query if the last-segment-position is valid.
        ///
        /// \return True if last-segment-position is valid, false if not.
        bool IsLastSegmentPosValid() const { return this->lastSegmentPosValid; }

        /// Gets the start of the last segment in the CZI. Note that the value returned is only to be considered valid
        /// if "sLastSegmentPosValid" returns true.
        ///
        /// \return The start of the last segment if valid (as determined by IsLastSegmentPosValid), otherwise some bogus number.
        std::uint64_t GetLastSegmentPos() const { return this->lastSegmentPos; }


        bool IsNextSegmentPosValid() const { return this->nextSegmentPosValid; }

        void SetNextSegmentPos(std::uint64_t nextSegmentPos)
        {
            this->nextSegmentPos = nextSegmentPos;
            this->nextSegmentPosValid = true;
        }

        std::uint64_t GetNextSegmentPos() const { return this->nextSegmentPos; }
    };

    CNextSegment    nextSegmentInfo;

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

    WrittenSegmentInfo  metadataSegment;
    WrittenSegmentInfo  subBlockDirectorySegment;
    WrittenSegmentInfo  attachmentDirectorySegment;
};
