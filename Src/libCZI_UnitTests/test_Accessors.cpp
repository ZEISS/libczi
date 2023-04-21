// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include <array>
#include <tuple>
#include <memory>
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include "MemInputOutputStream.h"

using namespace libCZI;
using namespace std;

/// Creates a synthetic CZI document and returns it as a blob. This is used by unit-tests below.
///
/// \param  m_indices_for_subblocks The m-indices for the three subblocks which are created and added to the document.
///
/// \returns A blob containing the synthetic CZI document.
static tuple<shared_ptr<void>, size_t> CreateTestCziDocumentAndGetAsBlob(const array<int, 3>& m_indices_for_subblocks)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::C,0,1 } },	// set a bounds C
        0, 2);	// set a bounds M : 0<=m<=2
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateGray8BitmapAndFill(2, 1, 42);
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = m_indices_for_subblocks[0];
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    {
        ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
        addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    bitmap = CreateGray8BitmapAndFill(2, 1, 45);
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = m_indices_for_subblocks[1];
    addSbBlkInfo.x = 1;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    {
        ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
        addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    bitmap = CreateGray8BitmapAndFill(2, 1, 47);
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = m_indices_for_subblocks[2];
    addSbBlkInfo.x = 2;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    {
        ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
        addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    PrepareMetadataInfo prepare_metadata_info;
    auto metaDataBuilder = writer->GetPreparedMetadata(prepare_metadata_info);
    WriteMetadataInfo write_metadata_info;
    write_metadata_info.Clear();
    const auto& strMetadata = metaDataBuilder->GetXml();
    write_metadata_info.szMetadata = strMetadata.c_str();
    write_metadata_info.szMetadataSize = strMetadata.size() + 1;
    write_metadata_info.ptrAttachment = nullptr;
    write_metadata_info.attachmentSize = 0;
    writer->SyncWriteMetadata(write_metadata_info);
    writer->Close();
    writer.reset();

    size_t czi_document_size = 0;
    shared_ptr<void> czi_document_data = outStream->GetCopy(&czi_document_size);
    return make_tuple(czi_document_data, czi_document_size);
}

struct ZOrderAndResultGray8Fixture : public testing::TestWithParam<tuple<int, int, int, array<uint8_t, 4>>> { };

TEST_P(ZOrderAndResultGray8Fixture, CreateDocumentAndUseSingleChannelScalingTileAccessorWithSortByMAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a single-channel scaling tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, the M-index is to give the z-order - so depending on the M-index, we expect a different result,
    // which is then checked.

    const auto parameters = GetParam();

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ get<0>(parameters), get<1>(parameters), get<2>(parameters) });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, 1.f, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected = get<3>(parameters);
    EXPECT_EQ(p[0], expected[0]);
    EXPECT_EQ(p[1], expected[1]);
    EXPECT_EQ(p[2], expected[2]);
    EXPECT_EQ(p[3], expected[3]);
}

TEST_P(ZOrderAndResultGray8Fixture, CreateDocumentAndUseSingleChannelTileAccessorWithSortByMAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a single-channel tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, the M-index is to give the z-order - so depending on the M-index, we expect a different result,
    // which is then checked.

    const auto parameters = GetParam();

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ get<0>(parameters), get<1>(parameters), get<2>(parameters) });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelTileAccessor::Options options;
    options.Clear();

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected = get<3>(parameters);
    EXPECT_EQ(p[0], expected[0]);
    EXPECT_EQ(p[1], expected[1]);
    EXPECT_EQ(p[2], expected[2]);
    EXPECT_EQ(p[3], expected[3]);
}

TEST_P(ZOrderAndResultGray8Fixture, CreateDocumentAndUseSingleChannelPyramidLayerTileAccessorWithSortByMAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a pyramid-layer tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, the M-index is to give the z-order - so depending on the M-index, we expect a different result,
    // which is then checked.

    const auto parameters = GetParam();

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ get<0>(parameters), get<1>(parameters), get<2>(parameters) });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();
    CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2,0 }, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected = get<3>(parameters);
    EXPECT_EQ(p[0], expected[0]);
    EXPECT_EQ(p[1], expected[1]);
    EXPECT_EQ(p[2], expected[2]);
    EXPECT_EQ(p[3], expected[3]);
}

INSTANTIATE_TEST_SUITE_P(
    Accessor,
    ZOrderAndResultGray8Fixture,
    testing::Values(
    // third tile on top, second tile in the middle, first tile at the bottom
    make_tuple(0, 1, 2, array<uint8_t, 4>{ 42, 45, 47, 47 }),
    // first tile on top, second tile in the middle, third tile at the bottom
    make_tuple(2, 1, 0, array<uint8_t, 4>{ 42, 42, 45, 47 }),
    // second tile on top, third tile in the middle, first tile at the bottom
    make_tuple(0, 2, 1, array<uint8_t, 4>{ 42, 45, 45, 47 }),
    // first tile on top, third tile in the middle, first tile at the bottom
    make_tuple(2, 0, 1, array<uint8_t, 4>{ 42, 42, 47, 47 }),
    // third tile on top, first tile in the middle, second tile at the bottom
    make_tuple(1, 0, 2, array<uint8_t, 4>{ 42, 42, 47, 47 }),
    // second tile on top, first tile in the middle, third tile at the bottom
    make_tuple(1, 2, 0, array<uint8_t, 4>{ 42, 45, 45, 47 })));

TEST(Accessor, CreateDocumentAndUseSingleChannelScalingTileAccessorAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a single-channel scaling tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, we instruct NOT to sort by M-index, so the order is undefined and we therefore check that the
    // result is one of four possible results.

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ 0, 1, 2 });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.sortByM = false;

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, 1.f, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected_variant1 = array<uint8_t, 4>{ 42, 45, 47, 47};
    const auto& expected_variant2 = array<uint8_t, 4>{ 42, 42, 45, 47};
    const auto& expected_variant3 = array<uint8_t, 4>{ 42, 42, 47, 47};
    const auto& expected_variant4 = array<uint8_t, 4>{ 42, 45, 45, 47};
    EXPECT_TRUE(memcmp(p, &expected_variant1, sizeof(expected_variant1)) == 0 ||
                memcmp(p, &expected_variant2, sizeof(expected_variant2)) == 0 ||
                memcmp(p, &expected_variant3, sizeof(expected_variant3)) == 0 ||
                memcmp(p, &expected_variant4, sizeof(expected_variant4)) == 0);
}

TEST(Accessor, CreateDocumentAndUseSingleChannelTileAccessorAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a single-channel tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, we instruct NOT to sort by M-index, so the order is undefined and we therefore check that the
    // result is one of four possible results.

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ 0, 1, 2 });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.sortByM = false;

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected_variant1 = array<uint8_t, 4>{ 42, 45, 47, 47};
    const auto& expected_variant2 = array<uint8_t, 4>{ 42, 42, 45, 47};
    const auto& expected_variant3 = array<uint8_t, 4>{ 42, 42, 47, 47};
    const auto& expected_variant4 = array<uint8_t, 4>{ 42, 45, 45, 47};
    EXPECT_TRUE(memcmp(p, &expected_variant1, sizeof(expected_variant1)) == 0 ||
                memcmp(p, &expected_variant2, sizeof(expected_variant2)) == 0 ||
                memcmp(p, &expected_variant3, sizeof(expected_variant3)) == 0 ||
                memcmp(p, &expected_variant4, sizeof(expected_variant4)) == 0);
}

TEST(Accessor, CreateDocumentAndUseSingleChannelPyramidLayerTileAccessorWithAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a pyramid-layer tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, we instruct NOT to sort by M-index, so the order is undefined and we therefore check that the
    // result is one of four possible results.

    // arrange
    auto czi_document_as_blob = CreateTestCziDocumentAndGetAsBlob(array<int, 3>{ 0, 1, 2 });
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    const auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();
    CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();
    options.sortByM = false;

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2,0 }, &options);

    // assert
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi);
    const auto& expected_variant1 = array<uint8_t, 4>{ 42, 45, 47, 47};
    const auto& expected_variant2 = array<uint8_t, 4>{ 42, 42, 45, 47};
    const auto& expected_variant3 = array<uint8_t, 4>{ 42, 42, 47, 47};
    const auto& expected_variant4 = array<uint8_t, 4>{ 42, 45, 45, 47};
    EXPECT_TRUE(memcmp(p, &expected_variant1, sizeof(expected_variant1)) == 0 ||
                memcmp(p, &expected_variant2, sizeof(expected_variant2)) == 0 ||
                memcmp(p, &expected_variant3, sizeof(expected_variant3)) == 0 ||
                memcmp(p, &expected_variant4, sizeof(expected_variant4)) == 0);
}
