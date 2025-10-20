// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include "libCZI.h"

namespace libCZI
{
    namespace detail
    {

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

            static libCZI::SubBlockAttachmentMaskInfoUncompressedBitonalBitmap  GetValidPixelMaskAsUncompressedBitonalBitmap(const ISubBlockAttachmentAccessor* accessor);
        };

    }   // namespace detail
}   // namespace libCZI
