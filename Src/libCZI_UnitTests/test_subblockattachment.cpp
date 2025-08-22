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
        // ---- The ONE method you’re allowed to use in tests ----
        MOCK_METHOD(void, DangerousGetRawData,
                    (MemBlkType type, const void*& ptr, size_t& size),
                    (const, override));

        // ---- Everything else: report error if called ----
        const SubBlockInfo& GetSubBlockInfo() const override {
            ADD_FAILURE() << "GetSubBlockInfo() must not be called on MockSubBlockOnlyAttachment.";
            return dummy_info_;
        }

        std::shared_ptr<const void> GetRawData(MemBlkType type, size_t* ptrSize) const override {
            const void* p = nullptr;
            size_t sz = 0;
            this->DangerousGetRawData(type, p, sz);   // reuse your test’s setup
            if (ptrSize) *ptrSize = sz;
            if (!p || sz == 0) return {};

            auto owner = std::make_shared<std::vector<unsigned char>>(
                static_cast<const unsigned char*>(p),
                static_cast<const unsigned char*>(p) + sz);
            return std::shared_ptr<const void>(owner, owner->data()); // aliasing ptr
        }

        std::shared_ptr<IBitmapData> CreateBitmap(const CreateBitmapOptions* /*options*/) override {
            ADD_FAILURE() << "CreateBitmap() must not be called on MockSubBlockOnlyAttachment.";
            return {}; // nullptr
        }

        // ---- Optional helpers to make DangerousGetRawData easy to use ----
        void SetBuffer(MemBlkType t, std::vector<std::uint8_t> bytes) {
            BufferFor(t) = std::move(bytes);
        }

        // Sets a default action so DangerousGetRawData returns pointers into our buffers.
        // You can still override with EXPECT_CALL(...) in a specific test if needed.
        void SetDefaultDangerousRawBehavior() {
            using ::testing::_;
            using ::testing::A;
            using ::testing::Invoke;

            ON_CALL(*this, DangerousGetRawData(_, A<const void*&>(), A<size_t&>()))
                .WillByDefault(Invoke([this](MemBlkType t, const void*& outPtr, size_t& outSize) {
                auto& buf = BufferFor(t);
                outPtr = buf.empty() ? nullptr : static_cast<const void*>(buf.data());
                outSize = buf.size();
                }));
        }

    private:
        std::vector<unsigned char>& BufferFor(MemBlkType t) {
            switch (t) {
            case Metadata:   return metadata_;
            case Data:       return data_;
            case Attachment: return attachment_;
            }
            // Fallback (should never happen)
            return data_;
        }

        // backing storage for the helper
        std::vector<unsigned char> metadata_;
        std::vector<unsigned char> data_;
        std::vector<unsigned char> attachment_;
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
    mockSubBlock->SetDefaultDangerousRawBehavior();
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
}
