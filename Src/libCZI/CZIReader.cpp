// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <utility>
#include "CZIReader.h"
#include "CziParse.h"
#include "CziSubBlock.h"
#include "CziMetadataSegment.h"
#include "CziUtils.h"
#include "utilities.h"
#include "CziAttachment.h"
#include "CziReaderCommon.h"

using namespace std;
using namespace libCZI;

static CCZIParse::SubblockDirectoryParseOptions GetParseOptionsFromOpenOptions(const ICZIReader::OpenOptions& options)
{
    CCZIParse::SubblockDirectoryParseOptions parse_options;
    if (options.lax_subblock_coordinate_checks == false)
    {
        parse_options.SetStrictParsing();
        if (options.ignore_sizem_for_pyramid_subblocks)
        {
            // in this case we require only that non-pyramid-subblocks have a SizeM=1
            parse_options.SetDimensionMMustHaveSizeOne(false);
            parse_options.SetDimensionMMustHaveSizeOneExceptForPyramidSubblocks(true);
        }
    }

    return parse_options;
}

CCZIReader::CCZIReader() : isOperational(false)
{
}

/*virtual */void CCZIReader::Open(const std::shared_ptr<libCZI::IStream>& stream, const ICZIReader::OpenOptions* options)
{
    if (this->isOperational == true)
    {
        throw logic_error("CZIReader is already operational.");
    }

    if (options == nullptr)
    {
        constexpr auto default_options = OpenOptions{};
        return CCZIReader::Open(stream, &default_options);
    }

    this->hdrSegmentData = CCZIParse::ReadFileHeaderSegmentData(stream.get());
    this->subBlkDir = CCZIParse::ReadSubBlockDirectory(stream.get(), this->hdrSegmentData.GetSubBlockDirectoryPosition(), GetParseOptionsFromOpenOptions(*options));
    const auto attachmentPos = this->hdrSegmentData.GetAttachmentDirectoryPosition();
    if (attachmentPos != 0)
    {
        // we should be operational without an attachment-directory as well I suppose.
        // TODO: how to determine whether there is "no attachment-directory" - is the check for 0 sufficient?
        this->attachmentDir = CCZIParse::ReadAttachmentsDirectory(stream.get(), attachmentPos);
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
            return funcEnum(index, CziReaderCommon::ConvertToSubBlockInfo(entry));
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
            info.pyramidType = CziUtils::PyramidTypeFromByte(entry.pyramid_type_from_spare);
            info.filePosition = entry.FilePosition;
            return funcEnum(index, info);
        });
}

/*virtual*/void CCZIReader::EnumSubset(const IDimCoordinate* planeCoordinate, const IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const SubBlockInfo& info)>& funcEnum)
{
    this->ThrowIfNotOperational();
    CziReaderCommon::EnumSubset(this, planeCoordinate, roi, onlyLayer0, funcEnum);
}

/*virtual*/std::shared_ptr<ISubBlock> CCZIReader::ReadSubBlock(int index)
{
    this->ThrowIfNotOperational();
    CCziSubBlockDirectory::SubBlkEntry entry;
    if (this->subBlkDir.TryGetSubBlock(index, entry) == false)
    {
        return {};
    }

    return this->ReadSubBlock(entry);
}

/*virtual*/bool CCZIReader::TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, SubBlockInfo& info)
{
    this->ThrowIfNotOperational();
    return CziReaderCommon::TryGetSubBlockInfoOfArbitrarySubBlockInChannel(this, channelIndex, info);
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
        *info = CziReaderCommon::ConvertToSubBlockInfo(entry);
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

    // We need to have a critical-section around modifying the stream-shared_ptr - there may be concurrent calls to ReadSubBlock, ReadAttachment, etc.
    //  in which the stream-shared_ptr is accessed. While the stream-shared_ptr is thread-safe, it is not thread-safe to reset it while another thread
    //  is dealing with the same shared_ptr. C.f. https://stackoverflow.com/questions/14482830/stdshared-ptr-thread-safety. With C++20 we could use 
    //  atomic<shared_ptr> instead of the manual critical-section (c.f. https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic2).
    std::unique_lock<std::mutex> lock(this->stream_mutex_);
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

/*virtual*/void CCZIReader::EnumerateSubset(const char* contentFileType, const char* name, const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum)
{
    this->ThrowIfNotOperational();
    CziReaderCommon::EnumerateSubset(
        std::bind(&CCziAttachmentsDirectory::EnumAttachments, &this->attachmentDir, std::placeholders::_1),
        contentFileType, 
        name, 
        funcEnum);
}

/*virtual*/std::shared_ptr<libCZI::IAttachment> CCZIReader::ReadAttachment(int index)
{
    this->ThrowIfNotOperational();
    CCziAttachmentsDirectory::AttachmentEntry entry;
    if (this->attachmentDir.TryGetAttachment(index, entry) == false)
    {
        return {};
    }

    return this->ReadAttachment(entry);
}

std::shared_ptr<ISubBlock> CCZIReader::ReadSubBlock(const CCziSubBlockDirectory::SubBlkEntry& entry)
{
    const CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

    // For thread-safety, we need to ensure that we hold a reference to the stream for the whole duration of the call, 
    //  in order to prepare for concurrent calls to Close() (which will reset the stream-shared_ptr).
    shared_ptr<libCZI::IStream> stream_reference;

    {
        unique_lock<mutex> lock(this->stream_mutex_);
        stream_reference = this->stream;
    }

    if (!stream_reference)
    {
        throw logic_error("CZIReader::ReadSubBlock: stream is null (Close was already called for this instance)");
    }

    auto subBlkData = CCZIParse::ReadSubBlock(stream_reference.get(), entry.FilePosition, allocateInfo);

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

std::shared_ptr<libCZI::IAttachment> CCZIReader::ReadAttachment(const CCziAttachmentsDirectory::AttachmentEntry& entry)
{
    const CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

    shared_ptr<libCZI::IStream> stream_reference;

    {
        unique_lock<mutex> lock(this->stream_mutex_);
        stream_reference = this->stream;
    }

    if (!stream_reference)
    {
        throw logic_error("CCZIReader::ReadAttachment: stream is null (Close was already called for this instance)");
    }

    auto attchmnt = CCZIParse::ReadAttachment(stream_reference.get(), entry.FilePosition, allocateInfo);
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
    const CCZIParse::SubBlockStorageAllocate allocateInfo{ malloc,free };

    shared_ptr<libCZI::IStream> stream_reference;

    {
        unique_lock<mutex> lock(this->stream_mutex_);
        stream_reference = this->stream;
    }

    if (!stream_reference)
    {
        throw logic_error("CCZIReader::ReadAttachment: stream is null (Close was already called for this instance)");
    }

    auto metaDataSegmentData = CCZIParse::ReadMetadataSegment(stream_reference.get(), position, allocateInfo);
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
