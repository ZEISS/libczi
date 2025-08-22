#pragma once

#include "SubblockAttachmentAccessor.h"
#include "libCZI.h"


class SubblockAttachmentAccessor : public libCZI::ISubBlockAttachmentAccessor
{
private:
    std::shared_ptr<libCZI::ISubBlock> sub_block_;
    std::shared_ptr<libCZI::ISubBlockMetadata> sub_block_metadata_;
    bool has_chunk_container_ = false;
public:
    SubblockAttachmentAccessor(std::shared_ptr<libCZI::ISubBlock> sub_block, std::shared_ptr<libCZI::ISubBlockMetadata> sub_block_metadata);

    bool HasChunkContainer() const override;

    bool EnumerateChunksInChunkContainer(const std::function<bool(int index, const ChunkInfo& info)>& functor_enum) override;
};

