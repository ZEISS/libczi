// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CziWriter.h"
#include "CziMetadataSegment.h"
#include "libCZI_Utilities.h"
#include "CziUtils.h"
#include "libCZI_exceptions.h"
#include "CziMetadataBuilder.h"
#include "utilities.h"

using namespace libCZI;
using namespace std;

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

void libCZI::ICziWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfo)
{
    AddSubBlockInfo addSbInfo(addSbBlkInfo);
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

    return this->SyncAddSubBlock(addSbInfo);
}

void libCZI::ICziWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise)
{
    AddSubBlockInfo addSbInfo(addSbInfoLinewise);

    size_t stride = addSbInfoLinewise.physicalWidth * (size_t)CziUtils::GetBytesPerPel(addSbInfoLinewise.PixelType);
    addSbInfo.sizeData = addSbInfoLinewise.physicalHeight * stride;
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

    return this->SyncAddSubBlock(addSbInfo);
}

void libCZI::ICziWriter::SyncAddSubBlock(const AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap)
{
    AddSubBlockInfo addSbInfo(addSbBlkInfoStrideBitmap);

    addSbInfo.sizeData = addSbBlkInfoStrideBitmap.physicalHeight * (addSbBlkInfoStrideBitmap.physicalWidth * (size_t)CziUtils::GetBytesPerPel(addSbBlkInfoStrideBitmap.PixelType));
    addSbInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
        {
            if (callCnt < addSbBlkInfoStrideBitmap.physicalHeight)
            {
                ptr = static_cast<const char*>(addSbBlkInfoStrideBitmap.ptrBitmap) + callCnt * (size_t)addSbBlkInfoStrideBitmap.strideBitmap;
                size = addSbBlkInfoStrideBitmap.physicalWidth * (size_t)CziUtils::GetBytesPerPel(addSbBlkInfoStrideBitmap.PixelType);
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

    return this->SyncAddSubBlock(addSbInfo);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

/*static*/std::uint64_t CWriterUtils::WriteSubBlock(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    auto sbBlkSegmentSize = CWriterUtils::CalcSubBlockSegmentDataSize(addSbBlkInfo);

    std::uint64_t allocatedSize;
    size_t bytesWritten = 0;
    if (sbBlkSegmentSize <= sizeof(SubBlockSegment))
    {
        // the size of "SubBlockSegment" is big enough to hold the data-structure (this should always be the case)
        SubBlockSegment sbBlkSegment = { 0 };
        CWriterUtils::FillSubBlockSegment(info, addSbBlkInfo, &sbBlkSegment);
        if (info.useSpecifiedAllocatedSize == true)
        {
            if (uint64_t(sbBlkSegment.header.AllocatedSize) > info.specifiedAllocatedSize)
            {
                // TODO
                throw runtime_error("specified segment-size not sufficient");
            }

            sbBlkSegment.header.AllocatedSize = info.specifiedAllocatedSize;
        }

        allocatedSize = sbBlkSegment.header.AllocatedSize + sizeof(sbBlkSegment.header);

        ConvertToHostByteOrder::ConvertAndAllSubBlkEntries(&sbBlkSegment);
        bytesWritten += WriteSubBlockSegment(info, &sbBlkSegment, sbBlkSegmentSize, info.segmentPos + bytesWritten);
    }
    else
    {
        std::unique_ptr<SubBlockSegment, void(*)(SubBlockSegment*)> upSbBlkSegment((SubBlockSegment*)malloc(sbBlkSegmentSize), [](SubBlockSegment* p)->void {free(p); });
        memset(upSbBlkSegment.get(), 0, sbBlkSegmentSize);
        CWriterUtils::FillSubBlockSegment(info, addSbBlkInfo, upSbBlkSegment.get());
        if (info.useSpecifiedAllocatedSize == true)
        {
            if (uint64_t(upSbBlkSegment->header.AllocatedSize) > info.specifiedAllocatedSize)
            {
                // TODO
                throw runtime_error("specified segment-size not sufficient");
            }

            upSbBlkSegment->header.AllocatedSize = info.specifiedAllocatedSize;
        }

        allocatedSize = upSbBlkSegment->header.AllocatedSize + sizeof(upSbBlkSegment->header);

        ConvertToHostByteOrder::ConvertAndAllSubBlkEntries(upSbBlkSegment.get());
        bytesWritten += WriteSubBlockSegment(info, upSbBlkSegment.get(), sbBlkSegmentSize, info.segmentPos + bytesWritten);
    }

    bytesWritten += CWriterUtils::WriteSubBlkMetaData(info, addSbBlkInfo, info.segmentPos + bytesWritten);
    bytesWritten += CWriterUtils::WriteSubBlkData(info, addSbBlkInfo, info.segmentPos + bytesWritten);
    bytesWritten += CWriterUtils::WriteSubBlkAttachment(info, addSbBlkInfo, info.segmentPos + bytesWritten);

    if (bytesWritten < allocatedSize)
    {
        bytesWritten += (size_t)WriteZeroes(info, info.segmentPos + bytesWritten, size_t(allocatedSize - bytesWritten));
    }

    return bytesWritten;
}

/*static*/std::uint64_t CWriterUtils::WriteAttachment(const WriteInfo& info, const libCZI::AddAttachmentInfo& addAttchmntInfo)
{
    AttachmentSegment attchmntSegment = { 0 };
    memcpy(attchmntSegment.header.Id, CCZIParse::ATTACHMENTBLKMAGIC, 16);
    attchmntSegment.header.UsedSize = sizeof(AttachmentSegmentData) + addAttchmntInfo.dataSize;
    if (!info.useSpecifiedAllocatedSize)
    {
        attchmntSegment.header.AllocatedSize = CWriterUtils::AlignSegmentSize(attchmntSegment.header.UsedSize);
    }
    else
    {
        attchmntSegment.header.AllocatedSize = info.specifiedAllocatedSize;
    }

    attchmntSegment.data.DataSize = addAttchmntInfo.dataSize;
    attchmntSegment.data.entry.SchemaType[0] = 'A';
    attchmntSegment.data.entry.SchemaType[1] = '1';
    attchmntSegment.data.entry.FilePosition = info.segmentPos;  // this is redundant, does not really make sense
    attchmntSegment.data.entry.ContentGuid = addAttchmntInfo.contentGuid;
    memcpy(&attchmntSegment.data.entry.ContentFileType[0], &addAttchmntInfo.contentFileType[0], sizeof(attchmntSegment.data.entry.ContentFileType));
    memcpy(&attchmntSegment.data.entry.Name[0], &addAttchmntInfo.name[0], sizeof(attchmntSegment.data.entry.Name));

    uint64_t  bytesWritten;
    uint64_t  totalBytesWritten = 0;
    auto attchmntSegmentHeaderAllocatedSize = attchmntSegment.header.AllocatedSize; // save this before we (potentially) modify the byte-order

    ConvertToHostByteOrder::Convert(&attchmntSegment);
    info.writeFunc(info.segmentPos, &attchmntSegment, sizeof(attchmntSegment), &bytesWritten, "AttachmentSegment");
    totalBytesWritten += bytesWritten;

    info.writeFunc(info.segmentPos + totalBytesWritten, addAttchmntInfo.ptrData, addAttchmntInfo.dataSize, &bytesWritten, "AttachmentData");
    totalBytesWritten += bytesWritten;

    if (uint64_t(attchmntSegmentHeaderAllocatedSize + sizeof(SegmentHeader)) > totalBytesWritten)
    {
        totalBytesWritten += WriteZeroes(info, info.segmentPos + totalBytesWritten, attchmntSegmentHeaderAllocatedSize + sizeof(SegmentHeader) - totalBytesWritten);
    }

    return totalBytesWritten;
}

/*static*/void CWriterUtils::FillSubBlockSegment(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegment* sbBlkSegment)
{
    memcpy(sbBlkSegment->header.Id, CCZIParse::SUBBLKMAGIC, 16);
    CWriterUtils::SetAllocatedAndUsedSize(addSbBlkInfo, sbBlkSegment);
    CWriterUtils::FillSubBlockSegmentData(addSbBlkInfo, &(sbBlkSegment->data));
    sbBlkSegment->data.entryDV.FilePosition = info.segmentPos; // this is redundant, does not really make sense
}

/*static*/bool CWriterUtils::SetAllocatedAndUsedSize(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegment* pSbBlkSegment)
{
    std::uint64_t allocatedSize, usedSize;
    bool b = CWriterUtils::CalculateSegmentDataSize(addSbBlkInfo, &allocatedSize, &usedSize);
    if (b == true)
    {
        pSbBlkSegment->header.AllocatedSize = allocatedSize - sizeof(SegmentHeader);
        pSbBlkSegment->header.UsedSize = usedSize - sizeof(SegmentHeader);;
    }

    return b;
}

/*static*/void CWriterUtils::FillSubBlockSegmentData(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockSegmentData* ptr)
{
    if (addSbBlkInfo.sizeMetadata > (size_t)(numeric_limits<int>::max)())
    {
        throw invalid_argument("sizeMetadata must be <= numeric_limits<int>::max()");
    }

    if (addSbBlkInfo.sizeAttachment > (size_t)(numeric_limits<int>::max)())
    {
        throw invalid_argument("sizeAttachment must be <= numeric_limits<int>::max()");
    }

    ptr->MetadataSize = int(addSbBlkInfo.sizeMetadata);
    ptr->AttachmentSize = int(addSbBlkInfo.sizeAttachment);
    ptr->DataSize = addSbBlkInfo.sizeData;

    // now, write the SubBlockDirectoryEntryDV
    CWriterUtils::FillSubBlockDirectoryEntryDv(addSbBlkInfo, &ptr->entryDV);
}

/*static*/void CWriterUtils::FillSubBlockDirectoryEntryDv(const libCZI::AddSubBlockInfo& addSbBlkInfo, SubBlockDirectoryEntryDV* ptr)
{
    ptr->SchemaType[0] = 'D';
    ptr->SchemaType[1] = 'V';
    ptr->PixelType = CziUtils::IntFromPixelType(addSbBlkInfo.PixelType);
    ptr->FilePosition = ptr->FilePart = 0;
    ptr->Compression = addSbBlkInfo.compressionModeRaw;
    ptr->_spare[0] = CziUtils::ByteFromPyramidType(addSbBlkInfo.pyramid_type);
    memset(ptr->_spare + 1, 0, sizeof(ptr->_spare) - 1);
    ptr->DimensionCount = CalcCountOfDimensionsEntriesInDirectoryEntryDV(addSbBlkInfo);

    // first X and Y
    int curDim = 0;
    SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'X');
    ptr->DimensionEntries[curDim].Start = addSbBlkInfo.x;
    ptr->DimensionEntries[curDim].Size = addSbBlkInfo.logicalWidth;
    ptr->DimensionEntries[curDim].StoredSize = addSbBlkInfo.physicalWidth;
    ptr->DimensionEntries[curDim].StartCoordinate = 0;

    ++curDim;

    SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'Y');
    ptr->DimensionEntries[curDim].Start = addSbBlkInfo.y;
    ptr->DimensionEntries[curDim].Size = addSbBlkInfo.logicalHeight;
    ptr->DimensionEntries[curDim].StoredSize = addSbBlkInfo.physicalHeight;
    ptr->DimensionEntries[curDim].StartCoordinate = 0;

    if (addSbBlkInfo.mIndexValid)
    {
        ++curDim;
        SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'M');
        ptr->DimensionEntries[curDim].Start = addSbBlkInfo.mIndex;
        ptr->DimensionEntries[curDim].Size = 1;
        ptr->DimensionEntries[curDim].StartCoordinate = 0;
        ptr->DimensionEntries[curDim].StoredSize = 1;
    }

    addSbBlkInfo.coordinate.EnumValidDimensions(
        [&](libCZI::DimensionIndex dim, int value)->bool
        {
            curDim++;
            SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), libCZI::Utils::DimensionToChar(dim));
            ptr->DimensionEntries[curDim].Start = value;
            ptr->DimensionEntries[curDim].Size = 1;
            ptr->DimensionEntries[curDim].StartCoordinate = 0;
            ptr->DimensionEntries[curDim].StoredSize = 1;
            return true;
        });
}

/*static*/int CWriterUtils::CalcCountOfDimensionsEntriesInDirectoryEntryDV(const CCziSubBlockDirectoryBase::SubBlkEntry& entry)
{
    int numberOfDimensionEntries = 2;   // we always have X AND Y
    numberOfDimensionEntries += entry.coordinate.GetValidDimensionsCount();
    if (entry.IsMIndexValid())
    {
        ++numberOfDimensionEntries;
    }

    return numberOfDimensionEntries;
}

/*static*/size_t CWriterUtils::FillSubBlockDirectoryEntryDV(SubBlockDirectoryEntryDV* ptr, const CCziSubBlockDirectoryBase::SubBlkEntry& entry)
{
    ptr->SchemaType[0] = 'D';
    ptr->SchemaType[1] = 'V';
    ptr->PixelType = entry.PixelType;
    ptr->FilePosition = entry.FilePosition;
    ptr->FilePart = 0;
    ptr->Compression = entry.Compression;
    ptr->_spare[0] = entry.pyramid_type_from_spare;
    memset(ptr->_spare + 1, 0, sizeof(ptr->_spare) - 1);    // skipping the pyramid-spare-byte we set above here
    ptr->DimensionCount = CalcCountOfDimensionsEntriesInDirectoryEntryDV(entry);

    // first X and Y
    int curDim = 0;
    SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'X');
    ptr->DimensionEntries[curDim].Start = entry.x;
    ptr->DimensionEntries[curDim].Size = entry.width;
    ptr->DimensionEntries[curDim].StoredSize = entry.storedWidth;
    ptr->DimensionEntries[curDim].StartCoordinate = 0;

    ++curDim;

    SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'Y');
    ptr->DimensionEntries[curDim].Start = entry.y;
    ptr->DimensionEntries[curDim].Size = entry.height;
    ptr->DimensionEntries[curDim].StoredSize = entry.storedHeight;
    ptr->DimensionEntries[curDim].StartCoordinate = 0;

    if (entry.IsMIndexValid())
    {
        ++curDim;
        SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), 'M');
        ptr->DimensionEntries[curDim].Start = entry.mIndex;
        ptr->DimensionEntries[curDim].Size = 1;
        ptr->DimensionEntries[curDim].StartCoordinate = 0;
        ptr->DimensionEntries[curDim].StoredSize = 1;
    }

    entry.coordinate.EnumValidDimensions(
        [&](libCZI::DimensionIndex dim, int value)->bool
        {
            curDim++;
            SetDimensionInDimensionEntry(&(ptr->DimensionEntries[curDim]), libCZI::Utils::DimensionToChar(dim));
            ptr->DimensionEntries[curDim].Start = value;
            ptr->DimensionEntries[curDim].Size = 1;
            ptr->DimensionEntries[curDim].StartCoordinate = 0;
            ptr->DimensionEntries[curDim].StoredSize = 1;
            return true;
        });

    return 32 + ptr->DimensionCount * 20;
}

/*static*/void CWriterUtils::SetDimensionInDimensionEntry(DimensionEntry* de, char c)
{
    de->Dimension[0] = c;
    de->Dimension[1] = 0;
    de->Dimension[2] = 0;
    de->Dimension[3] = 0;
}

/*static*/size_t CWriterUtils::CalcSubBlockSegmentDataSize(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    size_t dVSize = CWriterUtils::CalcSubBlockDirectoryEntryDVSize(addSbBlkInfo);
    return (std::max)(sizeof(SegmentHeader) + 256, sizeof(SegmentHeader) + 16 + dVSize);
}

/*static*/size_t CWriterUtils::CalcSubBlockDirectoryEntryDVSize(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    int numberOfDimensionEntries = CWriterUtils::CalcCountOfDimensionsEntriesInDirectoryEntryDV(addSbBlkInfo);
    return 32 + sizeof(DimensionEntryDV) * numberOfDimensionEntries;
}

/*static*/int CWriterUtils::CalcCountOfDimensionsEntriesInDirectoryEntryDV(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    int numberOfDimensionEntries = 2;   // we always have X AND Y
    numberOfDimensionEntries += addSbBlkInfo.coordinate.GetValidDimensionsCount();
    if (addSbBlkInfo.mIndexValid)
    {
        ++numberOfDimensionEntries;
    }

    return numberOfDimensionEntries;
}

/*static*/bool CWriterUtils::CalculateSegmentDataSize(const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t* pAllocatedSize, std::uint64_t* pUsedSize)
{
    size_t usedSize = CWriterUtils::CalcSubBlockSegmentDataSize(addSbBlkInfo);

    usedSize += addSbBlkInfo.sizeData;
    usedSize += addSbBlkInfo.sizeMetadata;
    usedSize += addSbBlkInfo.sizeAttachment;

    if (pAllocatedSize != nullptr)
    {
        *pAllocatedSize = CWriterUtils::AlignSegmentSize(usedSize);
    }

    if (pUsedSize != nullptr)
    {
        *pUsedSize = usedSize;
    }

    return true;
}

/*static*/bool CWriterUtils::CalculateSegmentDataSize(const libCZI::AddAttachmentInfo& addAttchmntInfo, std::uint64_t* pAllocatedSize, std::uint64_t* pUsedSize)
{
    uint64_t usedSize = sizeof(AttachmentSegmentData) + addAttchmntInfo.dataSize;
    if (pUsedSize != nullptr)
    {
        *pUsedSize = usedSize;
    }

    if (pAllocatedSize != nullptr)
    {
        *pAllocatedSize = CWriterUtils::AlignSegmentSize(usedSize);
    }

    return true;
}

/*static*/void CWriterUtils::WriteDeletedSegment(const MarkDeletedInfo& info)
{
    uint64_t bytesWritten;
    info.writeFunc(info.segmentPos, CCZIParse::DELETEDSEGMENTMAGIC, sizeof(CCZIParse::DELETEDSEGMENTMAGIC), &bytesWritten, "DELETE SEGMENT");
}

/*static*/std::uint64_t CWriterUtils::WriteZeroes(const WriteInfo& info, std::uint64_t filePos, std::uint64_t count)
{
    return CWriterUtils::WriteZeroes(info.writeFunc, filePos, count);
}

/*static*/std::uint64_t CWriterUtils::WriteZeroes(const std::function<void(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)>& writeFunc, std::uint64_t filePos, std::uint64_t count)
{
    size_t totalBytesWritten = 0;
    std::uint8_t zeroes[4096] = { 0 };
    for (std::uint64_t i = 0; i < (count + sizeof(zeroes) - 1) / sizeof(zeroes); ++i)
    {
        uint64_t bytesWritten;
        const auto bytesToWrite = (std::min)(static_cast<decltype(count)>(sizeof(zeroes)), count);
        writeFunc(filePos + i * sizeof(zeroes), zeroes, bytesToWrite, &bytesWritten, "AligningWithZeroes");
        count -= sizeof(zeroes);
        totalBytesWritten += (size_t)bytesWritten;
    }

    return totalBytesWritten;
}

/*static*/size_t CWriterUtils::WriteSubBlockSegment(const WriteInfo& info, const void* ptrData, size_t dataSize, std::uint64_t filePos)
{
    uint64_t bytesWritten;
    info.writeFunc(filePos, ptrData, dataSize, &bytesWritten, "SubBlockSegment");
    return (size_t)bytesWritten;
}

/*static*/size_t CWriterUtils::WriteSubBlkMetaData(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos)
{
    return CWriterUtils::WriteSubBlkDataGeneric(info, filePos, addSbBlkInfo.sizeMetadata, addSbBlkInfo.getMetaData, "SubBlockMetadata");
}

/*static*/size_t CWriterUtils::WriteSubBlkData(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos)
{
    return CWriterUtils::WriteSubBlkDataGeneric(info, filePos, addSbBlkInfo.sizeData, addSbBlkInfo.getData, "SubBlockData");
}

/*static*/size_t CWriterUtils::WriteSubBlkAttachment(const WriteInfo& info, const libCZI::AddSubBlockInfo& addSbBlkInfo, std::uint64_t filePos)
{
    return CWriterUtils::WriteSubBlkDataGeneric(info, filePos, addSbBlkInfo.sizeAttachment, addSbBlkInfo.getAttachment, "SubBlockAttachment");
}

/*static*/size_t CWriterUtils::WriteSubBlkDataGeneric(const WriteInfo& info, std::uint64_t filePos, size_t size, std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& sizePtr)> getFunc, const char* nameOfPartToWrite)
{
    if (size > 0)
    {
        uint64_t bytesWritten;
        size_t offset = 0;
        for (int i = 0;; ++i)
        {
            size_t sizeData = 0;
            const void* ptr = nullptr;
            bool b = getFunc(i, offset, ptr, sizeData);
            if (b == false)
            {
                if (offset < size)
                {
                    // fill with zeroes if getData-functor has returned false and we have not yet written the
                    // specified number of bytes
                    offset += CWriterUtils::WriteZeroes(info, filePos + offset, size - offset);
                }

                break;
            }

            if (ptr == nullptr)
            {
                stringstream ss;
                ss << "Got an invalid result when requesting data for '" << nameOfPartToWrite << "'";
                throw LibCZIWriteException(ss.str().c_str(), LibCZIWriteException::ErrorType::GetDataCallError);
            }

            auto sizeOfDataToWrite = (std::min)(size - offset, sizeData);
            info.writeFunc(filePos + offset, ptr, sizeOfDataToWrite, &bytesWritten, nameOfPartToWrite);
            offset += bytesWritten;
            if (offset >= size)
            {
                break;
            }
        }

        return offset;
    }

    return 0;
}

/*static*/size_t CWriterUtils::CalcSizeOfSubBlockDirectoryEntryDV(const CCziSubBlockDirectoryBase::SubBlkEntry& entry)
{
    int numberOfDimensionEntries = 2;   // we always have X AND Y
    numberOfDimensionEntries += entry.coordinate.GetValidDimensionsCount();
    if (entry.IsMIndexValid())
    {
        ++numberOfDimensionEntries;
    }

    return 32 + sizeof(DimensionEntryDV) * numberOfDimensionEntries;
}

/*static*/std::tuple<std::uint64_t, std::uint64_t> CWriterUtils::WriteMetadata(const MetadataWriteInfo& info, const libCZI::WriteMetadataInfo& metadataInfo)
{
    size_t payloadTotalLength = metadataInfo.szMetadataSize + metadataInfo.attachmentSize;

    MetadataSegment ms;
    memcpy(ms.header.Id, CCZIParse::METADATASEGMENTMAGIC, 16);
    ms.header.UsedSize = sizeof(MetadataSegmentData) + payloadTotalLength;
    ms.header.AllocatedSize = CWriterUtils::AlignSegmentSize(ms.header.UsedSize);

    ms.data.XmlSize = (int)metadataInfo.szMetadataSize;
    ms.data.AttachmentSize = (int)metadataInfo.attachmentSize;
    memset(ms.data._spare, 0, sizeof(ms.data._spare));

    uint64_t metadataSegmentPos;

    if (int64_t(info.sizeExistingSegmentPos) >= ms.header.AllocatedSize)
    {
        metadataSegmentPos = info.existingSegmentPos;
        ms.header.AllocatedSize = info.sizeExistingSegmentPos;
    }
    else
    {
        // if the existing segment is not large enough, we write "segmenttype=DELETED" into it(which is necessary if it was not a reservation)
        if (info.sizeExistingSegmentPos > 0 && info.existingSegmentPos > 0 && info.markAsDeletedIfExistingSegmentIsNotUsed == true)
        {
            MarkDeletedInfo mark_deleted_info;
            mark_deleted_info.segmentPos = info.existingSegmentPos;
            mark_deleted_info.writeFunc = info.writeFunc;
            WriteDeletedSegment(mark_deleted_info);
        }

        metadataSegmentPos = info.segmentPosForNewSegment;
    }

    uint64_t bytesWritten;
    uint64_t totalBytesWritten = 0;
    const auto msHeaderAllocatedSize = ms.header.AllocatedSize;   // need to save this information before (potentially) changing the byte-order

    ConvertToHostByteOrder::Convert(&ms);
    info.writeFunc(metadataSegmentPos, &ms, sizeof(ms), &bytesWritten, "MetadataSegment");
    totalBytesWritten += bytesWritten;

    if (metadataInfo.szMetadataSize > 0)
    {
        info.writeFunc(metadataSegmentPos + totalBytesWritten, metadataInfo.szMetadata, metadataInfo.szMetadataSize, &bytesWritten, "MetadataData");
        totalBytesWritten += bytesWritten;
    }

    if (metadataInfo.attachmentSize > 0)
    {
        info.writeFunc(metadataSegmentPos + totalBytesWritten, metadataInfo.ptrAttachment, metadataInfo.attachmentSize, &bytesWritten, "MetadataAttachment");
        totalBytesWritten += bytesWritten;
    }

    if (totalBytesWritten < msHeaderAllocatedSize + sizeof(SegmentHeader))
    {
        CWriterUtils::WriteZeroes(info.writeFunc, metadataSegmentPos + totalBytesWritten, msHeaderAllocatedSize + sizeof(SegmentHeader) - totalBytesWritten);
    }

    return make_tuple(metadataSegmentPos, static_cast<std::uint64_t>(msHeaderAllocatedSize));
}

/*static*/std::tuple<std::uint64_t, std::uint64_t> CWriterUtils::WriteSubBlkDirectory(const SubBlkDirWriteInfo& info)
{
    // determine size of "SubBlockDirectorySegmentData"
    size_t totalSizeSubBlockDirectoryEntryDV = 0;
    info.enumEntriesFunc(
        [&totalSizeSubBlockDirectoryEntryDV](size_t index, const CCziSubBlockDirectoryBase::SubBlkEntry& entry)->void
        {
            totalSizeSubBlockDirectoryEntryDV += CalcSizeOfSubBlockDirectoryEntryDV(entry);
        });

    size_t subBlockDirectorySegmentSize = sizeof(SubBlockDirectorySegment) + totalSizeSubBlockDirectoryEntryDV;
    std::unique_ptr<SubBlockDirectorySegment, void(*)(SubBlockDirectorySegment*)> upSbBlkDirSegment((SubBlockDirectorySegment*)malloc(subBlockDirectorySegmentSize), [](SubBlockDirectorySegment* p)->void {free(p); });
    memcpy(upSbBlkDirSegment->header.Id, CCZIParse::SUBBLKDIRMAGIC, 16);

    // the size must be AT LEAST 128 bytes (the _used_ size)
    upSbBlkDirSegment->header.UsedSize = sizeof(SubBlockDirectorySegmentData) + totalSizeSubBlockDirectoryEntryDV;
    upSbBlkDirSegment->header.AllocatedSize = CWriterUtils::AlignSegmentSize(upSbBlkDirSegment->header.UsedSize);
    memset(upSbBlkDirSegment->data._spare, 0, sizeof(upSbBlkDirSegment->data._spare));

    size_t offset = 0;
    std::uint8_t* ptr = ((std::uint8_t*)upSbBlkDirSegment.get()) + sizeof(SubBlockDirectorySegment);
    upSbBlkDirSegment->data.EntryCount = 0;
    info.enumEntriesFunc(
        [&upSbBlkDirSegment, ptr, &offset](size_t index, const CCziSubBlockDirectoryBase::SubBlkEntry& entry)->void
        {
            offset += CWriterUtils::FillSubBlockDirectoryEntryDV((SubBlockDirectoryEntryDV*)(ptr + offset), entry);
            ++upSbBlkDirSegment->data.EntryCount;
        });

    bool reUsedExistingSegment = false;
    uint64_t subBlkDirPos;

    // if we have already written a subblock-directory-segment (possibly a reservation), then we check here if the existing
    // segment is large enough, and if so we write our data into this segment
    if (int64_t(info.sizeExistingSegmentPos) >= upSbBlkDirSegment->header.AllocatedSize)
    {
        subBlkDirPos = info.existingSegmentPos;
        upSbBlkDirSegment->header.AllocatedSize = info.sizeExistingSegmentPos;
        reUsedExistingSegment = true;
    }
    else
    {
        // if the existing segment is not large enough, we write "segmenttype=DELETED" into it(which is necessary if it was not a reservation)
        if (info.sizeExistingSegmentPos > 0 && info.existingSegmentPos > 0 && info.markAsDeletedIfExistingSegmentIsNotUsed == true)
        {
            MarkDeletedInfo mark_deleted_info;
            mark_deleted_info.segmentPos = info.existingSegmentPos;
            mark_deleted_info.writeFunc = info.writeFunc;
            WriteDeletedSegment(mark_deleted_info);
        }
    }

    if (!reUsedExistingSegment)
    {
        subBlkDirPos = info.segmentPosForNewSegment;
    }

    uint64_t bytesWritten;
    const auto sbBlkDirSegmentHeaderAllocatedSize = upSbBlkDirSegment->header.AllocatedSize;

    ConvertToHostByteOrder::ConvertAndAllSubBlkDirEntries(upSbBlkDirSegment.get());
    info.writeFunc(subBlkDirPos, upSbBlkDirSegment.get(), subBlockDirectorySegmentSize, &bytesWritten, "SubBlockDir");

    if (bytesWritten < sbBlkDirSegmentHeaderAllocatedSize + sizeof(SegmentHeader))
    {
        bytesWritten += CWriterUtils::WriteZeroes(info.writeFunc, subBlkDirPos + bytesWritten, sbBlkDirSegmentHeaderAllocatedSize + sizeof(SegmentHeader) - bytesWritten);
    }

    return make_tuple(subBlkDirPos, std::uint64_t(sbBlkDirSegmentHeaderAllocatedSize));
}

/*static*/std::tuple<std::uint64_t, std::uint64_t> CWriterUtils::WriteAttachmentDirectory(const AttachmentDirWriteInfo& info)
{
    AttachmentDirectorySegment attchmntDirSegment = { 0 };
    const auto attchmntCnt = info.entryCnt;// this->attachmentDirectory.GetAttachmentCount();
    const size_t sizeAttchmntEntries = attchmntCnt * sizeof(AttachmentEntryA1);

    memcpy(attchmntDirSegment.header.Id, CCZIParse::ATTACHMENTSDIRMAGC, 16);
    attchmntDirSegment.header.UsedSize = sizeof(AttachmentDirectorySegmentData) + sizeAttchmntEntries;
    attchmntDirSegment.header.AllocatedSize = ((attchmntDirSegment.header.UsedSize + (SEGMENT_ALIGN - 1)) / SEGMENT_ALIGN) * SEGMENT_ALIGN;

    attchmntDirSegment.data.EntryCount = (int)attchmntCnt;

    bool reUsedExistingSegment = false;
    uint64_t attchmDirPos;
    // if we have already written a subblock-directory-segment (possibly a reservation), then we check here if the existing
    // segment is large enough, and if so we write our data into this segment
    if (int64_t(info.sizeExistingSegmentPos) >= attchmntDirSegment.header.UsedSize)
    {
        attchmDirPos = info.existingSegmentPos;// this->attachmentDirectorySegment.GetFilePos();
        attchmntDirSegment.header.AllocatedSize = info.sizeExistingSegmentPos;// this->attachmentDirectorySegment.GetAllocatedSize();
        reUsedExistingSegment = true;
    }
    else
    {
        // if the existing segment is not large enough, we write "segmenttype=DELETED" into it(which is necessary if it was not a reservation)
        if (info.sizeExistingSegmentPos > 0 && info.existingSegmentPos > 0 && info.markAsDeletedIfExistingSegmentIsNotUsed == true)
        {
            MarkDeletedInfo mark_deleted_info;
            mark_deleted_info.segmentPos = info.existingSegmentPos;
            mark_deleted_info.writeFunc = info.writeFunc;
            WriteDeletedSegment(mark_deleted_info);
        }
    }

    if (!reUsedExistingSegment)
    {
        attchmDirPos = info.segmentPosForNewSegment;
    }

    uint64_t  bytesWritten;
    uint64_t  totalBytesWritten = 0;
    const auto attchmntDirSegmentHeaderAllocatedSize = attchmntDirSegment.header.AllocatedSize;

    ConvertToHostByteOrder::Convert(&attchmntDirSegment);
    info.writeFunc(attchmDirPos, &attchmntDirSegment, sizeof(attchmntDirSegment), &bytesWritten, "AttachmentDirSegment");
    totalBytesWritten += bytesWritten;

    std::unique_ptr<AttachmentEntryA1, void(*)(AttachmentEntryA1*)> upAttchmntData((AttachmentEntryA1*)malloc(sizeAttchmntEntries), [](AttachmentEntryA1* p)->void {free(p); });

    size_t index_count = 0;
    info.enumEntriesFunc(
        [&](size_t index, const CCziAttachmentsDirectoryBase::AttachmentEntry& entry)->bool
        {
            AttachmentEntryA1* p = upAttchmntData.get() + index_count;
            p->SchemaType[0] = 'A';
            p->SchemaType[1] = '1';
            memset(&p->_spare[0], 0, sizeof(p->_spare));
            p->FilePosition = entry.FilePosition;
            p->FilePart = 0;
            p->ContentGuid = entry.ContentGuid;
            memcpy(&p->ContentFileType[0], &entry.ContentFileType[0], sizeof(p->ContentFileType));
            memcpy(&p->Name[0], &entry.Name[0], sizeof(p->Name));

            ConvertToHostByteOrder::Convert(p);
            ++index_count;
            return true;
        });

    info.writeFunc(attchmDirPos + totalBytesWritten, upAttchmntData.get(), sizeAttchmntEntries, &bytesWritten, "AttachmentDirData");
    totalBytesWritten += bytesWritten;

    if (totalBytesWritten < attchmntDirSegmentHeaderAllocatedSize + sizeof(SegmentHeader))
    {
        bytesWritten += CWriterUtils::WriteZeroes(info.writeFunc, attchmDirPos + bytesWritten, attchmntDirSegmentHeaderAllocatedSize + sizeof(SegmentHeader) - bytesWritten);
    }

    return make_tuple(attchmDirPos, std::uint64_t(attchmntDirSegmentHeaderAllocatedSize));
}

/// Check the arguments used in the "SyncAddSubBlock"-methods. We throw an "invalid_argument"-exceptions in the case we
/// detect invalid parameters.
///
/// \exception std::invalid_argument Thrown when an invalid argument error condition occurs.
///
/// \param addSbBlkInfo Information describing the sub block to be added.
/*static*/void CWriterUtils::CheckAddSubBlockArguments(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    if (addSbBlkInfo.sizeData > 0 && !addSbBlkInfo.getData)
    {
        throw std::invalid_argument("'getData' must be non-null");
    }

    if (addSbBlkInfo.sizeMetadata > 0 && !addSbBlkInfo.getMetaData)
    {
        throw std::invalid_argument("'getMetaData' must be non-null");
    }

    if (addSbBlkInfo.sizeAttachment > 0 && !addSbBlkInfo.getAttachment)
    {
        throw std::invalid_argument("'getAttachment' must be non-null");
    }

    if (addSbBlkInfo.logicalWidth < 0 || addSbBlkInfo.logicalHeight < 0 || addSbBlkInfo.physicalWidth < 0 || addSbBlkInfo.physicalHeight < 0)
    {
        // TODO: probably a zero width/height is also illegal
        throw std::invalid_argument("invalid width/height");
    }

    if (addSbBlkInfo.compressionModeRaw == Utils::CompressionModeToCompressionIdentifier(CompressionMode::Invalid))
    {
        throw std::invalid_argument("invalid compression-mode");
    }
}

/*static*/void CWriterUtils::CheckAddAttachmentArguments(const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    if (addAttachmentInfo.dataSize > 0 && addAttachmentInfo.ptrData == nullptr)
    {
        throw std::invalid_argument("'ptrData' must be non-null");
    }
}

/// Copies information from an AddSubBlockInfo-struct into an CCziSubBlockDirectoryBase::SubBlkEntry-struct.
/// Note that the member SubBlkEntry.FilePosition will not contain something valid (because this piece is information is not
/// part of the AddSubBlockInfo-struct).
///
/// \param addSbBlkInfo Information describing the sub-block.
///
/// \return A CCziSubBlockDirectoryBase::SubBlkEntry-struct containing the information from the passed-in AddSubBlockInfo-struct.
/*static*/CCziSubBlockDirectoryBase::SubBlkEntry CWriterUtils::SubBlkEntryFromAddSubBlockInfo(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    CCziSubBlockDirectoryBase::SubBlkEntry entry;
    entry.Invalidate();
    entry.coordinate = addSbBlkInfo.coordinate;
    if (addSbBlkInfo.mIndexValid)
    {
        entry.mIndex = addSbBlkInfo.mIndex;
    }

    entry.x = addSbBlkInfo.x;
    entry.y = addSbBlkInfo.y;
    entry.width = addSbBlkInfo.logicalWidth;
    entry.height = addSbBlkInfo.logicalHeight;
    entry.storedWidth = addSbBlkInfo.physicalWidth;
    entry.storedHeight = addSbBlkInfo.physicalHeight;
    entry.PixelType = CziUtils::IntFromPixelType(addSbBlkInfo.PixelType);
    entry.FilePosition = 0;
    entry.Compression = addSbBlkInfo.compressionModeRaw;
    entry.pyramid_type_from_spare = CziUtils::ByteFromPyramidType(addSbBlkInfo.pyramid_type);
    return entry;
}

/// Copies information from an AddAttachmentInfo-struct into an CCziAttachmentsDirectoryBase::AttachmentEntry-struct.
/// Note that the member AttachmentEntry.FilePosition will not contain something valid (because this piece is information is not
/// part of the AddAttachmentInfo-struct).
///
/// \param addAttachmentInfo Information describing the attachment.
///
/// \return A CCziAttachmentsDirectoryBase::AttachmentEntry-struct containing the information from the passed-in AddAttachmentInfo-struct.
/*static*/CCziAttachmentsDirectoryBase::AttachmentEntry CWriterUtils::AttchmntEntryFromAddAttachmentInfo(const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    CCziAttachmentsDirectoryBase::AttachmentEntry entry;
    entry.ContentGuid = addAttachmentInfo.contentGuid;
    memcpy(&entry.ContentFileType, &addAttachmentInfo.contentFileType, sizeof(entry.ContentFileType));
    memcpy(&entry.Name, addAttachmentInfo.name, sizeof(entry.Name));
    entry.FilePosition = 0;
    return entry;
}

/*static*/void CWriterUtils::CheckWriteMetadataArguments(const WriteMetadataInfo& metadataInfo)
{
    if (metadataInfo.szMetadataSize > 0 && metadataInfo.szMetadata == nullptr)
    {
        throw std::invalid_argument("'szMetadata' must be non-null");
    }

    if (metadataInfo.attachmentSize > 0 && metadataInfo.ptrAttachment == nullptr)
    {
        throw std::invalid_argument("'ptrAttachment' must be non-null");
    }
}

// calculate the "allocated" size from the "used size" (taking into account some alignment restrictions).
/*static*/std::uint64_t CWriterUtils::AlignSegmentSize(std::uint64_t usedSize)
{
    return ((usedSize + (SEGMENT_ALIGN - 1)) / SEGMENT_ALIGN) * SEGMENT_ALIGN;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

CCziWriter::CziWriterInfoWrapper::CziWriterInfoWrapper(std::shared_ptr<libCZI::ICziWriterInfo> writerInfo) : writerInfo(writerInfo)
{
    this->fileGuid = Utilities::GenerateNewGuid();
}

//--------------------------------------------------------------------------------------------------

CCziWriter::CCziWriter() : CCziWriter(CZIWriterOptions{})
{}

CCziWriter::CCziWriter(const libCZI::CZIWriterOptions& options) 
    : cziWriterOptions(options), sbBlkDirectory{options.allow_duplicate_subblocks}, nextSegmentPos(0)
{
}

CCziWriter::~CCziWriter()
{
}

/*virtual*/void CCziWriter::Create(std::shared_ptr<libCZI::IOutputStream> stream, std::shared_ptr<libCZI::ICziWriterInfo> info)
{
    this->ThrowIfAlreadyInitialized();

    this->stream = stream;

    if (info)
    {
        if (Utilities::IsGuidNull(info->GetFileGuid()))
        {
            // this wrapper will 'override' the GetFileGuid()-method
            this->info = make_shared<CziWriterInfoWrapper>(info);
        }
        else
        {
            this->info = info;
        }
    }
    else
    {
        this->info = make_shared<CCziWriterInfo>(Utilities::GenerateNewGuid());
    }

    FileHeaderData fhd;
    memset(&fhd, 0, sizeof(fhd));
    fhd.primaryFileGuid = this->info->GetFileGuid();
    this->WriteFileHeader(fhd);

    this->nextSegmentPos = sizeof(FileHeaderSegment);

    size_t s;
    if (this->info->TryGetReservedSizeForMetadataSegment(&s))
    {
        this->ReserveMetadataSegment(s);
    }

    if (this->info->TryGetReservedSizeForSubBlockDirectory(&s))
    {
        this->ReserveSubBlockDirectory(s);
    }

    if (this->info->TryGetReservedSizeForAttachmentDirectory(&s))
    {
        this->ReserveAttachmentDirectory(s);
    }
}

/*virtual*/void CCziWriter::SyncAddSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckAddSubBlockArguments(addSbBlkInfo);

    this->ThrowIfCoordinateIsOutOfBounds(addSbBlkInfo);

    CCziSubBlockDirectoryBase::SubBlkEntry entry = CWriterUtils::SubBlkEntryFromAddSubBlockInfo(addSbBlkInfo);
    entry.FilePosition = this->nextSegmentPos;
    bool b = this->sbBlkDirectory.TryAddSubBlock(entry);
    if (b == false)
    {
        throw LibCZIWriteException("Could not add subblock because it already exists", LibCZIWriteException::ErrorType::AddCoordinateAlreadyExisting);
    }

    this->WriteSubBlock(addSbBlkInfo);
}

/*virtual*/void CCziWriter::SyncAddAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckAddAttachmentArguments(addAttachmentInfo);

    CCziAttachmentsDirectoryBase::AttachmentEntry entry = CWriterUtils::AttchmntEntryFromAddAttachmentInfo(addAttachmentInfo);
    entry.FilePosition = this->nextSegmentPos;
    bool b = this->attachmentDirectory.TryAddAttachment(entry);
    if (b == false)
    {
        throw LibCZIWriteException("Could not add attachment because it already exists", LibCZIWriteException::ErrorType::AddAttachmentAlreadyExisting);
    }

    this->WriteAttachment(addAttachmentInfo);
}

/*virtual*/void CCziWriter::SyncWriteMetadata(const libCZI::WriteMetadataInfo& metadataInfo)
{
    this->ThrowIfNotOperational();

    // check arguments
    CWriterUtils::CheckWriteMetadataArguments(metadataInfo);

    auto mdSegmentPosAndSize = this->WriteMetadata(metadataInfo);
    this->metadataSegment.SetPositionAndAllocatedSize(std::get<0>(mdSegmentPosAndSize), std::get<1>(mdSegmentPosAndSize), false);
}

/*virtual*/std::shared_ptr<libCZI::ICziMetadataBuilder> CCziWriter::GetPreparedMetadata(const PrepareMetadataInfo& info)
{
    this->ThrowIfNotOperational();
    auto spMdBuilder = libCZI::CreateMetadataBuilder();
    MetadataUtils::WriteFillWithSubBlockStatistics(spMdBuilder.get(), this->sbBlkDirectory.GetStatistics());
    CMetadataPrepareHelper::FillDimensionChannel(
        spMdBuilder.get(),
        this->sbBlkDirectory.GetStatistics(),
        this->sbBlkDirectory.GetPixelTypeForChannel(),
        info.funcGenerateIdAndNameForChannel ? info.funcGenerateIdAndNameForChannel : CCziWriter::DefaultGenerateChannelIdAndName);

    return spMdBuilder;
}

/*virtual*/void CCziWriter::Close()
{
    this->ThrowIfNotOperational();
    this->Finish();
    this->nextSegmentPos = 0;
    this->sbBlkDirectory = CWriterCziSubBlockDirectory{ this->cziWriterOptions.allow_duplicate_subblocks };
    this->attachmentDirectory = CWriterCziAttachmentsDirectory();
    this->metadataSegment.Invalidate();
    this->subBlockDirectorySegment.Invalidate();
    this->attachmentDirectorySegment.Invalidate();
    this->stream.reset();
    this->info.reset();
}

//------------------------------------------------------------------------------------------------

void CCziWriter::ThrowIfNotOperational()
{
    if (!this->stream)
    {
        throw logic_error("CZIWriter is not operational (must call 'Create' first).");
    }
}

void CCziWriter::ThrowIfAlreadyInitialized()
{
    if (this->stream)
    {
        throw logic_error("CZIWriter is already operational.");
    }
}

/*static*/std::tuple<std::string, std::tuple<bool, std::string>> CCziWriter::DefaultGenerateChannelIdAndName(int chIdx)
{
    stringstream ssId;
    ssId << "Channel:" << chIdx;
    return make_tuple(ssId.str(), make_tuple(false, string()));
}

void CCziWriter::WriteSubBlock(const libCZI::AddSubBlockInfo& addSbBlkInfo)
{
    CWriterUtils::WriteInfo writeInfo;
    writeInfo.segmentPos = this->nextSegmentPos;
    writeInfo.writeFunc = std::bind(&CCziWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5); ;
    writeInfo.useSpecifiedAllocatedSize = false;
    this->nextSegmentPos += CWriterUtils::WriteSubBlock(writeInfo, addSbBlkInfo);
}

//------------------------------------------------------------------------------------------------

void CCziWriter::WriteToOutputStream(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten, const char* nameOfPartToWrite)
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
        CCziWriter::ThrowNotEnoughDataWritten(offset, size, bytesWritten);
    }

    if (ptrBytesWritten != nullptr)
    {
        *ptrBytesWritten = bytesWritten;
    }
}

/*static*/void CCziWriter::ThrowNotEnoughDataWritten(std::uint64_t offset, std::uint64_t bytesToWrite, std::uint64_t bytesActuallyWritten)
{
    stringstream ss;
    ss << "Not enough data written at offset " << offset << " -> bytes to write: " << bytesToWrite << " bytes, actually written " << bytesActuallyWritten << " bytes.";
    throw LibCZIWriteException(ss.str().c_str(), LibCZIWriteException::ErrorType::NotEnoughDataWritten);
}

void CCziWriter::WriteFileHeader(const FileHeaderData& fhd)
{
    FileHeaderSegment fhs = {};

    fhs.header.AllocatedSize = fhs.header.UsedSize = sizeof(fhs.data);
    memcpy(&fhs.header.Id, &CCZIParse::FILEHDRMAGIC, 16);

    fhs.data.Major = 1;
    fhs.data.Minor = 0;
    fhs.data.PrimaryFileGuid = fhd.primaryFileGuid;
    fhs.data.FileGuid = fhd.primaryFileGuid;
    fhs.data.SubBlockDirectoryPosition = fhd.subBlockDirectoryPosition;
    fhs.data.MetadataPosition = fhd.metadataPosition;
    fhs.data.AttachmentDirectoryPosition = fhd.attachmentDirectoryPosition;

    ConvertToHostByteOrder::Convert(&fhs);
    this->WriteToOutputStream(0, &fhs, sizeof(fhs), nullptr, "FileHeader");
}

void CCziWriter::WriteAttachment(const libCZI::AddAttachmentInfo& addAttachmentInfo)
{
    CWriterUtils::WriteInfo writeInfo;
    writeInfo.segmentPos = this->nextSegmentPos;
    writeInfo.writeFunc = std::bind(&CCziWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5); ;
    writeInfo.useSpecifiedAllocatedSize = false;
    this->nextSegmentPos += CWriterUtils::WriteAttachment(writeInfo, addAttachmentInfo);
}

//------------------------------------------------------------------------------------------------

void CCziWriter::Finish()
{
    this->WriteSubBlkDirectory();
    this->WriteAttachmentDirectory();

    FileHeaderData fhd;
    fhd.Clear();
    fhd.primaryFileGuid = this->info->GetFileGuid();

    if (this->subBlockDirectorySegment.IsValid() && !this->subBlockDirectorySegment.GetIsMarkedAsDeleted())
    {
        fhd.subBlockDirectoryPosition = this->subBlockDirectorySegment.GetFilePos();
    }

    if (this->metadataSegment.IsValid() && !this->metadataSegment.GetIsMarkedAsDeleted())
    {
        fhd.metadataPosition = this->metadataSegment.GetFilePos();
    }

    if (this->attachmentDirectorySegment.IsValid() && !this->attachmentDirectorySegment.GetIsMarkedAsDeleted())
    {
        fhd.attachmentDirectoryPosition = this->attachmentDirectorySegment.GetFilePos();
    }

    this->WriteFileHeader(fhd);
}

void CCziWriter::WriteSubBlkDirectory()
{
    auto mdSegmentPosAndSize = this->WriteCurrentSubBlkDirectory();
    this->subBlockDirectorySegment.SetPositionAndAllocatedSize(std::get<0>(mdSegmentPosAndSize), std::get<1>(mdSegmentPosAndSize), false);
}

void CCziWriter::WriteAttachmentDirectory()
{
    if (this->attachmentDirectory.GetAttachmentCount() > 0)
    {
        auto adSegmentPosAndSize = this->WriteCurrentAttachmentsDirectory();
        this->attachmentDirectorySegment.SetPositionAndAllocatedSize(std::get<0>(adSegmentPosAndSize), std::get<1>(adSegmentPosAndSize), false);
    }
}

std::tuple<std::uint64_t, std::uint64_t> CCziWriter::WriteCurrentAttachmentsDirectory()
{
    CWriterUtils::AttachmentDirWriteInfo info;
    if (this->attachmentDirectorySegment.IsValid())
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = !this->attachmentDirectorySegment.GetIsMarkedAsDeleted();
        info.existingSegmentPos = this->attachmentDirectorySegment.GetFilePos();
        info.sizeExistingSegmentPos = this->attachmentDirectorySegment.GetAllocatedSize();
    }
    else
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = false;
        info.existingSegmentPos = 0;
        info.sizeExistingSegmentPos = 0;
    }
    info.segmentPosForNewSegment = this->nextSegmentPos;
    info.entryCnt = this->attachmentDirectory.GetAttachmentCount();
    info.enumEntriesFunc = [&](const std::function<void(size_t, const CCziAttachmentsDirectoryBase::AttachmentEntry&)>& f)->void
        {
            this->attachmentDirectory.EnumEntries([&](size_t index, const CCziAttachmentsDirectoryBase::AttachmentEntry& e)->bool
                {
                    f(index, e);
                    return true;
                });
        };
    info.writeFunc = std::bind(&CCziWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    auto posAndSize = CWriterUtils::WriteAttachmentDirectory(info);
    if (get<0>(posAndSize) == info.segmentPosForNewSegment)
    {
        this->nextSegmentPos += get<1>(posAndSize) + sizeof(SegmentHeader);
    }

    return posAndSize;
}

/// Writes the subblock-directory-segment to output. The information put into this segment is derived from the sub-blocks
/// that have been added to this instance (and this data is internally maintained). 
/// The segment is appeneded to the file at the next available fileposition.
///
/// \return A std::tuple&lt;std::uint64_t,std::uint64_t&gt; giving the file-position (first parameter) and
///         the "allocated-size" of the segment.
std::tuple<std::uint64_t, std::uint64_t> CCziWriter::WriteCurrentSubBlkDirectory()
{
    CWriterUtils::SubBlkDirWriteInfo info;
    if (this->subBlockDirectorySegment.IsValid())
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = !this->subBlockDirectorySegment.GetIsMarkedAsDeleted();
        info.existingSegmentPos = this->subBlockDirectorySegment.GetFilePos();
        info.sizeExistingSegmentPos = this->subBlockDirectorySegment.GetAllocatedSize();
    }
    else
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = false;
        info.existingSegmentPos = 0;
        info.sizeExistingSegmentPos = 0;
    }

    info.segmentPosForNewSegment = this->nextSegmentPos;
    info.enumEntriesFunc = [&](const std::function<void(size_t, const CCziSubBlockDirectoryBase::SubBlkEntry&)>& f)->void
        {
            this->sbBlkDirectory.EnumEntries([&](size_t index, const CCziSubBlockDirectoryBase::SubBlkEntry& e)->bool
                {
                    f(index, e);
                    return true;
                });
        };
    info.writeFunc = std::bind(&CCziWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    auto posAndSize = CWriterUtils::WriteSubBlkDirectory(info);
    if (get<0>(posAndSize) == info.segmentPosForNewSegment)
    {
        this->nextSegmentPos += get<1>(posAndSize) + sizeof(SegmentHeader);;
    }

    return posAndSize;
}

/// Writes the metadata-segment to output. The segment is appeneded to the file at the next available fileposition.
///
/// \param metadataInfo Information describing the metadata.
///
/// \return A std::tuple<std::uint64_t,std::uint64_t>; giving the file-position (first parameter) and
///         the "allocated-size" of the segment.
std::tuple<std::uint64_t, std::uint64_t> CCziWriter::WriteMetadata(const libCZI::WriteMetadataInfo& metadataInfo)
{
    CWriterUtils::MetadataWriteInfo info;
    if (this->metadataSegment.IsValid())
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = !this->metadataSegment.GetIsMarkedAsDeleted();
        info.existingSegmentPos = this->metadataSegment.GetFilePos();
        info.sizeExistingSegmentPos = this->metadataSegment.GetAllocatedSize();
    }
    else
    {
        info.markAsDeletedIfExistingSegmentIsNotUsed = false;
        info.existingSegmentPos = 0;
        info.sizeExistingSegmentPos = 0;
    }

    info.segmentPosForNewSegment = this->nextSegmentPos;

    info.writeFunc = std::bind(&CCziWriter::WriteToOutputStream, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
    auto posAndSize = CWriterUtils::WriteMetadata(info, metadataInfo);
    if (get<0>(posAndSize) == info.segmentPosForNewSegment)
    {
        this->nextSegmentPos += get<1>(posAndSize) + sizeof(SegmentHeader);
    }

    return posAndSize;
}

void CCziWriter::ThrowIfCoordinateIsOutOfBounds(const libCZI::AddSubBlockInfo& addSbBlkInfo) const
{
    const auto r = this->CheckCoordinate(addSbBlkInfo);
    switch (r)
    {
    case SbBlkCoordinateCheckResult::Ok:
        return;
    case SbBlkCoordinateCheckResult::OutOfBounds:
        throw LibCZIWriteException("coordinate out-of-bounds", LibCZIWriteException::ErrorType::SubBlockCoordinateOutOfBounds);
    case SbBlkCoordinateCheckResult::InsufficientCoordinate:
        throw LibCZIWriteException("coordinate insufficient", LibCZIWriteException::ErrorType::SubBlockCoordinateInsufficient);
    case SbBlkCoordinateCheckResult::UnexpectedCoordinate:
        throw LibCZIWriteException("unexpected dimension", LibCZIWriteException::ErrorType::AddCoordinateContainsUnexpectedDimension);
    }
}

CCziWriter::SbBlkCoordinateCheckResult CCziWriter::CheckCoordinate(const libCZI::AddSubBlockInfo& addSbBlkInfo) const
{
    const IDimBounds* dimBounds = this->info->GetDimBounds();
    if (dimBounds == nullptr)
    {
        return SbBlkCoordinateCheckResult::Ok;
    }

    int cntOfValidDimensionsInDimBounds = 0;
    for (auto i = static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MinDim); i <= static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MaxDim); ++i)
    {
        int startIndex, sizeIndex;
        if (dimBounds->TryGetInterval(static_cast<DimensionIndex>(i), &startIndex, &sizeIndex))
        {
            int coord;
            bool b = addSbBlkInfo.coordinate.TryGetPosition(static_cast<DimensionIndex>(i), &coord);
            if (b == false)
            {
                return SbBlkCoordinateCheckResult::InsufficientCoordinate;
            }

            if (!(startIndex <= coord && (startIndex + sizeIndex) > coord))
            {
                return SbBlkCoordinateCheckResult::OutOfBounds;
            }

            ++cntOfValidDimensionsInDimBounds;
        }
    }

    if (cntOfValidDimensionsInDimBounds != addSbBlkInfo.coordinate.GetNumberOfValidDimensions())
    {
        return SbBlkCoordinateCheckResult::UnexpectedCoordinate;
    }

    int mMin, mMax;
    if (this->info->TryGetMIndexMinMax(&mMin, &mMax))
    {
        if (addSbBlkInfo.mIndexValid == false)
        {
            // if there is a bounds for M-index given, we require that an M-index is present
            return SbBlkCoordinateCheckResult::InsufficientCoordinate;
        }

        if (!(mMin <= addSbBlkInfo.mIndex && mMax >= addSbBlkInfo.mIndex))
        {
            return SbBlkCoordinateCheckResult::OutOfBounds;
        }
    }

    return SbBlkCoordinateCheckResult::Ok;
}

void CCziWriter::ReserveMetadataSegment(size_t s)
{
    if (s == 0)
    {
        // default size: 10KiB
        s = 10 * 1024;
    }

    MetadataSegment ms = { 0 };
    memcpy(ms.header.Id, CCZIParse::DELETEDSEGMENTMAGIC, 16);
    ms.header.UsedSize = sizeof(MetadataSegmentData) + s;
    ms.header.AllocatedSize = CWriterUtils::AlignSegmentSize(ms.header.UsedSize);
    ms.data.XmlSize = 0;
    ms.data.AttachmentSize = 0;

    uint64_t bytesWritten;
    const auto msHeaderAllocatedSize = ms.header.AllocatedSize;

    ConvertToHostByteOrder::Convert(&ms);
    this->WriteToOutputStream(this->nextSegmentPos, &ms, sizeof(ms), &bytesWritten, "MetadataSegment-Reservation");

    this->metadataSegment.SetPositionAndAllocatedSize(this->nextSegmentPos, msHeaderAllocatedSize, true);

    this->nextSegmentPos += sizeof(SegmentHeader) + msHeaderAllocatedSize;
}

void CCziWriter::ReserveSubBlockDirectory(size_t s)
{
    if (s == 0)
    {
        // try to determine the number of subblocks from the specified bounds
        auto bounds = this->info->GetDimBounds();
        if (bounds != nullptr)
        {
            for (auto i = static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MinDim); i <= static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MaxDim); ++i)
            {
                int size;
                if (bounds->TryGetInterval(static_cast<DimensionIndex>(i), nullptr, &size))
                {
                    s = (s != 0) ? (s * size) : size;
                }
            }
        }

        // multiply with the tile-count (if present)
        int mMin, mMax;
        if (this->info->TryGetMIndexMinMax(&mMin, &mMax))
        {
            s = (s != 0) ? (s * (mMax - mMin + 1)) : (mMax - mMin + 1);
        }

        if (s == 0)
        {
            // if still indeterminate... use a default
            s = 10;
        }
    }

    SubBlockDirectorySegment sds = { 0 };

    memcpy(sds.header.Id, CCZIParse::DELETEDSEGMENTMAGIC, 16);
    sds.header.UsedSize = sizeof(SubBlockDirectorySegmentData) + s * (32 + sizeof(DimensionEntryDV) * MAXDIMENSIONS);
    sds.header.AllocatedSize = CWriterUtils::AlignSegmentSize(sds.header.UsedSize);

    uint64_t bytesWritten;
    const auto sdsHeaderAllocatedSize = sds.header.AllocatedSize;

    ConvertToHostByteOrder::Convert(&sds);
    this->WriteToOutputStream(this->nextSegmentPos, &sds, sizeof(sds), &bytesWritten, "SubblockDirectorySegment-Reservation");

    this->subBlockDirectorySegment.SetPositionAndAllocatedSize(this->nextSegmentPos, sdsHeaderAllocatedSize, true);

    this->nextSegmentPos += sizeof(SegmentHeader) + sdsHeaderAllocatedSize;
}

void CCziWriter::ReserveAttachmentDirectory(size_t s)
{
    if (s == 0)
    {
        s = 10;
    }

    AttachmentDirectorySegment ads = { 0 };

    memcpy(ads.header.Id, CCZIParse::DELETEDSEGMENTMAGIC, 16);
    ads.header.UsedSize = sizeof(AttachmentDirectorySegmentData) + s * sizeof(AttachmentEntryA1);
    ads.header.AllocatedSize = CWriterUtils::AlignSegmentSize(ads.header.UsedSize);

    uint64_t bytesWritten;
    const auto adsHeaderAllocatedSize = ads.header.AllocatedSize;

    ConvertToHostByteOrder::Convert(&ads);
    this->WriteToOutputStream(this->nextSegmentPos, &ads, sizeof(ads), &bytesWritten, "AttachmentDirectorySegment-Reservation");

    this->attachmentDirectorySegment.SetPositionAndAllocatedSize(this->nextSegmentPos, adsHeaderAllocatedSize, true);

    this->nextSegmentPos += sizeof(SegmentHeader) + adsHeaderAllocatedSize;
}
