#pragma once

#include <memory>
#include "libCZI.h"

//struct SubBlockAttachmentMaskInfoGeneral
//{
//    std::uint32_t width;
//    std::uint32_t height;
//    std::int8_t type_of_representation;
//    size_t size_data;
//    std::shared_ptr<const void> data;
//};
//
//struct SubBlockAttachmentMaskInfoUncompressedBitonalBitmap
//{
//    std::uint32_t width;
//    std::uint32_t height;
//    std::uint32_t stride;
//    size_t size_data;
//    std::shared_ptr<const void> data;
//};

class SubblockAttachmentAccessor : public libCZI::ISubBlockAttachmentAccessor
{
private:
    std::shared_ptr<libCZI::ISubBlock> sub_block_;
    std::shared_ptr<libCZI::ISubBlockMetadata> sub_block_metadata_;
    bool has_chunk_container_ = false;
public:
    SubblockAttachmentAccessor(std::shared_ptr<libCZI::ISubBlock> sub_block, std::shared_ptr<libCZI::ISubBlockMetadata> sub_block_metadata);

    std::shared_ptr<libCZI::ISubBlockMetadata> GetSubBlockMetadata() override;

    bool HasChunkContainer() const override;

    bool EnumerateChunksInChunkContainer(const std::function<bool(int index, const ChunkInfo& info)>& functor_enum) const override;

    libCZI::SubBlockAttachmentMaskInfoGeneral GetValidPixelMaskFromChunkContainer() const override;
    //libCZI::SubBlockAttachmentMaskInfoUncompressedBitonalBitmap GetValidPixelMaskAsUncompressedBitonalBitmap() const override;

    static libCZI::SubBlockAttachmentMaskInfoUncompressedBitonalBitmap  GetValidPixelMaskAsUncompressedBitonalBitmap(const ISubBlockAttachmentAccessor* accessor);
};

