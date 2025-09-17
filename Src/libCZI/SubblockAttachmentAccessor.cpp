// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubblockAttachmentAccessor.h"
#include "libCZI.h"
#include "stdAllocator.h"
#include "utilities.h"
#include "bitmapData.h"

using namespace libCZI;
using namespace std;

static const libCZI::GUID kGuidValidPixelMask{ 0xCBE3EA67, 0x5BFC, 0x492B, { 0xA1, 0x6A, 0xEC, 0xE3, 0x78, 0x03, 0x14, 0x48 } };

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

std::shared_ptr<libCZI::ISubBlockMetadata> SubblockAttachmentAccessor::GetSubBlockMetadata()
{
    return this->sub_block_metadata_;
}

bool SubblockAttachmentAccessor::HasChunkContainer() const
{
    return this->has_chunk_container_;
}

bool SubblockAttachmentAccessor::EnumerateChunksInChunkContainer(const std::function<bool(int index, const ChunkInfo& info)>& functor_enum) const
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
    bool chunk_delivered = false;

    for (int index = 0;;)
    {
        ChunkInfo chunk_info;

        // Read the GUID (16 bytes)
        memcpy(&chunk_info.guid, attachment_data, 16);
        Utilities::ConvertGuidToHostByteOrder(&chunk_info.guid);
        attachment_data += 16;

        // Read the size (4 bytes)
        memcpy(&chunk_info.size, attachment_data, 4);
        Utilities::ConvertUint32ToHostByteOrder(&chunk_info.size);
        attachment_data += 4;

        // is sufficient data available?
        if (size_attachment_data < offset + 20 + chunk_info.size)
        {
            throw LibCZIException("Corrupted chunk data.");
        }

        chunk_info.offset = offset + 20;

        chunk_delivered = true;
        const bool b = functor_enum(index, chunk_info);
        if (b == false)
        {
            break;
        }

        offset += 20 + chunk_info.size;

        // if there are not at least 21 bytes left, we are done (note that we require at least 1 byte of payload!)
        if (size_attachment_data < offset + 21)
        {
            break;
        }
    }

    return chunk_delivered;
}

libCZI::SubBlockAttachmentMaskInfoGeneral SubblockAttachmentAccessor::GetValidPixelMaskFromChunkContainer() const
{
    if (!this->HasChunkContainer())
    {
        throw LibCZIException("Subblock does not have a chunk container.");
    }

    ChunkInfo chunk_info_valid_pixel_mask;
    this->EnumerateChunksInChunkContainer(
        [&chunk_info_valid_pixel_mask](int /*index*/, const ChunkInfo& info)
        {
            if (info.guid == kGuidValidPixelMask)
            {
                chunk_info_valid_pixel_mask = info;
                return false; // stop enumeration
            }

            return true; // continue enumeration
        });

    SubBlockAttachmentMaskInfoGeneral mask_info_general;
    shared_ptr<const void> spData = this->sub_block_->GetRawData(ISubBlock::MemBlkType::Attachment, nullptr);
    memcpy(&mask_info_general.width, static_cast<const uint8_t*>(spData.get()) + chunk_info_valid_pixel_mask.offset, sizeof(mask_info_general.width));
    Utilities::ConvertUint32ToHostByteOrder(&mask_info_general.width);
    memcpy(&mask_info_general.height, static_cast<const uint8_t*>(spData.get()) + chunk_info_valid_pixel_mask.offset + sizeof(uint32_t), sizeof(mask_info_general.height));
    Utilities::ConvertUint32ToHostByteOrder(&mask_info_general.height);
    memcpy(&mask_info_general.type_of_representation, static_cast<const uint8_t*>(spData.get()) + chunk_info_valid_pixel_mask.offset + 2 * sizeof(uint32_t), sizeof(mask_info_general.type_of_representation));
    Utilities::ConvertUint32ToHostByteOrder(&mask_info_general.type_of_representation);

    // create an aliasing shared_ptr that points to the correct offset
    mask_info_general.size_data = chunk_info_valid_pixel_mask.size - (3 * sizeof(uint32_t));
    if (mask_info_general.size_data > 0)
    {
        mask_info_general.data = shared_ptr<const void>(spData, static_cast<const uint8_t*>(spData.get()) + chunk_info_valid_pixel_mask.offset + 3 * sizeof(uint32_t));
    }

    return mask_info_general;
}

/*static*/SubBlockAttachmentMaskInfoUncompressedBitonalBitmap ISubBlockAttachmentAccessor::GetValidPixelMaskAsUncompressedBitonalBitmap(const ISubBlockAttachmentAccessor* accessor)
{
    const auto mask_info_general = accessor->GetValidPixelMaskFromChunkContainer();
    if (mask_info_general.type_of_representation != 0)
    {
        throw LibCZIException("Valid pixel mask is not an uncompressed bitonal bitmap.");
    }

    SubBlockAttachmentMaskInfoUncompressedBitonalBitmap mask_info_uncompressed_bitonal_bitmap;
    mask_info_uncompressed_bitonal_bitmap.width = mask_info_general.width;
    mask_info_uncompressed_bitonal_bitmap.height = mask_info_general.height;
    if (mask_info_general.size_data < 4)
    {
        throw LibCZIException("Invalid uncompressed bitonal bitmap pixel mask ");
    }

    mask_info_uncompressed_bitonal_bitmap.stride = *reinterpret_cast<const std::uint32_t*>(static_cast<const uint8_t*>(mask_info_general.data.get()));
    Utilities::ConvertUint32ToHostByteOrder(&mask_info_uncompressed_bitonal_bitmap.stride);
    mask_info_uncompressed_bitonal_bitmap.size_data = mask_info_general.size_data - sizeof(uint32_t);
    if (mask_info_uncompressed_bitonal_bitmap.size_data > 0)
    {
        mask_info_uncompressed_bitonal_bitmap.data = shared_ptr<const void>(mask_info_general.data, sizeof(uint32_t) + static_cast<const uint8_t*>(mask_info_general.data.get()));
    }

    return mask_info_uncompressed_bitonal_bitmap;
}

/*static*/std::shared_ptr<libCZI::IBitonalBitmapData> ISubBlockAttachmentAccessor::CreateBitonalBitmapFromMaskInfo(const ISubBlockAttachmentAccessor* accessor)
{
    SubBlockAttachmentMaskInfoUncompressedBitonalBitmap mask_info = accessor->GetValidPixelMaskAsUncompressedBitonalBitmap();

    if (mask_info.width == 0 || mask_info.height == 0)
    {
        throw LibCZIException("Invalid dimensions for uncompressed bitonal bitmap.");
    }

    const uint32_t minimal_stride = (mask_info.width + 7) / 8;
    if (mask_info.stride < minimal_stride)
    {
        throw LibCZIException("Invalid stride for uncompressed bitonal bitmap.");
    }

    size_t minimal_size = (mask_info.height - 1) * static_cast<size_t>(mask_info.stride) + minimal_stride;
    if (mask_info.size_data < minimal_size)
    {
        throw LibCZIException("Insufficient size of uncompressed bitonal bitmap pixel mask data.");
    }
    
    // Create a new bitonal bitmap data object (but using the existing data buffer!).
    CSharedPtrAllocator sharedPtrAllocator(mask_info.data); 
    auto bitonal_bitmap = CBitonalBitmapData<CSharedPtrAllocator>::Create(
                                                                    sharedPtrAllocator,
                                                                    mask_info.width,
                                                                    mask_info.height,
                                                                    mask_info.stride);
    return bitonal_bitmap;
}
