// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CziReaderWriter.h"
#include "CziMetadataSegment.h"
#include "libCZI_Utilities.h"
#include "CziUtils.h"
#include "libCZI_exceptions.h"
#include "CziMetadataBuilder.h"
#include "utilities.h"
#include "CziSubBlock.h"
#include "CziAttachment.h"

using namespace libCZI;
using namespace std;

struct AddHelper
{
    ICziReaderWriter* t;
    AddHelper(ICziReaderWriter* t) :t(t)
    {}

    void operator()(const AddSubBlockInfo& addSbBlkInfo) const
    {
        this->t->SyncAddSubBlock(addSbBlkInfo);
    }
};

struct ReplaceHelper
{
    int key;
    ICziReaderWriter* t;
    ReplaceHelper(int key, ICziReaderWriter* t)
        :key(key), t(t) {}

    void operator()(const AddSubBlockInfo& addSbBlkInfo) const
    {
        this->t->ReplaceSubBlock(this->key, addSbBlkInfo);
    }
};

void ICziReaderWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfo)
{
    AddHelper f(this);
    AddSubBlockHelper::SyncAddSubBlock<AddHelper>(f, addSbBlkInfo);
}
void ICziReaderWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise)
{
    AddHelper f(this);
    AddSubBlockHelper::SyncAddSubBlock<AddHelper>(f, addSbInfoLinewise);
}
void ICziReaderWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap)
{
    AddHelper f(this);
    AddSubBlockHelper::SyncAddSubBlock<AddHelper>(f, addSbBlkInfoStrideBitmap);
}

void ICziReaderWriter::ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfoMemPtr)
{
    struct ReplaceHelper f(key, this);
    AddSubBlockHelper::SyncAddSubBlock<ReplaceHelper >(f, addSbBlkInfoMemPtr);
}
void ICziReaderWriter::ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise)
{
    struct ReplaceHelper f(key, this);
    AddSubBlockHelper::SyncAddSubBlock<ReplaceHelper >(f, addSbInfoLinewise);
}
void ICziReaderWriter::ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap)
{
    struct ReplaceHelper f(key, this);
    AddSubBlockHelper::SyncAddSubBlock<ReplaceHelper >(f, addSbBlkInfoStrideBitmap);
}

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

/*virtual*/void CCziReaderWriter::Create(std::shared_ptr<libCZI::IInputOutputStream> stream, std::shared_ptr<libCZI::ICziReaderWriterInfo> info)
{
    this->ThrowIfAlreadyInitialized();
    if (!info)
    {
        this->info = make_shared<CCziReaderWriterInfo>();
    }
    else
    {
        this->info = info;
    }

    this->stream = stream;
    try
    {
        this->ReadCziStructure();
    }
    catch (...)
    {
        this->stream.reset();
        this->info.reset();
        throw;
    }
}

/*virtual*/FileHeaderInfo CCziReaderWriter::GetFileHeaderInfo()
{
    this->ThrowIfNotOperational();
    FileHeaderInfo fhi;
    fhi.fileGuid = this->hdrSegmentData.GetFileGuid();
    this->hdrSegmentData.GetVersion(&fhi.majorVersion, &fhi.minorVersion);
    return fhi;
}

/*virtual*/void CCziReaderWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckAddSubBlockArguments(addSbBlkInfo);

    CCziSubBlockDirectoryBase::SubBlkEntry entry = CWriterUtils::SubBlkEntryFromAddSubBlockInfo(addSbBlkInfo);
    this->EnsureNextSegmentInfo();
    entry.FilePosition = this->nextSegmentInfo.GetNextSegmentPos();
    bool b = this->sbBlkDirectory.TryAddSubBlock(entry, nullptr);
    if (b == false)
    {
        throw LibCZIReaderWriteException("Could not add subblock because it already exists", LibCZIReaderWriteException::ErrorType::AddCoordinateAlreadyExisting);
    }

    CWriterUtils::WriteInfo wi;
    wi.segmentPos = this->nextSegmentInfo.GetNextSegmentPos();
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = false;

    auto sizeOfSbBlk = CWriterUtils::WriteSubBlock(wi, addSbBlkInfo);
    this->nextSegmentInfo.SetNextSegmentPos(wi.segmentPos + sizeOfSbBlk /*+ sizeof(SegmentHeader)*/);
}

/*virtual*/void CCziReaderWriter::SyncAddAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckAddAttachmentArguments(addAttachmentInfo);

    CCziAttachmentsDirectoryBase::AttachmentEntry entry = CWriterUtils::AttchmntEntryFromAddAttachmentInfo(addAttachmentInfo);
    this->EnsureNextSegmentInfo();
    entry.FilePosition = this->nextSegmentInfo.GetNextSegmentPos();
    bool b = this->attachmentDirectory.TryAddAttachment(entry, nullptr);
    if (b == false)
    {
        throw LibCZIReaderWriteException("Could not add attachment because it already exists", LibCZIReaderWriteException::ErrorType::AddAttachmentAlreadyExisting);
    }

    CWriterUtils::WriteInfo wi;
    wi.segmentPos = this->nextSegmentInfo.GetNextSegmentPos();
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = false;

    auto sizeOfAttchmnt = CWriterUtils::WriteAttachment(wi, addAttachmentInfo);
    this->nextSegmentInfo.SetNextSegmentPos(wi.segmentPos + sizeOfAttchmnt);
}

/*virtual*/void CCziReaderWriter::ReplaceSubBlock(int key, const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    this->ThrowIfNotOperational();
    CCziSubBlockDirectoryBase::SubBlkEntry sbEntryExisting;
    bool b = this->sbBlkDirectory.TryGetSubBlock(key, &sbEntryExisting);
    if (b == false)
    {
        throw LibCZIReaderWriteException("invalid id specified in \"ReplaceSubBlock\"", LibCZIReaderWriteException::ErrorType::InvalidSubBlkId);
    }

    auto allocatedSizeAndNewSbEntry = this->ReplaceSubBlock(addSbBlkInfo, sbEntryExisting);
    this->sbBlkDirectory.TryModifySubBlock(key, std::get<2>(allocatedSizeAndNewSbEntry));
    if (get<0>(allocatedSizeAndNewSbEntry))
    {
        // only advance the "nextSegmentInfo" in case we appended the subblock at the end
        this->nextSegmentInfo.SetNextSegmentPos(get<2>(allocatedSizeAndNewSbEntry).FilePosition + get<1>(allocatedSizeAndNewSbEntry) + sizeof(SegmentHeader));
    }
}

/*virtual*/void CCziReaderWriter::RemoveSubBlock(int key)
{
    this->ThrowIfNotOperational();
    CCziSubBlockDirectoryBase::SubBlkEntry sbEntryExisting;
    bool b = this->sbBlkDirectory.TryRemoveSubBlock(key, &sbEntryExisting);
    if (b == false)
    {
        throw LibCZIReaderWriteException("invalid id specified in \"RemoveSubBlock\"", LibCZIReaderWriteException::ErrorType::InvalidSubBlkId);
    }

    CWriterUtils::MarkDeletedInfo mdi;
    mdi.segmentPos = sbEntryExisting.FilePosition;
    mdi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    CWriterUtils::WriteDeletedSegment(mdi);
}

/*virtual*/void CCziReaderWriter::SyncWriteMetadata(const WriteMetadataInfo& metadataInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckWriteMetadataArguments(metadataInfo);

    this->EnsureNextSegmentInfo();
    CWriterUtils::MetadataWriteInfo mdWriterInfo;
    mdWriterInfo.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    if (!this->metadataSegment.IsValid())
    {
        mdWriterInfo.markAsDeletedIfExistingSegmentIsNotUsed = false;
        mdWriterInfo.existingSegmentPos = 0;
        mdWriterInfo.sizeExistingSegmentPos = 0;
    }
    else
    {
        mdWriterInfo.markAsDeletedIfExistingSegmentIsNotUsed = true;
        mdWriterInfo.existingSegmentPos = this->metadataSegment.GetFilePos();
        mdWriterInfo.sizeExistingSegmentPos = this->metadataSegment.GetAllocatedSize();
    }

    mdWriterInfo.segmentPosForNewSegment = this->nextSegmentInfo.GetNextSegmentPos();
    auto posAndAllocatedSize = CWriterUtils::WriteMetadata(mdWriterInfo, metadataInfo);
    if (get<0>(posAndAllocatedSize) == mdWriterInfo.segmentPosForNewSegment)
    {
        this->metadataSegment.SetPositionAndAllocatedSize(get<0>(posAndAllocatedSize), get<1>(posAndAllocatedSize), false);
        this->nextSegmentInfo.SetNextSegmentPos(get<0>(posAndAllocatedSize) + get<1>(posAndAllocatedSize) + sizeof(SegmentHeader));
    }
}

/*virtual*/std::shared_ptr<IMetadataSegment> CCziReaderWriter::ReadMetadataSegment()
{
    this->ThrowIfNotOperational();

    if (!this->metadataSegment.IsValid())
    {
        return shared_ptr<IMetadataSegment>();
    }

    CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };
    auto metaDataSegmentData = CCZIParse::ReadMetadataSegment(this->stream.get(), this->metadataSegment.GetFilePos(), allocateInfo);
    auto metaDataSegment = make_shared<CCziMetadataSegment>(metaDataSegmentData, allocateInfo.free);
    return metaDataSegment;
}

/*virtual*/void CCziReaderWriter::Close()
{
    this->ThrowIfNotOperational();
    this->Finish();
}

void CCziReaderWriter::Finish()
{
    if (this->sbBlkDirectory.IsModified())
    {
        this->EnsureNextSegmentInfo();
        CWriterUtils::SubBlkDirWriteInfo sbBlkDirWriteInfo;
        if (this->subBlockDirectorySegment.IsValid())
        {
            sbBlkDirWriteInfo.existingSegmentPos = this->subBlockDirectorySegment.GetFilePos();
            sbBlkDirWriteInfo.sizeExistingSegmentPos = this->subBlockDirectorySegment.GetAllocatedSize();
            sbBlkDirWriteInfo.markAsDeletedIfExistingSegmentIsNotUsed = true;
        }
        else
        {
            sbBlkDirWriteInfo.existingSegmentPos = 0;
            sbBlkDirWriteInfo.sizeExistingSegmentPos = 0;
            sbBlkDirWriteInfo.markAsDeletedIfExistingSegmentIsNotUsed = false;
        }

        sbBlkDirWriteInfo.segmentPosForNewSegment = this->nextSegmentInfo.GetNextSegmentPos();
        sbBlkDirWriteInfo.enumEntriesFunc = [&](const std::function<void(size_t, const CCziSubBlockDirectoryBase::SubBlkEntry&)>& f)->void
        {
            this->sbBlkDirectory.EnumEntries(
                [&](size_t index, const CCziSubBlockDirectoryBase::SubBlkEntry& e)->bool
                {
                    f(index, e);
            return true;
                });
        };
        sbBlkDirWriteInfo.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
        auto posAndSize = CWriterUtils::WriteSubBlkDirectory(sbBlkDirWriteInfo);
        this->subBlockDirectorySegment.SetPositionAndAllocatedSize(get<0>(posAndSize), get<1>(posAndSize), false);
        if (get<0>(posAndSize) == sbBlkDirWriteInfo.segmentPosForNewSegment)
        {
            // ie the subblock-directory was appended at the end
            this->nextSegmentInfo.SetNextSegmentPos(get<0>(posAndSize) + get<1>(posAndSize));
        }
    }

    if (this->attachmentDirectory.IsModified())
    {
        this->EnsureNextSegmentInfo();
        CWriterUtils::AttachmentDirWriteInfo attchmntDirWriteInfo;
        if (this->attachmentDirectorySegment.IsValid())
        {
            attchmntDirWriteInfo.markAsDeletedIfExistingSegmentIsNotUsed = true;
            attchmntDirWriteInfo.existingSegmentPos = this->attachmentDirectorySegment.GetFilePos();
            attchmntDirWriteInfo.sizeExistingSegmentPos = this->attachmentDirectorySegment.GetAllocatedSize();
        }
        else
        {
            attchmntDirWriteInfo.markAsDeletedIfExistingSegmentIsNotUsed = false;
            attchmntDirWriteInfo.existingSegmentPos = 0;
            attchmntDirWriteInfo.sizeExistingSegmentPos = 0;
        }

        attchmntDirWriteInfo.segmentPosForNewSegment = this->nextSegmentInfo.GetNextSegmentPos();
        attchmntDirWriteInfo.entryCnt = this->attachmentDirectory.GetEntryCnt();
        attchmntDirWriteInfo.enumEntriesFunc = [&](const std::function<void(size_t, const CCziAttachmentsDirectoryBase::AttachmentEntry&)>& f)->void
        {
            this->attachmentDirectory.EnumEntries(
                [&](size_t index, const CCziAttachmentsDirectoryBase::AttachmentEntry& e)->bool
                {
                    f(index, e);
            return true;
                });
        };
        attchmntDirWriteInfo.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
        auto posAndSize = CWriterUtils::WriteAttachmentDirectory(attchmntDirWriteInfo);
        this->attachmentDirectorySegment.SetPositionAndAllocatedSize(get<0>(posAndSize), get<1>(posAndSize), false);
        if (get<0>(posAndSize) == attchmntDirWriteInfo.segmentPosForNewSegment)
        {
            // ie the attachment-directory was appended at the end
            this->nextSegmentInfo.SetNextSegmentPos(get<0>(posAndSize) + get<1>(posAndSize));
        }
    }

    if ((this->attachmentDirectory.IsModified() && this->hdrSegmentData.GetAttachmentDirectoryPosition() != this->attachmentDirectorySegment.GetFilePos()) ||
        (this->sbBlkDirectory.IsModified() && this->hdrSegmentData.GetSubBlockDirectoryPosition() != this->subBlockDirectorySegment.GetFilePos()) ||
        this->hdrSegmentData.GetMetadataPosition() != this->metadataSegment.GetFilePos())
    {
        this->UpdateFileHeader();
    }
}

void CCziReaderWriter::UpdateFileHeader()
{
    FileHeaderSegment fhs = { 0 };

    fhs.header.AllocatedSize = fhs.header.UsedSize = sizeof(fhs.data);
    memcpy(&fhs.header.Id, &CCZIParse::FILEHDRMAGIC, 16);

    fhs.data.Major = 1;
    fhs.data.Minor = 0;
    fhs.data.PrimaryFileGuid = this->hdrSegmentData.GetFileGuid();
    fhs.data.FileGuid = this->hdrSegmentData.GetFileGuid();
    fhs.data.SubBlockDirectoryPosition = this->subBlockDirectorySegment.GetFilePos();
    fhs.data.MetadataPosition = this->metadataSegment.GetFilePos();
    fhs.data.AttachmentDirectoryPosition = this->attachmentDirectorySegment.GetFilePos();

    ConvertToHostByteOrder::Convert(&fhs);
    this->WriteToOutputStream(0, &fhs, sizeof(fhs), nullptr, "FileHeader");
}

void CCziReaderWriter::ReadCziStructure()
{
    FileHeaderSegmentData fileHeaderSegment;
    bool cziHeaderSuccessfullyRead;
    // we try to read the CZI-file-header (from the existing file)
    try
    {
        fileHeaderSegment = CCZIParse::ReadFileHeaderSegment(this->stream.get());
        cziHeaderSuccessfullyRead = true;
    }
    catch (const LibCZICZIParseException& excp)
    {
        if (excp.GetErrorCode() == LibCZICZIParseException::ErrorCode::NotEnoughData)
        {
            // this now means that the existing file did not contain a CZI-file-header, which let's us
            // treat this file as "new" (so, there is no information we re-use)     
            cziHeaderSuccessfullyRead = false;
        }
        else
        {
            throw;
        }
    }

    if (cziHeaderSuccessfullyRead)
    {
        if (this->info->GetForceFileGuid())
        {
            // we then immediately update the File-Guid
            auto newGuid = this->UpdateFileHeaderGuid();
            memcpy(&fileHeaderSegment.FileGuid, &newGuid, sizeof(newGuid));
            memcpy(&fileHeaderSegment.PrimaryFileGuid, &newGuid, sizeof(newGuid));
        }

        this->hdrSegmentData = CFileHeaderSegmentData(&fileHeaderSegment);

        CCZIParse::SegmentSizes sbBlkDirSegmentSize;
        CCZIParse::ReadSubBlockDirectory(
            this->stream.get(),
            this->hdrSegmentData.GetSubBlockDirectoryPosition(),
            [&](const CCziSubBlockDirectoryBase::SubBlkEntry& e)->void
            {
                this->sbBlkDirectory.AddSubBlock(e);
            },
            CCZIParse::SubblockDirectoryParseOptions{},
            &sbBlkDirSegmentSize);

        this->sbBlkDirectory.SetModified(false);

        this->subBlockDirectorySegment.SetPositionAndAllocatedSize(this->hdrSegmentData.GetSubBlockDirectoryPosition(), sbBlkDirSegmentSize.AllocatedSize, false);

        auto pos = this->hdrSegmentData.GetAttachmentDirectoryPosition();
        if (pos != 0)
        {
            CCZIParse::SegmentSizes attchmntDirSegmentSize;
            CCZIParse::ReadAttachmentsDirectory(
                this->stream.get(),
                pos,
                [&](const CCziAttachmentsDirectoryBase::AttachmentEntry& ae)->void
                {
                    this->attachmentDirectory.AddAttachment(ae);
                },
                &attchmntDirSegmentSize);

            this->attachmentDirectorySegment.SetPositionAndAllocatedSize(pos, attchmntDirSegmentSize.AllocatedSize, false);
        }

        pos = this->hdrSegmentData.GetMetadataPosition();
        if (pos != 0)
        {
            auto segmentSize = CCZIParse::ReadSegmentHeader(CCZIParse::SegmentType::Metadata, this->stream.get(), this->hdrSegmentData.GetMetadataPosition());
            this->metadataSegment.SetPositionAndAllocatedSize(pos, segmentSize.AllocatedSize, false);
        }
    }
    else
    {
        // if there is no valid CZI-file-header, we now write one
        FileHeaderSegment fhs = { 0 };
        fhs.header.AllocatedSize = fhs.header.UsedSize = sizeof(fhs.data);
        memcpy(&fhs.header.Id, &CCZIParse::FILEHDRMAGIC, 16);

        fhs.data.Major = 1;
        fhs.data.Minor = 0;

        libCZI::GUID fileGuid = this->info->GetFileGuid();
        if (Utilities::IsGuidNull(fileGuid))
        {
            fileGuid = Utilities::GenerateNewGuid();
        }

        fhs.data.PrimaryFileGuid = fileGuid;
        fhs.data.FileGuid = fileGuid;
        fhs.data.SubBlockDirectoryPosition = 0;
        fhs.data.MetadataPosition = 0;
        fhs.data.AttachmentDirectoryPosition = 0;

        ConvertToHostByteOrder::Convert(&fhs);
        this->WriteToOutputStream(0, &fhs, sizeof(fhs), nullptr, "FileHeader");

        this->hdrSegmentData = CFileHeaderSegmentData(&fhs.data);
    }

    this->DetermineNextSubBlockOffset();
}

libCZI::GUID CCziReaderWriter::UpdateFileHeaderGuid()
{
    libCZI::GUID fileGuid = this->info->GetFileGuid();
    if (Utilities::IsGuidNull(fileGuid))
    {
        fileGuid = Utilities::GenerateNewGuid();
    }

    Utilities::ConvertGuidToHostByteOrder(&fileGuid);
    this->WriteToOutputStream(offsetof(FileHeaderSegment, data.PrimaryFileGuid), &fileGuid, sizeof(fileGuid), nullptr, "UpdateFileHeaderGuid");
    this->WriteToOutputStream(offsetof(FileHeaderSegment, data.FileGuid), &fileGuid, sizeof(fileGuid), nullptr, "UpdateFileHeaderGuid");
    return fileGuid;
}

void CCziReaderWriter::DetermineNextSubBlockOffset()
{
    std::uint64_t lastSegmentPos = 0;
    this->sbBlkDirectory.EnumEntries(
        [&](size_t index, const CCziSubBlockDirectoryBase::SubBlkEntry& sbBlkEntry)->bool
        {
            if (sbBlkEntry.FilePosition > lastSegmentPos)
            {
                lastSegmentPos = sbBlkEntry.FilePosition;
            }

    return true;
        });

    this->attachmentDirectory.EnumEntries(
        [&](size_t index, const CCziAttachmentsDirectoryBase::AttachmentEntry& attEntry)->bool
        {
            if (uint64_t(attEntry.FilePosition) > lastSegmentPos)
            {
                lastSegmentPos = attEntry.FilePosition;
            }

    return true;
        });

    if (this->hdrSegmentData.GetIsSubBlockDirectoryPositionValid())
    {
        lastSegmentPos = (std::max)(lastSegmentPos, this->hdrSegmentData.GetSubBlockDirectoryPosition());
    }

    if (this->hdrSegmentData.GetIsAttachmentDirectoryPositionValid())
    {
        lastSegmentPos = (std::max)(lastSegmentPos, this->hdrSegmentData.GetAttachmentDirectoryPosition());
    }

    if (this->hdrSegmentData.GetIsMetadataPositionPositionValid())
    {
        lastSegmentPos = (std::max)(lastSegmentPos, this->hdrSegmentData.GetMetadataPosition());
    }

    this->nextSegmentInfo.SetLastSegmentPos(lastSegmentPos);
}

std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> CCziReaderWriter::ReplaceSubBlock(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry)
{
    std::uint64_t usedSizeAddedSbBlk;
    CWriterUtils::CalculateSegmentDataSize(addSubBlockInfo, nullptr, &usedSizeAddedSbBlk);
    // usedSizeAddedSbBlk <- the size we require for the newly to be added subblock (including the SegmentHeader)

    const auto existingSbBlkSize = this->ReadSegmentHdrOfSubBlock(subBlkEntry.FilePosition);

    if (existingSbBlkSize.AllocatedSize + sizeof(SegmentHeader) < usedSizeAddedSbBlk)
    {
        return this->ReplaceSubBlockAddNewAtEnd(addSubBlockInfo, subBlkEntry);
    }
    else
    {
        return this->ReplaceSubBlockInplace(addSubBlockInfo, subBlkEntry, existingSbBlkSize.AllocatedSize);
    }
}

std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> CCziReaderWriter::ReplaceSubBlockInplace(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry, std::uint64_t existingSegmentAllocatedSize)
{
    CWriterUtils::WriteInfo wi;
    wi.segmentPos = subBlkEntry.FilePosition;
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = true;
    wi.specifiedAllocatedSize = existingSegmentAllocatedSize;

    auto sizeOfSbBlk = CWriterUtils::WriteSubBlock(wi, addSubBlockInfo);

    CCziSubBlockDirectoryBase::SubBlkEntry entry = CWriterUtils::SubBlkEntryFromAddSubBlockInfo(addSubBlockInfo);
    entry.FilePosition = wi.segmentPos;

    return make_tuple(false, sizeOfSbBlk, entry);
}

std::tuple<bool, std::uint64_t, CCziSubBlockDirectoryBase::SubBlkEntry> CCziReaderWriter::ReplaceSubBlockAddNewAtEnd(const libCZI::AddSubBlockInfo& addSubBlockInfo, const CCziSubBlockDirectoryBase::SubBlkEntry& subBlkEntry)
{
    this->EnsureNextSegmentInfo();
    CWriterUtils::WriteInfo wi;
    wi.segmentPos = this->nextSegmentInfo.GetNextSegmentPos();
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = false;

    auto sizeOfSbBlk = CWriterUtils::WriteSubBlock(wi, addSubBlockInfo);

    CWriterUtils::MarkDeletedInfo mdi;
    mdi.segmentPos = subBlkEntry.FilePosition;
    mdi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    CWriterUtils::WriteDeletedSegment(mdi);

    CCziSubBlockDirectoryBase::SubBlkEntry entry = CWriterUtils::SubBlkEntryFromAddSubBlockInfo(addSubBlockInfo);
    entry.FilePosition = wi.segmentPos;

    return make_tuple(true, sizeOfSbBlk, entry);
}

void CCziReaderWriter::EnsureNextSegmentInfo()
{
    // This method ensures that the "position for the next segment" is valid. We delay the determination of this position until
    // it is actually required (ie. when we have to add a new segment). This is a kind of micro-optimization because this determination
    // involves to actually read the last segment (and this might not be necessary if we do not add segments at all).
    if (!this->nextSegmentInfo.IsNextSegmentPosValid())
    {
        // TODO: consider the special case where we want to extend the last segment

        const auto lastSegmentSize = CCZIParse::ReadSegmentHeaderAny(this->stream.get(), this->nextSegmentInfo.GetLastSegmentPos());
        const std::uint64_t nextSegmentPos = lastSegmentSize.AllocatedSize + sizeof(SegmentHeader) + this->nextSegmentInfo.GetLastSegmentPos();

        this->nextSegmentInfo.SetNextSegmentPos(nextSegmentPos);
    }
}

CCZIParse::SegmentSizes CCziReaderWriter::ReadSegmentHdrOfSubBlock(std::uint64_t pos)
{
    return CCZIParse::ReadSegmentHeader(CCZIParse::SegmentType::SbBlk, this->stream.get(), pos);
}

CCZIParse::SegmentSizes CCziReaderWriter::ReadSegmentHdrOfAttachment(std::uint64_t pos)
{
    return CCZIParse::ReadSegmentHeader(CCZIParse::SegmentType::Attachment, this->stream.get(), pos);
}

void CCziReaderWriter::WriteToOutputStream(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)
{
    uint64_t bytesWritten;
    try
    {
        this->stream->Write(offset, pv, size, &bytesWritten);
    }
    catch (const std::exception&)
    {
        stringstream ss;
        if (nameOfPartToWrite == nullptr)
        {
            ss << "Error writing output-stream";
        }
        else
        {
            ss << "Error writing '" << nameOfPartToWrite << "'";
        }

        std::throw_with_nested(LibCZIIOException(ss.str().c_str(), offset, size));
    }

    if (bytesWritten != size)
    {
        CCziReaderWriter::ThrowNotEnoughDataWritten(offset, size, bytesWritten);
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesWritten;
    }
}

/*static*/void CCziReaderWriter::ThrowNotEnoughDataWritten(std::uint64_t offset, std::uint64_t bytesToWrite, std::uint64_t bytesActuallyWritten)
{
    stringstream ss;
    ss << "Not enough data written at offset " << offset << " -> bytes to write: " << bytesToWrite << " bytes, actually written " << bytesActuallyWritten << " bytes.";
    throw LibCZIWriteException(ss.str().c_str(), LibCZIWriteException::ErrorType::NotEnoughDataWritten);
}

/*virtual*/void CCziReaderWriter::EnumerateSubBlocks(const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum)
{
    this->ThrowIfNotOperational();
    this->sbBlkDirectory.EnumEntries(
        [&](int index, const CCziSubBlockDirectory::SubBlkEntry& entry)->bool
        {
            SubBlockInfo info;
    info.coordinate = entry.coordinate;
    info.logicalRect = IntRect{ entry.x,entry.y,entry.width,entry.height };
    info.physicalSize = IntSize{ (std::uint32_t)entry.storedWidth, (std::uint32_t)entry.storedHeight };
    info.mIndex = entry.mIndex;
    info.pixelType = CziUtils::PixelTypeFromInt(entry.PixelType);
    return funcEnum(index, info);
        });
}

/*virtual*/void CCziReaderWriter::EnumSubset(const libCZI::IDimCoordinate* planeCoordinate, const libCZI::IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum)
{
    this->ThrowIfNotOperational();
    throw std::runtime_error("Not Implemented");
}

/*virtual*/std::shared_ptr<libCZI::ISubBlock> CCziReaderWriter::ReadSubBlock(int index)
{
    this->ThrowIfNotOperational();
    CCziSubBlockDirectoryBase::SubBlkEntry entry;
    if (this->sbBlkDirectory.TryGetSubBlock(index, &entry) == false)
    {
        return std::shared_ptr<ISubBlock>();
    }

    CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

    auto subBlkData = CCZIParse::ReadSubBlock(this->stream.get(), entry.FilePosition, allocateInfo);

    libCZI::SubBlockInfo info;
    info.pixelType = CziUtils::PixelTypeFromInt(subBlkData.pixelType);
    info.compressionModeRaw = subBlkData.compression;
    info.coordinate = subBlkData.coordinate;
    info.mIndex = subBlkData.mIndex;
    info.logicalRect = subBlkData.logicalRect;
    info.physicalSize = subBlkData.physicalSize;
    info.pyramidType = CziUtils::PyramidTypeFromByte(subBlkData.spare[0]);

    return std::make_shared<CCziSubBlock>(info, subBlkData, free);
}

/*virtual*/bool CCziReaderWriter::TryGetSubBlockInfo(int index, libCZI::SubBlockInfo* info) const
{
    this->ThrowIfNotOperational();
    CCziSubBlockDirectoryBase::SubBlkEntry entry;
    if (this->sbBlkDirectory.TryGetSubBlock(index, &entry) == false)
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
        info->pyramidType = CziUtils::PyramidTypeFromByte(entry.pyramid_type_from_spare);
    }

    return true;
}

/*virtual*/bool CCziReaderWriter::TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, libCZI::SubBlockInfo& info)
{
    this->ThrowIfNotOperational();
    throw std::runtime_error("Not Implemented");
}

/*virtual*/libCZI::SubBlockStatistics CCziReaderWriter::GetStatistics()
{
    this->ThrowIfNotOperational();
    return this->sbBlkDirectory.GetStatistics();
}

/*virtual*/libCZI::PyramidStatistics CCziReaderWriter::GetPyramidStatistics()
{
    this->ThrowIfNotOperational();
    return this->sbBlkDirectory.GetPyramidStatistics();
}

/*virtual*/void CCziReaderWriter::EnumerateAttachments(const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum)
{
    this->ThrowIfNotOperational();
    this->attachmentDirectory.EnumEntries(
        [&](int index, const CCziAttachmentsDirectoryBase::AttachmentEntry& entry)->bool
        {
            libCZI::AttachmentInfo info;
    info.contentGuid = entry.ContentGuid;
    memcpy(info.contentFileType, entry.ContentFileType, sizeof(entry.ContentFileType));
    info.name = entry.Name;
    bool b = funcEnum(index, info);
    return b;
        });
}

/*virtual*/void CCziReaderWriter::EnumerateSubset(const char* contentFileType, const char* name, const std::function<bool(int index, const libCZI::AttachmentInfo& infi)>& funcEnum)
{
    this->ThrowIfNotOperational();
    throw std::runtime_error("Not Implemented");
}

/*virtual*/std::shared_ptr<libCZI::IAttachment> CCziReaderWriter::ReadAttachment(int index)
{
    this->ThrowIfNotOperational();
    CCziAttachmentsDirectoryBase::AttachmentEntry entry;
    if (this->attachmentDirectory.TryGetAttachment(index, &entry) == false)
    {
        return std::shared_ptr<IAttachment>();
    }

    return this->ReadAttachment(entry);
}

/*virtual*/void CCziReaderWriter::ReplaceAttachment(int attchmntId, const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    this->ThrowIfNotOperational();
    CCziAttachmentsDirectoryBase::AttachmentEntry attchmntEntryExisting;
    bool b = this->attachmentDirectory.TryGetAttachment(attchmntId, &attchmntEntryExisting);
    if (b == false)
    {
        throw LibCZIReaderWriteException("invalid id specified in \"ReplaceAttachment\"", LibCZIReaderWriteException::ErrorType::InvalidAttachmentId);
    }

    auto allocatedSizeAndNewAttchmntEntry = this->ReplaceAttachment(addAttachmentInfo, attchmntEntryExisting);
    this->attachmentDirectory.TryModifyAttachment(attchmntId, std::get<2>(allocatedSizeAndNewAttchmntEntry));
    if (get<0>(allocatedSizeAndNewAttchmntEntry))
    {
        // only advance the "nextSegmentInfo" in case we appended the subblock at the end
        this->nextSegmentInfo.SetNextSegmentPos(get<2>(allocatedSizeAndNewAttchmntEntry).FilePosition + get<1>(allocatedSizeAndNewAttchmntEntry) + sizeof(SegmentHeader));
    }
}

std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> CCziReaderWriter::ReplaceAttachment(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo)
{
    std::uint64_t usedSizeAddedAttchmnt;
    CWriterUtils::CalculateSegmentDataSize(addAttchmntInfo, nullptr, &usedSizeAddedAttchmnt);
    // usedSizeAddedSbBlk <- the size we require for the newly to be added subblock

    auto existingSbBlkSize = this->ReadSegmentHdrOfAttachment(attchmntInfo.FilePosition);

    if (uint64_t(existingSbBlkSize.AllocatedSize) < usedSizeAddedAttchmnt)
    {
        return this->ReplaceAttachmentAddNewAtEnd(addAttchmntInfo, attchmntInfo);
    }
    else
    {
        return this->ReplaceAttachmentInplace(addAttchmntInfo, attchmntInfo, existingSbBlkSize.AllocatedSize);
    }
}

std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> CCziReaderWriter::ReplaceAttachmentAddNewAtEnd(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo)
{
    this->EnsureNextSegmentInfo();
    CWriterUtils::WriteInfo wi;
    wi.segmentPos = this->nextSegmentInfo.GetNextSegmentPos();
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = false;

    auto sizeOfSbBlk = CWriterUtils::WriteAttachment(wi, addAttchmntInfo);

    CWriterUtils::MarkDeletedInfo mdi;
    mdi.segmentPos = attchmntInfo.FilePosition;
    mdi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    CWriterUtils::WriteDeletedSegment(mdi);

    CCziAttachmentsDirectoryBase::AttachmentEntry entry;
    entry.FilePosition = wi.segmentPos;
    entry.ContentGuid = addAttchmntInfo.contentGuid;
    memcpy(entry.ContentFileType, addAttchmntInfo.contentFileType, sizeof(entry.ContentFileType));
    memcpy(entry.Name, addAttchmntInfo.name, sizeof(entry.Name));

    return make_tuple(false, sizeOfSbBlk, entry);
}

std::tuple<bool, std::uint64_t, CCziAttachmentsDirectoryBase::AttachmentEntry> CCziReaderWriter::ReplaceAttachmentInplace(const libCZI::AddAttachmentInfo& addAttchmntInfo, const CCziAttachmentsDirectoryBase::AttachmentEntry& attchmntInfo, std::uint64_t existingSegmentAllocatedSize)
{
    CWriterUtils::WriteInfo wi;
    wi.segmentPos = attchmntInfo.FilePosition;
    wi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    wi.useSpecifiedAllocatedSize = true;
    wi.specifiedAllocatedSize = existingSegmentAllocatedSize;

    auto sizeOfSbBlk = CWriterUtils::WriteAttachment(wi, addAttchmntInfo);
    CCziAttachmentsDirectoryBase::AttachmentEntry entry;
    entry.FilePosition = wi.segmentPos;
    entry.ContentGuid = addAttchmntInfo.contentGuid;
    memcpy(entry.ContentFileType, addAttchmntInfo.contentFileType, sizeof(entry.ContentFileType));
    memcpy(entry.Name, addAttchmntInfo.name, sizeof(entry.Name));

    return make_tuple(false, sizeOfSbBlk, entry);
}

/*virtual*/void CCziReaderWriter::RemoveAttachment(int attchmntId)
{
    this->ThrowIfNotOperational();
    CCziAttachmentsDirectoryBase::AttachmentEntry attchmntEntryExisting;
    bool b = this->attachmentDirectory.TryRemoveAttachment(attchmntId, &attchmntEntryExisting);
    if (b == false)
    {
        throw LibCZIReaderWriteException("invalid id specified in \"RemoveAttachment\"", LibCZIReaderWriteException::ErrorType::InvalidAttachmentId);
    }

    CWriterUtils::MarkDeletedInfo mdi;
    mdi.segmentPos = attchmntEntryExisting.FilePosition;
    mdi.writeFunc = std::bind(&CCziReaderWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    CWriterUtils::WriteDeletedSegment(mdi);
}

std::shared_ptr<libCZI::IAttachment> CCziReaderWriter::ReadAttachment(const CCziAttachmentsDirectoryBase::AttachmentEntry& entry)
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

void CCziReaderWriter::ThrowIfNotOperational() const
{
    if (!this->stream)
    {
        throw logic_error("CCziReaderWriter is not operational (must call 'Create' first).");
    }
}

void CCziReaderWriter::ThrowIfAlreadyInitialized() const
{
    if (this->stream)
    {
        throw logic_error("CCziReaderWriter is already operational.");
    }
}
