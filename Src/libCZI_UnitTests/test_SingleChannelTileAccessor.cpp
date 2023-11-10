// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/SingleChannelTileAccessor.h"
#include "MemOutputStream.h"
#include "utils.h"

using namespace libCZI;
using namespace std;

class SubBlockRepositoryShim : public ISubBlockRepository
{
private:
    std::shared_ptr<libCZI::ISubBlockRepository> subblock_repository_;
    vector<int> subblocks_read_;
public:
    explicit SubBlockRepositoryShim(std::shared_ptr<libCZI::ISubBlockRepository> subblock_repository)
        : subblock_repository_(std::move(subblock_repository))
    {}

    /// Gets a vector containing the indices of the subblocks that were read
    /// (by calling the ReadSubBlock-method).
    ///
    /// \returns    The indices of the subblocks read.
    const vector<int>& GetSubblocksRead() const
    {
        return this->subblocks_read_;
    }

    void EnumerateSubBlocks(const std::function<bool(int index, const SubBlockInfo& info)>& funcEnum) override
    {
        this->subblock_repository_->EnumerateSubBlocks(funcEnum);
    }
    void EnumSubset(const IDimCoordinate* planeCoordinate, const IntRect* roi, bool onlyLayer0, const std::function<bool(int index, const SubBlockInfo& info)>& funcEnum) override
    {
        this->subblock_repository_->EnumSubset(planeCoordinate, roi, onlyLayer0, funcEnum);
    }
    std::shared_ptr<ISubBlock> ReadSubBlock(int index) override
    {
        this->subblocks_read_.push_back(index);
        return this->subblock_repository_->ReadSubBlock(index);
    }
    bool TryGetSubBlockInfoOfArbitrarySubBlockInChannel(int channelIndex, SubBlockInfo& info) override
    {
        return this->subblock_repository_->TryGetSubBlockInfoOfArbitrarySubBlockInChannel(channelIndex, info);
    }
    bool TryGetSubBlockInfo(int index, SubBlockInfo* info) const override
    {
        return this->subblock_repository_->TryGetSubBlockInfo(index, info);
    }
    SubBlockStatistics GetStatistics() override
    {
        return this->subblock_repository_->GetStatistics();
    }
    PyramidStatistics GetPyramidStatistics() override
    {
        return this->subblock_repository_->GetPyramidStatistics();
    }
};

static tuple<shared_ptr<void>, size_t> CreateTestCzi()
{
    const auto writer = CreateCZIWriter();
    const auto outStream = make_shared<CMemOutputStream>(0);

    const auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::T, 0, 1 }, { DimensionIndex::C, 0, 1 } },	// set a bounds for Z and C
        0, 5);	// set a bounds M : 0<=m<=5

    writer->Create(outStream, spWriterInfo);

    static constexpr uint8_t kBitmap1[4] = { 1, 1, 1, 1 };
    static constexpr uint8_t kBitmap2[4] = { 2, 2, 2, 2 };
    static constexpr uint8_t kBitmap3[4] = { 3, 3, 3, 3 };

    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.coordinate.Set(DimensionIndex::T, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = 2;
    addSbBlkInfo.logicalHeight = 2;
    addSbBlkInfo.physicalWidth = 2;
    addSbBlkInfo.physicalHeight = 2;
    addSbBlkInfo.PixelType = PixelType::Gray8;
    addSbBlkInfo.ptrBitmap = kBitmap1;
    addSbBlkInfo.strideBitmap = 2;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.x = 1;
    addSbBlkInfo.y = 1;
    addSbBlkInfo.mIndex = 1;
    addSbBlkInfo.ptrBitmap = kBitmap2;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.x = 2;
    addSbBlkInfo.y = 2;
    addSbBlkInfo.mIndex = 2;
    addSbBlkInfo.ptrBitmap = kBitmap3;
    writer->SyncAddSubBlock(addSbBlkInfo);

    PrepareMetadataInfo prepare_metadata_info;
    const auto metaDataBuilder = writer->GetPreparedMetadata(prepare_metadata_info);

    WriteMetadataInfo write_metadata_info;
    const auto& strMetadata = metaDataBuilder->GetXml();
    write_metadata_info.szMetadata = strMetadata.c_str();
    write_metadata_info.szMetadataSize = strMetadata.size() + 1;
    write_metadata_info.ptrAttachment = nullptr;
    write_metadata_info.attachmentSize = 0;
    writer->SyncWriteMetadata(write_metadata_info);

    writer->Close();

    return make_tuple(outStream->GetCopy(nullptr), outStream->GetDataSize());
}

TEST(SingleChannelTileAccessor, VisibilityCheck)
{
    auto czi_document_as_blob = CreateTestCzi();

    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
    const auto accessor = make_shared<CSingleChannelTileAccessor>(subblock_repository_with_read_history);
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

    const auto tile_composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 1, 1, 1, 1 }, &plane_coordinate, nullptr);
    EXPECT_EQ(tile_composite_bitmap->GetWidth(), 1);
    EXPECT_EQ(tile_composite_bitmap->GetHeight(), 1);
    const ScopedBitmapLockerSP locked_tile_composite_bitmap{ tile_composite_bitmap };
    EXPECT_EQ(*(static_cast<const std::uint8_t*>(locked_tile_composite_bitmap.ptrDataRoi)), 2);

    // check that subblock #0 and #2 have NOT been read
    EXPECT_TRUE(
        find(subblock_repository_with_read_history->GetSubblocksRead().cbegin(),
        subblock_repository_with_read_history->GetSubblocksRead().cend(),
        0) == subblock_repository_with_read_history->GetSubblocksRead().cend()) << "subblock #0 is not expected to be read";
    EXPECT_TRUE(
        find(subblock_repository_with_read_history->GetSubblocksRead().cbegin(),
        subblock_repository_with_read_history->GetSubblocksRead().cend(),
        2) == subblock_repository_with_read_history->GetSubblocksRead().cend()) << "subblock #2 is not expected to be read";
}
