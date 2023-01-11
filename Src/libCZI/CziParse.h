// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>

#include "CziSubBlockDirectory.h"
#include "CziAttachmentsDirectory.h"
#include "FileHeaderSegmentData.h"
#include "libCZI.h"

class CCZIParse
{
    friend class CCZIParseTest;
public:
    static const std::uint8_t FILEHDRMAGIC[16];
    static const std::uint8_t SUBBLKDIRMAGIC[16];
    static const std::uint8_t SUBBLKMAGIC[16];
    static const std::uint8_t METADATASEGMENTMAGIC[16];
    static const std::uint8_t ATTACHMENTSDIRMAGC[16];
    static const std::uint8_t ATTACHMENTBLKMAGIC[16];
    static const std::uint8_t DELETEDSEGMENTMAGIC[16];
public:
    enum class SegmentType
    {
        SbBlkDirectory,
        SbBlk,
        AttchmntDirectory,
        Attachment,
        Metadata
    };
    struct SegmentSizes
    {
        std::int64_t AllocatedSize;
        std::int64_t UsedSize;
        std::int64_t GetTotalSegmentSize() const { return this->AllocatedSize + sizeof(SegmentHeader); }
    };

    static FileHeaderSegmentData ReadFileHeaderSegment(libCZI::IStream* str);
    static CFileHeaderSegmentData ReadFileHeaderSegmentData(libCZI::IStream* str);

    static CCziSubBlockDirectory ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset);
    static CCziAttachmentsDirectory ReadAttachmentsDirectory(libCZI::IStream* str, std::uint64_t offset);
    static void ReadAttachmentsDirectory(libCZI::IStream* str, std::uint64_t offset, const std::function<void(const CCziAttachmentsDirectoryBase::AttachmentEntry&)>& addFunc, SegmentSizes* segmentSizes = nullptr);

    static void ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset, CCziSubBlockDirectory& subBlkDir);

    static void ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc, SegmentSizes* segmentSizes = nullptr);

    struct SubBlockStorageAllocate
    {
        std::function<void* (size_t size)> alloc;
        std::function<void(void*)> free;
    };

    struct SubBlockData
    {
        void*           ptrData;
        std::uint64_t	dataSize;
        void*           ptrAttachment;
        std::uint32_t	attachmentSize;
        void*           ptrMetadata;
        std::uint32_t	metaDataSize;

        int						compression;
        int						pixelType;
        libCZI::CDimCoordinate	coordinate;
        libCZI::IntRect			logicalRect;
        libCZI::IntSize			physicalSize;
        int						mIndex;			// if not present, then this is int::max
    };

    static SubBlockData ReadSubBlock(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    struct MetadataSegmentData
    {
        void*           ptrXmlData;
        std::uint64_t	xmlDataSize;
        void*           ptrAttachment;
        std::uint32_t	attachmentSize;
    };

    static MetadataSegmentData ReadMetadataSegment(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    struct AttachmentData
    {
        void*           ptrData;
        std::uint64_t	dataSize;
    };

    static AttachmentData ReadAttachment(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    static CCZIParse::SegmentSizes ReadSegmentHeader(SegmentType type, libCZI::IStream* str, std::uint64_t pos);
    static CCZIParse::SegmentSizes ReadSegmentHeaderAny(libCZI::IStream* str, std::uint64_t pos);
private:
    static void ParseThroughDirectoryEntries(int count, const std::function<void(int, void*)>& funcRead, const std::function<void(const SubBlockDirectoryEntryDE*, const SubBlockDirectoryEntryDV*)>& funcAddEntry);

    static void AddEntryToSubBlockDirectory(const SubBlockDirectoryEntryDE* subBlkDirDE, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc);
    static void AddEntryToSubBlockDirectory(const SubBlockDirectoryEntryDV* subBlkDirDE, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc);

    static libCZI::DimensionIndex DimensionCharToDimensionIndex(const char* ptr, size_t size);
    static bool IsMDimension(const char* ptr, size_t size);
    static bool IsXDimension(const char* ptr, size_t size);
    static bool IsYDimension(const char* ptr, size_t size);
    static char ToUpperCase(char c);

    static void ThrowNotEnoughDataRead(std::uint64_t offset, std::uint64_t bytesRequested, std::uint64_t bytesActuallyRead);
    static void ThrowIllegalData(std::uint64_t offset, const char* sz);
    static void ThrowIllegalData(const char* sz);

    static bool CheckAttachmentSchemaType(const char* p, size_t cnt);
};