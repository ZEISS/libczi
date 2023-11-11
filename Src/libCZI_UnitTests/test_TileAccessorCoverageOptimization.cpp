// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later


#include "include_gtest.h"
#include <random>
#include "inc_libCZI.h"
#include "../libCZI/SingleChannelTileAccessor.h"
#include "../libCZI/SingleChannelScalingTileAccessor.h"
#include "MemOutputStream.h"
#include "utils.h"

using namespace libCZI;
using namespace std;

/// This is a shim for the ISubBlockRepository interface, which keeps track of the subblocks that were read.
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

    /// Clears the subblocks-read history.
    void ClearSubblockReadHistory()
    {
        this->subblocks_read_.clear();
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

struct SubBlockPositions
{
    IntRect rectangle;
    int mIndex;
};

static tuple<shared_ptr<void>, size_t> CreateTestCzi(const vector<SubBlockPositions>& subblocks)
{
    const auto writer = CreateCZIWriter();
    const auto outStream = make_shared<CMemOutputStream>(0);

    const auto spWriterInfo = make_shared<CCziWriterInfo>(
        GUID{ 0,0,0,{ 0,0,0,0,0,0,0,0 } },
        CDimBounds{ { DimensionIndex::T, 0, 1 }, { DimensionIndex::C, 0, 1 } },	// set a bounds for Z and C
        0, static_cast<int>(subblocks.size() - 1));

    writer->Create(outStream, spWriterInfo);

    int count = 0;
    for (const auto& block : subblocks)
    {
        ++count;
        const size_t size_of_bitmap = static_cast<size_t>(block.rectangle.w) * block.rectangle.h;
        unique_ptr<uint8_t[]> bitmap(new uint8_t[size_of_bitmap]);
        memset(bitmap.get(), count, size_of_bitmap);
        AddSubBlockInfoStridedBitmap addSbBlkInfo;
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
        addSbBlkInfo.coordinate.Set(DimensionIndex::T, 0);
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = block.mIndex;
        addSbBlkInfo.x = block.rectangle.x;
        addSbBlkInfo.y = block.rectangle.y;
        addSbBlkInfo.logicalWidth = block.rectangle.w;
        addSbBlkInfo.logicalHeight = block.rectangle.h;
        addSbBlkInfo.physicalWidth = block.rectangle.w;
        addSbBlkInfo.physicalHeight = block.rectangle.h;
        addSbBlkInfo.PixelType = PixelType::Gray8;
        addSbBlkInfo.ptrBitmap = bitmap.get();
        addSbBlkInfo.strideBitmap = block.rectangle.w;
        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    const auto metaDataBuilder = writer->GetPreparedMetadata(PrepareMetadataInfo{});

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

template<class tAccessorHandler>
void ThreeOverlappingSubBlockWithVisibilityOptimizationTest(tAccessorHandler handler)
{
    // We create a CZI with 3 subblocks, each containing a 2x2 bitmap.
    // 1st subblock is at (0,0), 2nd subblock is at (1,1), 3rd subblock is at (2,2).
    // We then query for the ROI (1,1,1,1) and check that only the 2nd subblock is read -
    // because subblock #0 is not visible (overdrawn by #1), and #2 does not intersect.

    // arrange
    //auto czi_document_as_blob = CreateTestCzi(0, 0, 1, 1, 2, 2);
    auto czi_document_as_blob = CreateTestCzi(vector<SubBlockPositions>{{ {0, 0, 2, 2}, 0 }, { {1, 1, 2, 2}, 1 }, { {2, 2, 2, 2}, 2 }});
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
    handler.Initialize(subblock_repository_with_read_history);
    //const auto accessor = make_shared<CSingleChannelTileAccessor>(subblock_repository_with_read_history);
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.useVisibilityCheckOptimization = true;
    //const auto tile_composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 1, 1, 1, 1 }, &plane_coordinate, &options);
    const auto tile_composite_bitmap = handler.GetBitmap(PixelType::Gray8, IntRect{ 1, 1, 1, 1 }, &plane_coordinate);

    // assert
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


TEST(TileAccessorCoverageOptimization, ThreeOverlappingSubBlockWithVisibilityOptimizationTest_SingleChannelTileAccessor)
{
    class SingleChannelTileAccessorHandler
    {
        shared_ptr<CSingleChannelTileAccessor> accessor_;
    public:
        void Initialize(shared_ptr<libCZI::ISubBlockRepository> repository)
        {
            this->accessor_ = make_shared<CSingleChannelTileAccessor>(repository);
        }

        shared_ptr<IBitmapData> GetBitmap(PixelType pixeltype, const IntRect& roi, const IDimCoordinate* planeCoordinate) const
        {
            ISingleChannelTileAccessor::Options options;
            options.Clear();
            options.useVisibilityCheckOptimization = true;
            return this->accessor_->Get(pixeltype, roi, planeCoordinate, &options);
        }
    };

    ThreeOverlappingSubBlockWithVisibilityOptimizationTest(SingleChannelTileAccessorHandler{});
}

TEST(TileAccessorCoverageOptimization, ThreeOverlappingSubBlockWithVisibilityOptimizationTest_SingleChannelScalingTileAccessor)
{
    class SingleChannelScalingTileAccessorHandler
    {
        shared_ptr<CSingleChannelScalingTileAccessor> accessor_;
    public:
        void Initialize(shared_ptr<libCZI::ISubBlockRepository> repository)
        {
            this->accessor_ = make_shared<CSingleChannelScalingTileAccessor>(repository);
        }

        shared_ptr<IBitmapData> GetBitmap(PixelType pixeltype, const IntRect& roi, const IDimCoordinate* planeCoordinate) const
        {
            ISingleChannelScalingTileAccessor::Options options;
            options.Clear();
            options.useVisibilityCheckOptimization = true;
            return this->accessor_->Get(pixeltype, roi, planeCoordinate, 1.f, &options);
        }
    };

    ThreeOverlappingSubBlockWithVisibilityOptimizationTest(SingleChannelScalingTileAccessorHandler{});
}

/*
TEST(TileAccessorCoverageOptimization, VisibilityCheck1)
{
    // We create a CZI with 3 subblocks, each containing a 2x2 bitmap.
    // 1st subblock is at (0,0), 2nd subblock is at (1,1), 3rd subblock is at (2,2).
    // We then query for the ROI (1,1,1,1) and check that only the 2nd subblock is read -
    // because subblock #0 is not visible (overdrawn by #1), and #2 does not intersect.

    // arrange
    //auto czi_document_as_blob = CreateTestCzi(0, 0, 1, 1, 2, 2);
    auto czi_document_as_blob = CreateTestCzi(vector<SubBlockPositions>{{ {0, 0, 2, 2}, 0 }, { {1, 1, 2, 2}, 1 }, { {2, 2, 2, 2}, 2 }});
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
    const auto accessor = make_shared<CSingleChannelTileAccessor>(subblock_repository_with_read_history);
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.useVisibilityCheckOptimization = true;
    const auto tile_composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 1, 1, 1, 1 }, &plane_coordinate, &options);

    // assert
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
*/

TEST(TileAccessorCoverageOptimization, VisibilityCheck2)
{
    // Now the three subblocks are all positioned at (0,0). We query for the ROI (1,1,1,1) and check that
    // only the top-most subblock (Which is #2) is read, because the other two are not visible (are overdrawn).

    // arrange
    auto czi_document_as_blob = CreateTestCzi(vector<SubBlockPositions>{{ {0, 0, 2, 2}, 0 }, { {0, 0, 2, 2}, 1 }, { {0, 0, 2, 2}, 2 }});
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
    const auto accessor = make_shared<CSingleChannelTileAccessor>(subblock_repository_with_read_history);
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.useVisibilityCheckOptimization = true;
    const auto tile_composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 1, 1, 1, 1 }, &plane_coordinate, &options);

    // assert
    EXPECT_EQ(tile_composite_bitmap->GetWidth(), 1);
    EXPECT_EQ(tile_composite_bitmap->GetHeight(), 1);
    const ScopedBitmapLockerSP locked_tile_composite_bitmap{ tile_composite_bitmap };
    EXPECT_EQ(*(static_cast<const std::uint8_t*>(locked_tile_composite_bitmap.ptrDataRoi)), 3);

    // check that subblock #0 and #1 have NOT been read
    EXPECT_TRUE(
        find(subblock_repository_with_read_history->GetSubblocksRead().cbegin(),
        subblock_repository_with_read_history->GetSubblocksRead().cend(),
        0) == subblock_repository_with_read_history->GetSubblocksRead().cend()) << "subblock #0 is not expected to be read";
    EXPECT_TRUE(
        find(subblock_repository_with_read_history->GetSubblocksRead().cbegin(),
        subblock_repository_with_read_history->GetSubblocksRead().cend(),
        1) == subblock_repository_with_read_history->GetSubblocksRead().cend()) << "subblock #1 is not expected to be read";
}

TEST(TileAccessorCoverageOptimization, RandomSubblocksCompareRenderingWithAndWithoutVisibilityOptimizationFor)
{
    // Here we place a random number of subblocks at random positions, and then check that the
    // rendering result w/ and w/o visibility-optimization is the same

    random_device dev;
    mt19937 rng(dev());
    uniform_int_distribution<int> distribution(0, 99); // distribution in range [0, 99]

    static constexpr  IntRect kRoi{ 0, 0, 120, 120 };

    for (int repeat = 0; repeat < 10; repeat++) // let's repeat this 10 times
    {
        const int number_of_rectangles = distribution(rng) + 1;

        vector<SubBlockPositions> subblocks;
        subblocks.reserve(number_of_rectangles);
        for (int i = 0; i < number_of_rectangles; ++i)
        {
            subblocks.emplace_back(SubBlockPositions{ IntRect{ distribution(rng), distribution(rng), 1 + distribution(rng), 1 + distribution(rng) }, i });
        }

        // Shuffle the vector into a random order
        std::shuffle(subblocks.begin(), subblocks.end(), rng);

        auto czi_document_as_blob = CreateTestCzi(subblocks);

        const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
        const auto reader = CreateCZIReader();
        reader->Open(memory_stream);

        // We construct a subblock-repository shim here which keeps track of the subblocks that were read - which
        //  is not really necessary here, but we do it anyway to make sure that the visibility-optimization
        //  is actually reducing the number of subblocks read.
        auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
        const auto accessor = make_shared<CSingleChannelTileAccessor>(subblock_repository_with_read_history);
        const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

        ISingleChannelTileAccessor::Options options;
        options.Clear();
        options.backGroundColor = RgbFloatColor{ 0,0,0 };
        options.useVisibilityCheckOptimization = true;
        const auto tile_composite_bitmap_with_visibility_optimization = accessor->Get(PixelType::Gray8, kRoi, &plane_coordinate, &options);
        const auto number_of_subblocks_read_with_visibility_optimization = subblock_repository_with_read_history->GetSubblocksRead().size();

        options.useVisibilityCheckOptimization = false;
        subblock_repository_with_read_history->ClearSubblockReadHistory();
        const auto tile_composite_bitmap_without_visibility_optimization = accessor->Get(PixelType::Gray8, kRoi, &plane_coordinate, &options);
        const auto number_of_subblocks_read_without_visibility_optimization = subblock_repository_with_read_history->GetSubblocksRead().size();

        EXPECT_TRUE(AreBitmapDataEqual(tile_composite_bitmap_with_visibility_optimization, tile_composite_bitmap_without_visibility_optimization)) <<
            "tile-composites w/ and w/o visibility-optimization are found to differ";

        EXPECT_LE(number_of_subblocks_read_with_visibility_optimization, number_of_subblocks_read_without_visibility_optimization) <<
            "the number of subblocks actually read w/ visibility-optimization must be less or equal to the number w/o this optimization";
    }
}

TEST(TileAccessorCoverageOptimization, Scaling_RandomSubblock_CompareRenderingWithAndWithoutVisibilityOptimization)
{
    // Here we place a random number of subblocks at random positions, and then check that the
    // rendering result w/ and w/o visibility-optimization is the same

    random_device dev;
    mt19937 rng(dev());
    uniform_int_distribution<int> distribution(0, 99); // distribution in range [0, 99]

    static constexpr  IntRect kRoi{ 0, 0, 120, 120 };

    for (int repeat = 0; repeat < 1000; repeat++) // let's repeat this 10 times
    {
        const int number_of_rectangles = distribution(rng) + 1;

        vector<SubBlockPositions> subblocks;
        subblocks.reserve(number_of_rectangles);
        for (int i = 0; i < number_of_rectangles; ++i)
        {
            subblocks.emplace_back(SubBlockPositions{ IntRect{ distribution(rng), distribution(rng), 1 + distribution(rng), 1 + distribution(rng) }, i });
        }

        // Shuffle the vector into a random order
        std::shuffle(subblocks.begin(), subblocks.end(), rng);

        auto czi_document_as_blob = CreateTestCzi(subblocks);

        const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
        const auto reader = CreateCZIReader();
        reader->Open(memory_stream);

        // We construct a subblock-repository shim here which keeps track of the subblocks that were read - which
        //  is not really necessary here, but we do it anyway to make sure that the visibility-optimization
        //  is actually reducing the number of subblocks read.
        auto subblock_repository_with_read_history = make_shared<SubBlockRepositoryShim>(reader);
        const auto accessor = make_shared<CSingleChannelScalingTileAccessor>(subblock_repository_with_read_history);
        const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0}, {DimensionIndex::T, 0} };

        ISingleChannelScalingTileAccessor::Options options;
        options.Clear();
        options.backGroundColor = RgbFloatColor{ 0,0,0 };
        options.useVisibilityCheckOptimization = true;
        const auto tile_composite_bitmap_with_visibility_optimization = accessor->Get(PixelType::Gray8, kRoi, &plane_coordinate, 1, &options);
        const auto number_of_subblocks_read_with_visibility_optimization = subblock_repository_with_read_history->GetSubblocksRead().size();

        options.useVisibilityCheckOptimization = false;
        subblock_repository_with_read_history->ClearSubblockReadHistory();
        const auto tile_composite_bitmap_without_visibility_optimization = accessor->Get(PixelType::Gray8, kRoi, &plane_coordinate, 1, &options);
        const auto number_of_subblocks_read_without_visibility_optimization = subblock_repository_with_read_history->GetSubblocksRead().size();

        EXPECT_TRUE(AreBitmapDataEqual(tile_composite_bitmap_with_visibility_optimization, tile_composite_bitmap_without_visibility_optimization)) <<
            "tile-composites w/ and w/o visibility-optimization are found to differ";

        EXPECT_LE(number_of_subblocks_read_with_visibility_optimization, number_of_subblocks_read_without_visibility_optimization) <<
            "the number of subblocks actually read w/ visibility-optimization must be less or equal to the number w/o this optimization";
    }
}
