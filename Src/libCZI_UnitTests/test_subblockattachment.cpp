// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include <gmock/gmock.h>
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include <codecvt>
#include <locale>

using namespace libCZI;
using namespace std;

namespace
{
    class MockSubBlockOnlyAttachment : public ISubBlock
    {
    public:
        const SubBlockInfo& GetSubBlockInfo() const override
        {
            ADD_FAILURE() << "GetSubBlockInfo() must not be called on MockSubBlockOnlyAttachment.";
            return dummy_info_;
        }

        std::shared_ptr<const void> GetRawData(MemBlkType type, size_t* ptrSize) const override
        {
            auto buffer_and_size = this->BufferFor(type);
            if (ptrSize)
            {
                *ptrSize = std::get<1>(buffer_and_size);
            }

            return get<0>(buffer_and_size);
        }

        void DangerousGetRawData(MemBlkType type, const void*& ptr, size_t& size) const override
        {
            auto buffer_and_size = this->BufferFor(type);
            ptr = get<0>(buffer_and_size).get();
            size = get<1>(buffer_and_size);
        }

        std::shared_ptr<IBitmapData> CreateBitmap(const CreateBitmapOptions* /*options*/) override
        {
            ADD_FAILURE() << "CreateBitmap() must not be called on MockSubBlockOnlyAttachment.";
            return {}; // nullptr
        }

        void SetBuffer(MemBlkType t, const vector<std::uint8_t>& bytes)
        {
            auto buffer = shared_ptr<void>(malloc(bytes.size()), free);
            memcpy(const_cast<void*>(buffer.get()), bytes.data(), bytes.size());
            switch (t)
            {
            case Metadata:
                this->metadata_ = buffer;
                this->size_metadata_ = bytes.size();
                break;
            case Data:
                this->data_ = buffer;
                this->size_data_ = bytes.size();
                break;
            case Attachment:
                this->attachment_ = buffer;
                this->size_attachment_ = bytes.size();
                break;
            default:
                throw runtime_error("invalid memory block type");
            }
        }
    private:
        tuple<shared_ptr<const void>, size_t> BufferFor(MemBlkType t) const
        {
            switch (t)
            {
            case Metadata:   return make_tuple(this->metadata_, this->size_metadata_);
            case Data:       return make_tuple(this->data_, this->size_data_);
            case Attachment: return make_tuple(this->attachment_, this->size_attachment_);
            }

            throw runtime_error("invalid memory block type");
        }

        shared_ptr<const void> metadata_;
        size_t size_metadata_{ 0 };
        shared_ptr<const void> data_;
        size_t size_data_{ 0 };
        shared_ptr<const void> attachment_;
        size_t size_attachment_{ 0 };
        SubBlockInfo dummy_info_{}; // used only to satisfy reference return when erroring
    };
}

TEST(SubBlockAttachment, BasicTest)
{
    string xml_string =
        "<METADATA>\n"
        "  <AttachmentSchema>\n"
        "    <DataFormat>CHUNKCONTAINER</DataFormat>\n"
        "  </AttachmentSchema>\n"
        "</METADATA>";
    vector<uint8_t> metadata(xml_string.begin(), xml_string.end());

    auto mockSubBlock = make_shared<MockSubBlockOnlyAttachment>();
    mockSubBlock->SetBuffer(libCZI::ISubBlock::MemBlkType::Metadata, metadata);
    mockSubBlock->SetBuffer(libCZI::ISubBlock::MemBlkType::Attachment,
        {
            1, 2, 3, 4 , 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
            1 , 0 , 0 ,0,
            42
        });

    auto sub_block_metadata = CreateSubBlockMetadataFromSubBlock(mockSubBlock.get());
    ASSERT_NE(sub_block_metadata, nullptr);

    auto sub_block_metadata_accessor = CreateSubBlockAttachmentAccessor(mockSubBlock, sub_block_metadata);

    vector<ISubBlockAttachmentAccessor::ChunkInfo> chunks;
    sub_block_metadata_accessor->EnumerateChunksInChunkContainer(
        [&chunks](int index, const ISubBlockAttachmentAccessor::ChunkInfo& info)
        {
            chunks.push_back(info);
            return true; // continue enumeration
        });

    ASSERT_EQ(chunks.size(), 1);
    ASSERT_EQ(chunks[0].offset, 20);
    const GUID g{ 0x04030201, 0x0605, 0x0807, { 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10 } };
    ASSERT_EQ(chunks[0].guid, g);
}

TEST(SubBlockAttachment, InvalidChunkContainer1Test)
{
    string xml_string =
        "<METADATA>\n"
        "  <AttachmentSchema>\n"
        "    <DataFormat>CHUNKCONTAINER</DataFormat>\n"
        "  </AttachmentSchema>\n"
        "</METADATA>";
    vector<uint8_t> metadata(xml_string.begin(), xml_string.end());

    auto mockSubBlock = make_shared<MockSubBlockOnlyAttachment>();
    mockSubBlock->SetBuffer(libCZI::ISubBlock::MemBlkType::Metadata, metadata);
    mockSubBlock->SetBuffer(libCZI::ISubBlock::MemBlkType::Attachment,
        {
            // this is too short to be a valid chunk container
            1, 2, 3, 4 , 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
            1 , 0 , 0 ,0
        });

    auto sub_block_metadata = CreateSubBlockMetadataFromSubBlock(mockSubBlock.get());
    ASSERT_NE(sub_block_metadata, nullptr);

    auto sub_block_metadata_accessor = CreateSubBlockAttachmentAccessor(mockSubBlock, sub_block_metadata);

    vector<ISubBlockAttachmentAccessor::ChunkInfo> chunks;
    // Option 2: Use EXPECT_ANY_THROW if you don't care about specific exception type
    EXPECT_ANY_THROW(
        sub_block_metadata_accessor->EnumerateChunksInChunkContainer(
            [&chunks](int index, const ISubBlockAttachmentAccessor::ChunkInfo& info)
            {
                chunks.push_back(info);
                return true; // continue enumeration
            }),
        libCZI::LibCZIException
    );
}
