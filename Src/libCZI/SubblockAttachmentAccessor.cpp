#include "SubblockAttachmentAccessor.h"
#include "libCZI.h"
#include "utilities.h"

using namespace libCZI;
using namespace std;

SubblockAttachmentAccessor::SubblockAttachmentAccessor(std::shared_ptr<libCZI::ISubBlock> sub_block, std::shared_ptr<libCZI::ISubBlockMetadata> sub_block_metadata)
    : sub_block_(std::move(sub_block)),
      sub_block_metadata_(std::move(sub_block_metadata))
{
    if (sub_block_metadata_->IsXmlValid())
    {
        std::wstring attachment_data_format;
        if (sub_block_metadata_->TryGetAttachmentDataFormat(&attachment_data_format) &&
            attachment_data_format == L"CHUNKCONTAINER")
        {
            has_chunk_container_ = true;
        }
    }
}

bool SubblockAttachmentAccessor::HasChunkContainer() const
{
    return this->has_chunk_container_;
}

bool SubblockAttachmentAccessor::EnumerateChunksInChunkContainer(const std::function<bool(int index, const ChunkInfo& info)>& functor_enum)
{
    if (!this->has_chunk_container_)
    {
        throw LibCZIException("Subblock does not have a chunk container.");
    }

    // the minimal size is 21 bytes: Guid (16 bytes) + size (4 bytes) + 1 byte payload
    const std::uint8_t* attachment_data;
    size_t size_attachment_data;
    this->sub_block_->DangerousGetRawData(ISubBlock::MemBlkType::Attachment, attachment_data, size_attachment_data);
    if (attachment_data == nullptr || size_attachment_data < 21)
    {
        throw LibCZIException("Invalid attachment data in sub-block or no attachment present.");
    }

    std::uint32_t offset = 0;


    for (int index = 0;;)
    {
        ChunkInfo chunk_info;

        // Read the GUID (16 bytes)
        chunk_info.guid = *reinterpret_cast<const GUID*>(attachment_data);
        Utilities::ConvertGuidToHostByteOrder(&chunk_info.guid);
        attachment_data += 16;

        // Read the size (4 bytes)
        chunk_info.size = *reinterpret_cast<const std::uint32_t*>(attachment_data);
        Utilities::ConvertUint32ToHostByteOrder(&chunk_info.size);
        attachment_data += 4;

        // is sufficient data available?
        if (size_attachment_data < offset + 20 + chunk_info.size)
        {
            throw LibCZIException("Corrupted chunk data.");
        }

        chunk_info.offset = offset + 20;

        const bool b = functor_enum(index, chunk_info);
        if (b==false)
        {
            break;
        }

        offset += 20 + chunk_info.size;

        // if there are not at least 21 bytes left, we are done
        if (size_attachment_data < offset + 21)
        {
            break;
        }
    }

    return true;
}

// "CBE3EA67-5BFC-492B-A16A-ECE378031448"
