// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>
#include <bitset>

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
    /// Options for parsing the subblock-directory are gathered in this struct.
    /// The default value is to do "lax parsing".
    struct SubblockDirectoryParseOptions
    {
    private:
        enum class ParseFlags : std::uint8_t
        {
            kDimensionXyMustBePresent = 0,
            kDimensionOtherThanMMustHaveSizeOne,
            kPhysicalDimensionOtherThanMMustHaveSizeOne,
            kDimensionMMustHaveSizeOneExceptForPyramidSubblocks,
            kDimensionMMustHaveSizeOne,

            kParseFlagsCount    ///< The number of flags - this is not a flag itself, and it must be the last entry in the enum.
        };
        std::bitset<static_cast<std::underlying_type<ParseFlags>::type>(ParseFlags::kParseFlagsCount)> flags;
    public:
        /// Require that for each subblock, the dimensions X and Y are present.
        /// 
        /// \param  enable  True to enable, false to disable.
        void SetDimensionXyMustBePresent(bool enable) { return this->SetFlag(ParseFlags::kDimensionXyMustBePresent, enable); }

        /// Require that for each subblock the physical size (for all dimensions other than X, Y and M) is "1".
        ///
        /// \param  enable  True to enable, false to disable.
        void SetPhysicalDimensionOtherThanMMustHaveSizeOne(bool enable) { return this->SetFlag(ParseFlags::kPhysicalDimensionOtherThanMMustHaveSizeOne, enable); }

        /// Require that for each subblock the size (for all dimensions other than X, Y and M) is "1".
        ///
        /// \param  enable  True to enable, false to disable.

        void SetDimensionOtherThanMMustHaveSizeOne(bool enable) { return this->SetFlag(ParseFlags::kDimensionOtherThanMMustHaveSizeOne, enable); }
        /// Require that for all subblocks that the size of dimension M is "1" except for pyramid subblocks.
        ///
        /// \param  enable  True to enable, false to disable.
        void SetDimensionMMustHaveSizeOneExceptForPyramidSubblocks(bool enable) { return this->SetFlag(ParseFlags::kDimensionMMustHaveSizeOneExceptForPyramidSubblocks, enable); }

        /// Require that for all subblocks that the size of dimension M is "1" (without exceptions).
        ///
        /// \param  enable  True to enable, false to disable.
        void SetDimensionMMustHaveSizeOne(bool enable) { return this->SetFlag(ParseFlags::kDimensionMMustHaveSizeOne, enable); }

        /// Gets a boolean indicating whether to check that the dimensions X and Y are be present for each subblock.
        ///
        /// \returns    True if it is to be checked whether all subblocks have an X and Y dimension specified; false otherwise.
        bool GetDimensionXyMustBePresent() const { return this->GetFlag(ParseFlags::kDimensionXyMustBePresent); }

        /// Gets a boolean indicating whether to check that the size of all dimensions other than X, Y and M is "1" for each subblock.
        ///
        /// \returns    True if it is to be checked that the size of all dimensions other than X, Y and M is "1" for each subblock; false otherwise.
        bool GetDimensionOtherThanMMustHaveSizeOne() const { return this->GetFlag(ParseFlags::kDimensionOtherThanMMustHaveSizeOne); }

        /// Gets a boolean indicating whether to check that the physical size of all dimensions other than X, Y and M is "1" for each subblock.
        ///
        /// \returns    True if it is to be checked that the physical size of all dimensions other than X, Y and M is "1" for each subblock; false otherwise.
        bool GetPhysicalDimensionOtherThanMMustHaveSizeOne() const { return this->GetFlag(ParseFlags::kPhysicalDimensionOtherThanMMustHaveSizeOne); }

        /// Gets a boolean indicating whether to check that the size is "1" for dimension M for all non-pyramid-subblocks.
        /// This flag is more specific than the flag "DimensionMMustHaveSizeOne".
        ///
        /// \returns    True if it is to be checked that the is "1" for dimension M for all non-pyramid-subblocks; false otherwise.
        bool GetDimensionMMustHaveSizeOneForPyramidSubblocks() const { return this->GetFlag(ParseFlags::kDimensionMMustHaveSizeOneExceptForPyramidSubblocks); }

        /// Gets a boolean indicating whether to check that the size is "1" for dimension M for all subblocks.
        ///
        /// \returns    True if it is to be checked that the is "1" for dimension M for all subblocks; false otherwise.
        bool GetDimensionMMustHaveSizeOne() const { return this->GetFlag(ParseFlags::kDimensionMMustHaveSizeOne); }

        /// Sets options to "lax parsing". This is the default.
        void SetLaxParsing()
        {
            this->SetDimensionXyMustBePresent(false);
            this->SetDimensionOtherThanMMustHaveSizeOne(false);
            this->SetPhysicalDimensionOtherThanMMustHaveSizeOne(false);
            this->SetDimensionMMustHaveSizeOne(false);
        }

        /// Sets strict parsing - all options are enabled.
        void SetStrictParsing()
        {
            this->SetDimensionXyMustBePresent(true);
            this->SetPhysicalDimensionOtherThanMMustHaveSizeOne(true);
            this->SetDimensionOtherThanMMustHaveSizeOne(true);
            this->SetDimensionMMustHaveSizeOne(true);
        }
    private:
        void SetFlag(ParseFlags flag, bool enable);
        bool GetFlag(ParseFlags flag) const;
    };

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

    /// Parse the subblock-directory from the specified stream at the specified offset.
    /// Historically, libCZI did not check whether the elements in the dimensions-entry-list had
    /// a size other than "1" given (for all dimensions other than X and Y). We refer to this as
    /// "lax parsing". If the argument lax_subblock_coordinate_checks is true, then we check for
    /// those sizes to be as expected and otherwise throw an exception.
    ///
    /// \param [in,out] str     The stream to read from.
    /// \param          offset  The offset in the stream.
    /// \param          options Options controlling the operation, allowing to choose various variants for parsing.
    ///
    /// \returns    An in-memory representation of the subblock-directory.
    static CCziSubBlockDirectory ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset, const SubblockDirectoryParseOptions& options);

    static CCziAttachmentsDirectory ReadAttachmentsDirectory(libCZI::IStream* str, std::uint64_t offset);
    static void ReadAttachmentsDirectory(libCZI::IStream* str, std::uint64_t offset, const std::function<void(const CCziAttachmentsDirectoryBase::AttachmentEntry&)>& addFunc, SegmentSizes* segmentSizes);

    static void ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset, CCziSubBlockDirectory& subBlkDir, const SubblockDirectoryParseOptions& options);

    static void ReadSubBlockDirectory(libCZI::IStream* str, std::uint64_t offset, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc, const SubblockDirectoryParseOptions& options, SegmentSizes* segmentSizes);

    struct SubBlockStorageAllocate
    {
        std::function<void* (size_t size)> alloc;
        std::function<void(void*)> free;
    };

    struct SubBlockData
    {
        void* ptrData;
        std::uint64_t   dataSize;
        void* ptrAttachment;
        std::uint32_t   attachmentSize;
        void* ptrMetadata;
        std::uint32_t   metaDataSize;

        int                     compression;
        int                     pixelType;
        libCZI::CDimCoordinate  coordinate;
        libCZI::IntRect         logicalRect;
        libCZI::IntSize         physicalSize;
        int                     mIndex;         // if not present, then this is int::max
        std::uint8_t            spare[6];   
    };

    static SubBlockData ReadSubBlock(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    struct MetadataSegmentData
    {
        void* ptrXmlData;
        std::uint64_t   xmlDataSize;
        void* ptrAttachment;
        std::uint32_t   attachmentSize;
    };

    static MetadataSegmentData ReadMetadataSegment(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    struct AttachmentData
    {
        void* ptrData;
        std::uint64_t   dataSize;
    };

    static AttachmentData ReadAttachment(libCZI::IStream* str, std::uint64_t offset, const SubBlockStorageAllocate& allocateInfo);

    static CCZIParse::SegmentSizes ReadSegmentHeader(SegmentType type, libCZI::IStream* str, std::uint64_t pos);
    static CCZIParse::SegmentSizes ReadSegmentHeaderAny(libCZI::IStream* str, std::uint64_t pos);
private:
    static void ParseThroughDirectoryEntries(int count, const std::function<void(int, void*)>& funcRead, const std::function<void(const SubBlockDirectoryEntryDE*, const SubBlockDirectoryEntryDV*)>& funcAddEntry);

    static void AddEntryToSubBlockDirectory(const SubBlockDirectoryEntryDE* subBlkDirDE, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc);
    static void AddEntryToSubBlockDirectory(const SubBlockDirectoryEntryDV* subBlkDirDV, const std::function<void(const CCziSubBlockDirectoryBase::SubBlkEntry&)>& addFunc, const SubblockDirectoryParseOptions& options);

    static libCZI::DimensionIndex DimensionCharToDimensionIndex(const char* ptr, size_t size);
    static bool IsMDimension(const char* ptr, size_t size);
    static bool IsXDimension(const char* ptr, size_t size);
    static bool IsYDimension(const char* ptr, size_t size);
    static char ToUpperCase(char c);

    [[noreturn]] static void ThrowNotEnoughDataRead(std::uint64_t offset, std::uint64_t bytesRequested, std::uint64_t bytesActuallyRead);
    [[noreturn]] static void ThrowIllegalData(std::uint64_t offset, const char* sz);
    [[noreturn]] static void ThrowIllegalData(const char* sz);

    static bool CheckAttachmentSchemaType(const char* p, size_t cnt);
};
