// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
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
        CDimBounds{ { DimensionIndex::C, 0, 1 } },	// set a bounds C
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

/// Creates a "special" CZI which was found problematic wrt pixel accuracy - it contains a single subblock
/// at position (0,2671) and size (761,2449). The subblock is of pixel type gray8, and it contains
/// the value 0x2a for all pixels.
///
/// \returns A blob containing a CZI document.
static tuple<shared_ptr<void>, size_t> CreateCziWhichWasFoundProblematicWrtPixelAccuracyAndGetAsBlob()
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::C, 0, 1 } },	// set a bounds C
        0, 0);	// set a bounds M : 0<=m<=0
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateGray8BitmapAndFill(761, 2449, 0x2a);
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 2671;
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

/// Creates a CZI with four subblock of size 2x2 of pixeltype "Gray8" in a mosaic arrangement. The arrangement is as follows:
/// +--+--+
/// |0 |1 |
/// |  |  |
/// +--+--+
/// |2 |3 |
/// |  |  |
/// +--+--+
/// Subblock 0 contains the value 0x1, subblock 1 contains the value 0x2, subblock 2 contains the value 0x3 and subblock 3 contains the value 0x4.
/// \returns    A blob containing a CZI document.
static tuple<shared_ptr<void>, size_t> CreateCziWithFourSubblockInMosaicArragengement()
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::C, 0, 1 } },	// set a bounds for C
        0, 3);	// set a bounds M : 0<=m<=0
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateGray8BitmapAndFill(2, 2, 0x1);
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
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

    bitmap = CreateGray8BitmapAndFill(2, 2, 0x2);
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 1;
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

    bitmap = CreateGray8BitmapAndFill(2, 2, 0x3);
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 2;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 2;
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

    bitmap = CreateGray8BitmapAndFill(2, 2, 0x4);
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 3;
    addSbBlkInfo.x = 2;
    addSbBlkInfo.y = 2;
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
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2, 0 }, & options);

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
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();
    options.sortByM = false;

    // act
    const auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2, 0 }, & options);

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

TEST(Accessor, CreateDocumentAndEnsurePixelAccuracyWithScalingAccessor)
{
    // arrange

    // we now create a document with characteristics which have been "problematic" - in this case the composition
    //  result was not pixel-accurate (despite the zoom being exactly 1)
    auto czi_document_as_blob = CreateCziWhichWasFoundProblematicWrtPixelAccuracyAndGetAsBlob();

    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);

    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0,0,0 };   // request to have background cleared with black

    // act
    const auto composite_bitmap = accessor->Get(
        PixelType::Gray8,
        IntRect{ 0,0,5121,5121 },
        &plane_coordinate,
        1,
        &options);

    // assert

    // ok, we now expect that composite-bitmap is all black, except for a rectangle of size 761x2449 at (0,2671) which has the pixel-value 0x2a
    ASSERT_EQ(composite_bitmap->GetWidth(), 5121);
    ASSERT_EQ(composite_bitmap->GetHeight(), 5121);
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    for (size_t y = 0; y < 5121; ++y)
    {
        for (size_t x = 0; x < 5121; ++x)
        {
            uint8_t expected_value = (x < 761 && y >= 2671 && y < 2671 + 2449) ? 0x2a : 0;
            const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + y * lock_info_bitmap.stride + x;
            if (*p != expected_value)
            {
                FAIL() << "resulting bitmap is incorrect (at x=" << x << " y=" << y << ").";
            }
        }
    }

    SUCCEED();
}

TEST(Accessor, CreateDocumentAndExerciseScalingAccessorAllowingForInaccuracy)
{
    // in this test, we use the same CZI-document as before, but we use a zoom not exactly equal to 1.0, 
    // and when checking the result, we allow for some inaccuracy (due to the zoom not being exactly 1.0)

    // arrange

    // we now create a document with characteristics which have been "problematic" - in this case the composition
    //  result was not pixel-accurate (despite the zoom being exactly 1)
    auto czi_document_as_blob = CreateCziWhichWasFoundProblematicWrtPixelAccuracyAndGetAsBlob();

    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);

    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0,0,0 };   // request to have background cleared with black

    // act
    constexpr float zoom = 1 - numeric_limits<float>::epsilon();    // use a zoom a tiny bit less than 1
    const IntSize resulting_size = accessor->CalcSize(IntRect{ 0,0,5121,5121 }, zoom);
    const auto composite_bitmap = accessor->Get(
        PixelType::Gray8,
        IntRect{ 0,0,5121,5121 },
        &plane_coordinate,
        zoom,
        &options);

    // assert

    EXPECT_EQ(composite_bitmap->GetWidth(), resulting_size.w);
    EXPECT_EQ(composite_bitmap->GetHeight(), resulting_size.h);
    // ok, we now expect that composite-bitmap is all black, except for a rectangle of size 761x2449 at (0,2671) which has the pixel-value 0x2a
    ASSERT_TRUE(composite_bitmap->GetWidth() == 5121 || composite_bitmap->GetWidth() == 5120);
    ASSERT_TRUE(composite_bitmap->GetHeight() == 5121 || composite_bitmap->GetHeight() == 5120);
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    for (size_t y = 0; y < composite_bitmap->GetHeight(); ++y)
    {
        for (size_t x = 0; x < composite_bitmap->GetWidth(); ++x)
        {
            const uint8_t expected_value = (x < 761 && y >= 2671 && y < 2671 + 2449) ? 0x2a : 0;

            // allow both values for the exact borders of the subblock, i.e. allow for the bitmap to be one pixel smaller on the edges
            const bool inaccuracy_allowed = ((y == 2670 || y == 2671 || y == 2670 + 2449 || y == 2671 + 2449) && (x < 760));
            const uint8_t* p = static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + y * lock_info_bitmap.stride + x;
            if (*p != expected_value)
            {
                if (!inaccuracy_allowed || *p != 0)
                {
                    FAIL() << "resulting bitmap is incorrect (at x=" << x << " y=" << y << ").";
                }
            }
        }
    }

    SUCCEED();
}

TEST(Accessor, CreateDocumentAndCheckSingleChannelScalingAccessor1)
{
    // arrange

    // We create a CZI-document with four subblocks, each of size 2x2, and arranged as a mosaic like this:
    //
    // +--+--+
    // |0 |1 |
    // |  |  |
    // +--+--+
    // |2 |3 |
    // |  |  |
    // +--+--+
    //
    // We then request a tile-composite bitmap of size 2x2 for the ROI (1,1,2,2), and expect to find the
    // following pixel values in the resulting bitmap:
    // +-+-+
    // |1|2|
    // |3|4|
    // +-+-+
    auto czi_document_as_blob = CreateCziWithFourSubblockInMosaicArragengement();

    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);

    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0,0,0 };   // request to have background cleared with black

    // act
    const auto composite_bitmap = accessor->Get(
        PixelType::Gray8,
        IntRect{ 1,1,2,2 },
        &plane_coordinate,
        1,
        &options);

    // assert

    // ok, we now expect that composite-bitmap is all black, except for a rectangle of size 761x2449 at (0,2671) which has the pixel-value 0x2a
    ASSERT_EQ(composite_bitmap->GetWidth(), 2);
    ASSERT_EQ(composite_bitmap->GetHeight(), 2);
    const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
    const uint8_t pixel_x0_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 0);
    const uint8_t pixel_x1_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 1);
    const uint8_t pixel_x0_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 0);
    const uint8_t pixel_x1_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 1);

    EXPECT_EQ(pixel_x0_y0, 1);
    EXPECT_EQ(pixel_x1_y0, 2);
    EXPECT_EQ(pixel_x0_y1, 3);
    EXPECT_EQ(pixel_x1_y1, 4);
}

TEST(Accessor, CreateDocumentAndCheckSingleChannelScalingAccessorWithSubBlockCache)
{
    // we use the same CZI-document as before, but we use subblock-cache 
    auto czi_document_as_blob = CreateCziWithFourSubblockInMosaicArragengement();

    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);

    const auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    const auto subblock_cache = CreateSubBlockCache();
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0,0,0 };   // request to have background cleared with black
    options.subBlockCache = subblock_cache;
    options.onlyUseSubBlockCacheForCompressedData = false;

    // act
    auto composite_bitmap = accessor->Get(
        PixelType::Gray8,
        IntRect{ 1,1,2,2 },
        &plane_coordinate,
        1,
        &options);

    // assert

    // first, check that the result is correct
    ASSERT_EQ(composite_bitmap->GetWidth(), 2);
    ASSERT_EQ(composite_bitmap->GetHeight(), 2);
    {
        const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
        const uint8_t pixel_x0_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 0);
        const uint8_t pixel_x1_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 1);
        const uint8_t pixel_x0_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 0);
        const uint8_t pixel_x1_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 1);

        EXPECT_EQ(pixel_x0_y0, 1);
        EXPECT_EQ(pixel_x1_y0, 2);
        EXPECT_EQ(pixel_x0_y1, 3);
        EXPECT_EQ(pixel_x1_y1, 4);
    }

    const auto cache_statistics = subblock_cache->GetStatistics(ISubBlockCacheStatistics::kMemoryUsage | ISubBlockCacheStatistics::kElementsCount);
    EXPECT_GE(cache_statistics.memoryUsage, 16);
    EXPECT_EQ(cache_statistics.elementsCount, 4);

    // now, we do the same request again, and this time we expect that the subblock-cache is used
    composite_bitmap = accessor->Get(
        PixelType::Gray8,
        IntRect{ 1,1,2,2 },
        &plane_coordinate,
        1,
        &options);

    // we check that the result is the same as before
    ASSERT_EQ(composite_bitmap->GetWidth(), 2);
    ASSERT_EQ(composite_bitmap->GetHeight(), 2);
    {
        const ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
        const uint8_t pixel_x0_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 0);
        const uint8_t pixel_x1_y0 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + 1);
        const uint8_t pixel_x0_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 0);
        const uint8_t pixel_x1_y1 = *(static_cast<const uint8_t*>(lock_info_bitmap.ptrDataRoi) + static_cast<size_t>(1) * lock_info_bitmap.stride + 1);

        EXPECT_EQ(pixel_x0_y0, 1);
        EXPECT_EQ(pixel_x1_y0, 2);
        EXPECT_EQ(pixel_x0_y1, 3);
        EXPECT_EQ(pixel_x1_y1, 4);
    }
}
