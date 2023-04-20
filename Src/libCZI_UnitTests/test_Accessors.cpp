// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include <array>
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include "MemInputOutputStream.h"

using namespace libCZI;
using namespace std;

struct ZOrderAndResultGray8Fixture : public testing::TestWithParam<tuple<int, int, int, array<uint8_t, 4>>> { };

TEST_P(ZOrderAndResultGray8Fixture, CreateDocumentAndUseSingleChannelScalingTileAccessorAndCheckResult)
{
    // We create a document with 3 subblocks, where the M-index (of each subblock) is given by the test parameters.
    // The subblocks are 2x1 pixels, and the pixel values are 42 for the 1st, 45 for the second, and 47 for the third
    // 1st subblock is at (0,0), 2nd at (0,1) and 3rd at (0,2).
    // Then we use a single-channel scaling tile accessor to get the tile composite of size 4x1 pixels at (0,0) and check the result.
    // When doing the tile composite, the M-index is to give the z-order - so depending on the M-index, we expect a different result,
    // which is then checked.

    const auto parameters = GetParam();

    // arrange
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
    addSbBlkInfo.mIndex = get<0>(parameters);
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
    addSbBlkInfo.mIndex = get<1>(parameters);
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
    addSbBlkInfo.mIndex = get<2>(parameters);
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

    size_t czi_document_size;
    shared_ptr<void> czi_document_data = outStream->GetCopy(&czi_document_size);
    outStream.reset();

    auto memory_stream = make_shared<CMemInputOutputStream>(czi_document_data.get(), czi_document_size);
    auto reader = CreateCZIReader();
    reader->Open(memory_stream);
    auto accessor = reader->CreateSingleChannelScalingTileAccessor();
    CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();

    // act
    auto composite_bitmap = accessor->Get(PixelType::Gray8, IntRect{ 0,0,4,1 }, &plane_coordinate, 1.f, &options);

    // assert
    ScopedBitmapLockerSP lock_info_bitmap{ composite_bitmap };
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
