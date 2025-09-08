// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/bitmapData.h"
#include "MemOutputStream.h"
#include "utils.h"

using namespace libCZI;
using namespace std;

namespace
{
    /// Creates czi document with two overlapping subblocks of pixel type Gray8 with mask data.
    /// We have a subblock (M=0) at 0,0 (4x4 gray8, filled with 0) with no mask, and 
    /// a subblock (M=1) at 2,2 (4x4 gray8, filled with 255) with a mask (4x4 checkerboard pattern).
    ///
    /// \returns	A tuple with the CZI-data and its size.
    tuple<shared_ptr<void>, size_t> CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateGray8BitmapAndFill(4, 4, 0);

        static constexpr char sub_block_metadata_xml[] =
            "<METADATA>"
            "<AttachmentSchema>"
            "<DataFormat>CHUNKCONTAINER</DataFormat>"
            "</AttachmentSchema>"
            "</METADATA>";
        constexpr size_t sub_block_metadata_xml_size = sizeof(sub_block_metadata_xml) - 1;

        {
            ScopedBitmapLockerSP lockBm{ bitmap };
            AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = 0;
            addSbBlkInfo.x = 0;
            addSbBlkInfo.y = 0;
            addSbBlkInfo.logicalWidth = bitmap->GetWidth();
            addSbBlkInfo.logicalHeight = bitmap->GetHeight();
            addSbBlkInfo.physicalWidth = bitmap->GetWidth();
            addSbBlkInfo.physicalHeight = bitmap->GetHeight();
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
            addSbBlkInfo.strideBitmap = lockBm.stride;
            addSbBlkInfo.ptrSbBlkMetadata = nullptr;
            addSbBlkInfo.sbBlkMetadataSize = 0;
            writer->SyncAddSubBlock(addSbBlkInfo);
        }

        bitmap = CreateGray8BitmapAndFill(4, 4, 255);

        static const uint8_t sub_block_attachment[] =
        {
            0x67, 0xEA, 0xE3, 0xCB, 0xFC, 0x5B, 0x2B, 0x49, 0xA1, 0x6A, 0xEC, 0xE3, 0x78, 0x03, 0x14, 0x48, // that's the GUID of the 'mask' chunk
            0x14, 0x00, 0x00, 0x00, // the size - 20 bytes of data
            0x04, 0x00, 0x00, 0x00, // the width (4 pixels)
            0x04, 0x00, 0x00, 0x00, // the height (4 pixels)
            0x00, 0x00, 0x00, 0x00, // the representation type (0 -> uncompressed bitonal bitmap)
            0x01, 0x00, 0x00, 0x00, // the stride (1 byte per row)
            0xa0,       // the actual mask data - a 4x4 checkerboard pattern   X_X_
            0x50,       //                                                     _X_X
            0xa0,       //                                                     X_X_
            0x50        //                                                     _X_X
        };

        {
            ScopedBitmapLockerSP lockBm{ bitmap };
            AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = 1;
            addSbBlkInfo.x = 2;
            addSbBlkInfo.y = 2;
            addSbBlkInfo.logicalWidth = bitmap->GetWidth();
            addSbBlkInfo.logicalHeight = bitmap->GetHeight();
            addSbBlkInfo.physicalWidth = bitmap->GetWidth();
            addSbBlkInfo.physicalHeight = bitmap->GetHeight();
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
            addSbBlkInfo.strideBitmap = lockBm.stride;
            addSbBlkInfo.ptrSbBlkAttachment = sub_block_attachment;
            addSbBlkInfo.sbBlkAttachmentSize = sizeof(sub_block_attachment);
            addSbBlkInfo.ptrSbBlkMetadata = sub_block_metadata_xml;
            addSbBlkInfo.sbBlkMetadataSize = sub_block_metadata_xml_size;
            writer->SyncAddSubBlock(addSbBlkInfo);
        }

        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }

    /// Creates czi document with two overlapping subblocks of pixel type Gray16 with mask data.
    /// We have a subblock (M=0) at 0,0 (4x4 gray16, filled with 0) with no mask, and 
    /// a subblock (M=1) at 2,2 (4x4 gray16, filled with 256) with a mask (4x4 checkerboard pattern).
    ///
    /// \returns	A tuple with the CZI-data and its size.
    tuple<shared_ptr<void>, size_t> CreateCziDocumentWithTwoOverlappingSubblocksGray16WithMaskData()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateGray16BitmapAndFill(4, 4, 0);

        static constexpr char sub_block_metadata_xml[] =
            "<METADATA>"
            "<AttachmentSchema>"
            "<DataFormat>CHUNKCONTAINER</DataFormat>"
            "</AttachmentSchema>"
            "</METADATA>";
        constexpr size_t sub_block_metadata_xml_size = sizeof(sub_block_metadata_xml) - 1;

        {
            ScopedBitmapLockerSP lockBm{ bitmap };
            AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = 0;
            addSbBlkInfo.x = 0;
            addSbBlkInfo.y = 0;
            addSbBlkInfo.logicalWidth = bitmap->GetWidth();
            addSbBlkInfo.logicalHeight = bitmap->GetHeight();
            addSbBlkInfo.physicalWidth = bitmap->GetWidth();
            addSbBlkInfo.physicalHeight = bitmap->GetHeight();
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
            addSbBlkInfo.strideBitmap = lockBm.stride;
            addSbBlkInfo.ptrSbBlkMetadata = nullptr;
            addSbBlkInfo.sbBlkMetadataSize = 0;
            writer->SyncAddSubBlock(addSbBlkInfo);
        }

        bitmap = CreateGray16BitmapAndFill(4, 4, 256);

        static const uint8_t sub_block_attachment[] =
        {
            0x67, 0xEA, 0xE3, 0xCB, 0xFC, 0x5B, 0x2B, 0x49, 0xA1, 0x6A, 0xEC, 0xE3, 0x78, 0x03, 0x14, 0x48, // that's the GUID of the 'mask' chunk
            0x14, 0x00, 0x00, 0x00, // the size - 20 bytes of data
            0x04, 0x00, 0x00, 0x00, // the width (4 pixels)
            0x04, 0x00, 0x00, 0x00, // the height (4 pixels)
            0x00, 0x00, 0x00, 0x00, // the representation type (0 -> uncompressed bitonal bitmap)
            0x01, 0x00, 0x00, 0x00, // the stride (1 byte per row)
            0xa0,       // the actual mask data - a 4x4 checkerboard pattern   X_X_
            0x50,       //                                                     _X_X
            0xa0,       //                                                     X_X_
            0x50        //                                                     _X_X
        };

        {
            ScopedBitmapLockerSP lockBm{ bitmap };
            AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = 1;
            addSbBlkInfo.x = 2;
            addSbBlkInfo.y = 2;
            addSbBlkInfo.logicalWidth = bitmap->GetWidth();
            addSbBlkInfo.logicalHeight = bitmap->GetHeight();
            addSbBlkInfo.physicalWidth = bitmap->GetWidth();
            addSbBlkInfo.physicalHeight = bitmap->GetHeight();
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
            addSbBlkInfo.strideBitmap = lockBm.stride;
            addSbBlkInfo.ptrSbBlkAttachment = sub_block_attachment;
            addSbBlkInfo.sbBlkAttachmentSize = sizeof(sub_block_attachment);
            addSbBlkInfo.ptrSbBlkMetadata = sub_block_metadata_xml;
            addSbBlkInfo.sbBlkMetadataSize = sub_block_metadata_xml_size;
            writer->SyncAddSubBlock(addSbBlkInfo);
        }

        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }

    tuple<shared_ptr<void>, size_t> CreateCziDocumentWithOneSubBlockWhereMaskDataIsTooSmall()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateGray8BitmapAndFill(4, 4, 0);

        static constexpr char sub_block_metadata_xml[] =
            "<METADATA>"
            "<AttachmentSchema>"
            "<DataFormat>CHUNKCONTAINER</DataFormat>"
            "</AttachmentSchema>"
            "</METADATA>";
        constexpr size_t sub_block_metadata_xml_size = sizeof(sub_block_metadata_xml) - 1;

        bitmap = CreateGray8BitmapAndFill(5, 5, 255);

        static const uint8_t sub_block_attachment[] =
        {
            0x67, 0xEA, 0xE3, 0xCB, 0xFC, 0x5B, 0x2B, 0x49, 0xA1, 0x6A, 0xEC, 0xE3, 0x78, 0x03, 0x14, 0x48, // that's the GUID of the 'mask' chunk
            0x14, 0x00, 0x00, 0x00, // the size - 20 bytes of data
            0x04, 0x00, 0x00, 0x00, // the width (4 pixels)
            0x04, 0x00, 0x00, 0x00, // the height (4 pixels)
            0x00, 0x00, 0x00, 0x00, // the representation type (0 -> uncompressed bitonal bitmap)
            0x01, 0x00, 0x00, 0x00, // the stride (1 byte per row)
            0xa0,       // the actual mask data - a 4x4 checkerboard pattern   X_X_
            0x50,       //                                                     _X_X
            0xa0,       //                                                     X_X_
            0x50        //                                                     _X_X
        };

        {
            ScopedBitmapLockerSP lockBm{ bitmap };
            AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = 0;
            addSbBlkInfo.x = 2;
            addSbBlkInfo.y = 2;
            addSbBlkInfo.logicalWidth = bitmap->GetWidth();
            addSbBlkInfo.logicalHeight = bitmap->GetHeight();
            addSbBlkInfo.physicalWidth = bitmap->GetWidth();
            addSbBlkInfo.physicalHeight = bitmap->GetHeight();
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
            addSbBlkInfo.strideBitmap = lockBm.stride;
            addSbBlkInfo.ptrSbBlkAttachment = sub_block_attachment;
            addSbBlkInfo.sbBlkAttachmentSize = sizeof(sub_block_attachment);
            addSbBlkInfo.ptrSbBlkMetadata = sub_block_metadata_xml;
            addSbBlkInfo.sbBlkMetadataSize = sub_block_metadata_xml_size;
            writer->SyncAddSubBlock(addSbBlkInfo);
        }

        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }
}

TEST(MaskAwareComposition, ReadSubBlockWithMaskAndExamineIt)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    // act
    const auto sub_block = reader->ReadSubBlock(1);

    // assert
    auto sub_block_attachment_accessor = CreateSubBlockAttachmentAccessor(sub_block, nullptr);
    ASSERT_TRUE(sub_block_attachment_accessor->HasChunkContainer());
    const auto sub_block_attachment_mask_info_general = sub_block_attachment_accessor->GetValidPixelMaskFromChunkContainer();
    ASSERT_EQ(sub_block_attachment_mask_info_general.width, 4);
    ASSERT_EQ(sub_block_attachment_mask_info_general.height, 4);
    ASSERT_EQ(sub_block_attachment_mask_info_general.type_of_representation, 0);
    ASSERT_EQ(sub_block_attachment_mask_info_general.size_data, 8);
    static const uint8_t expected_bitonal_bitmap_data[8] = { 0x01, 0x00, 0x00, 0x00, 0xa0, 0x50, 0xa0, 0x50 };
    ASSERT_EQ(memcmp(sub_block_attachment_mask_info_general.data.get(), expected_bitonal_bitmap_data, 8), 0);

    const auto mask_bitonal_bitmap = sub_block_attachment_accessor->CreateBitonalBitmapFromMaskInfo();
    ASSERT_TRUE(mask_bitonal_bitmap);
    ASSERT_EQ(mask_bitonal_bitmap->GetWidth(), 4);
    ASSERT_EQ(mask_bitonal_bitmap->GetHeight(), 4);
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 0, 0));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 1, 0));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 2, 0));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 3, 0));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 0, 1));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 1, 1));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 2, 1));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 3, 1));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 0, 2));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 1, 2));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 2, 2));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 3, 2));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 0, 3));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 1, 3));
    ASSERT_FALSE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 2, 3));
    ASSERT_TRUE(BitonalBitmapOperations::GetPixelValue(mask_bitonal_bitmap, 3, 3));
}

TEST(MaskAwareComposition, SingleChannelScalingTileAccessorWithMaskGray8Scenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0,0,6,6 }, &plane_coordinate, 1.f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 6);
    ASSERT_EQ(composition->GetHeight(), 6);
    ASSERT_EQ(composition->GetPixelType(), PixelType::Gray8);

    // The expected result is a 6x6 image where:
    // - The background is gray (128,128,128).
    // - then, the first sub-block (black, 0) is drawn at (0,0) - (4,4)
    // - then, the second sub-block (white, 255) is drawn at (2,2) - (6,6) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0xff, 0x00, 0xff, 0x80,
        0x00, 0x00, 0x00, 0xff, 0x80, 0xff,
        0x80, 0x80, 0xff, 0x80, 0xff, 0x80,
        0x80, 0x80, 0x80, 0xff, 0x80, 0xff,
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 6, 6);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelScalingTileAccessorWithMaskGray16Scenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray16WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0,0,6,6 }, &plane_coordinate, 1.f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 6);
    ASSERT_EQ(composition->GetHeight(), 6);
    ASSERT_EQ(composition->GetPixelType(), PixelType::Gray16);

    // The expected result is a 6x6 image where:
    // - The background is gray (32768,32768,32768).
    // - then, the first sub-block (black, 0) is drawn at (0,0) - (4,4)
    // - then, the second sub-block (white, 256) is drawn at (2,2) - (6,6) with the checkerboard mask applied
    static const uint16_t expected_result[] =
    {
        0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x8000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x8000,
        0x0000, 0x0000, 0x0100, 0x0000, 0x0100, 0x8000,
        0x0000, 0x0000, 0x0000, 0x0100, 0x8000, 0x0100,
        0x8000, 0x8000, 0x0100, 0x8000, 0x0100, 0x8000,
        0x8000, 0x8000, 0x8000, 0x0100, 0x8000, 0x0100,
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint16_t* composition_line = (const uint16_t*)(static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride);
        int r = memcmp(composition_line, expected_result + y * 6, 6 * sizeof(uint16_t));
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelScalingTileAccessorWithMaskScenario1MaskAwareCompositingOff)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = false;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0,0,6,6 }, &plane_coordinate, 1.f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 6);
    ASSERT_EQ(composition->GetHeight(), 6);

    // The expected result is a 6x6 image where:
    // - The background is gray (128,128,128).
    // - then, the first sub-block (black, 0) is drawn at (0,0) - (4,4)
    // - then, the second sub-block (white, 255) is drawn at (2,2) - (6,6) (with the checkerboard mask ignored)
    static const uint8_t expected_result[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x80, 0x80, 0xff, 0xff, 0xff, 0xff,
        0x80, 0x80, 0xff, 0xff, 0xff, 0xff,
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 6, 6);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelScalingTileAccessorWithMaskScenario2)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    // act
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.25f, 0.25f, 0.25f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ -1,-1,8,8 }, &plane_coordinate, 1.f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 8);
    ASSERT_EQ(composition->GetHeight(), 8);

    // The expected result is a 8x8 image where:
    // - The background is gray (64,64,64).
    // - then, the first sub-block (black, 0) is drawn at 1,1) - (5,5)
    // - then, the second sub-block (white, 255) is drawn at (3,3) - (7,7) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0xff, 0x00, 0xff, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40, 0x40,
        0x40, 0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    };

    // assert
    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 8, 8);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelScalingTileAccessorWithMaskScenario3)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    auto destination_bitmap = CreateRandomBitmap(PixelType::Gray8, 5, 5);

    // create a copy of the original background
    auto copy_of_background = CStdBitmapData::Create(destination_bitmap->GetPixelType(), destination_bitmap->GetWidth(), destination_bitmap->GetHeight());
    {
        ScopedBitmapLockerSP lockCopy{ copy_of_background };
        ScopedBitmapLockerSP sourceLock{ destination_bitmap };
        CBitmapOperations::Copy(
            destination_bitmap->GetPixelType(),
            sourceLock.ptrDataRoi,
            sourceLock.stride,
            copy_of_background->GetPixelType(),
            lockCopy.ptrDataRoi,
            lockCopy.stride,
            destination_bitmap->GetWidth(),
            destination_bitmap->GetHeight(),
            false);
    }

    // act
    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    // instruct to NOT clear the background, i.e. the content of 'destination_bitmap' is the background
    options.backGroundColor = RgbFloatColor{ numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN() };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    accessor->Get(
                destination_bitmap.get(),
                IntRectAndFrameOfReference{ CZIFrameOfReference::PixelCoordinateSystem,IntRect{ 2,2,5,5 } },
                &plane_coordinate,
                1.f,
                &options);

    // assert
    ScopedBitmapLockerSP copy_of_background_locker{ copy_of_background };
    ScopedBitmapLockerSP destination_locker{ destination_bitmap };
    for (size_t y = 0; y < destination_bitmap->GetHeight(); ++y)
    {
        const uint8_t* copy_of_background_line = static_cast<const uint8_t*>(copy_of_background_locker.ptrDataRoi) + y * copy_of_background_locker.stride;
        const uint8_t* destination_line = static_cast<const uint8_t*>(destination_locker.ptrDataRoi) + y * destination_locker.stride;
        for (size_t x = 0; x < destination_bitmap->GetWidth(); ++x)
        {
            const uint8_t pixel_value_composition = destination_line[x];
            if (x == 1 && y == 0 || x == 0 && y == 1)
            {
                // for those pixels, we expect the subblock#0 (=0x00)
                ASSERT_EQ(pixel_value_composition, 0);
            }
            else if ((y == 0 && (x == 0 || x == 2)) || (y == 1 && (x == 1 || x == 3)) || (y == 2 && (x == 0 || x == 2)) || (y == 3 && (x == 1 || x == 3)))
            {
                // for those pixels, we expect the (valid) pixels of subblock#1 (=0xff)
                ASSERT_EQ(pixel_value_composition, 0xff);
            }
            else
            {
                // otherwise - we expect the original pixel value
                ASSERT_EQ(pixel_value_composition, copy_of_background_line[x]);
            }
        }
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorWithMaskScenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelTileAccessor();

    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(0, 0, 6, 6, &plane_coordinate, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 6);
    ASSERT_EQ(composition->GetHeight(), 6);

    // The expected result is a 6x6 image where:
    // - The background is gray (128,128,128).
    // - then, the first sub-block (black, 0) is drawn at (0,0) - (4,4)
    // - then, the second sub-block (white, 255) is drawn at (2,2) - (6,6) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0xff, 0x00, 0xff, 0x80,
        0x00, 0x00, 0x00, 0xff, 0x80, 0xff,
        0x80, 0x80, 0xff, 0x80, 0xff, 0x80,
        0x80, 0x80, 0x80, 0xff, 0x80, 0xff,
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 6, 6);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorScalingGray8WithMaskScenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0, 0, 6, 6 }, &plane_coordinate, 0.5f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 3);
    ASSERT_EQ(composition->GetHeight(), 3);

    // We expect a nearby-neighbor scaling of the previous expected result
    static const uint8_t expected_result[] =
    {
        0x00, 0x00, 0x00,
        0x00, 0xff, 0xff,
        0x00, 0xff, 0xff
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 3, 3);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorScalingGray16WithMaskScenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray16WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    ISingleChannelScalingTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0, 0, 6, 6 }, &plane_coordinate, 0.5f, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 3);
    ASSERT_EQ(composition->GetHeight(), 3);

    // We expect a nearby-neighbor scaling of the previous expected result
    static const uint16_t expected_result[] =
    {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0100, 0x0100,
        0x0000, 0x0100, 0x0100
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint16_t* composition_line = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride);
        int r = memcmp(composition_line, expected_result + y * 3, 3 * sizeof(uint16_t));
        ASSERT_EQ(r, 0);
    }
}


TEST(MaskAwareComposition, SingleChannelTileAccessorWithMaskScenario2)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelTileAccessor();

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.25f, 0.25f, 0.25f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(-1, -1, 8, 8, &plane_coordinate, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 8);
    ASSERT_EQ(composition->GetHeight(), 8);

    // The expected result is a 8x8 image where:
    // - The background is gray (64,64,64).
    // - then, the first sub-block (black, 0) is drawn at 1,1) - (5,5)
    // - then, the second sub-block (white, 255) is drawn at (3,3) - (7,7) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0xff, 0x00, 0xff, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40, 0x40,
        0x40, 0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    };

    // assert
    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 8, 8);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorWithMaskScenario3)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto accessor = reader->CreateSingleChannelTileAccessor();

    auto destination_bitmap = CreateRandomBitmap(PixelType::Gray8, 5, 5);

    // create a copy of the original background
    auto copy_of_background = CStdBitmapData::Create(destination_bitmap->GetPixelType(), destination_bitmap->GetWidth(), destination_bitmap->GetHeight());
    {
        ScopedBitmapLockerSP lockCopy{ copy_of_background };
        ScopedBitmapLockerSP sourceLock{ destination_bitmap };
        CBitmapOperations::Copy(
            destination_bitmap->GetPixelType(),
            sourceLock.ptrDataRoi,
            sourceLock.stride,
            copy_of_background->GetPixelType(),
            lockCopy.ptrDataRoi,
            lockCopy.stride,
            destination_bitmap->GetWidth(),
            destination_bitmap->GetHeight(),
            false);
    }

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    // instruct to NOT clear the background, i.e. the content of 'destination_bitmap' is the background
    options.backGroundColor = RgbFloatColor{ numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN() };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    accessor->Get(
                destination_bitmap.get(),
                IntPointAndFrameOfReference{ CZIFrameOfReference::PixelCoordinateSystem,IntPoint{ 2,2 } },
                &plane_coordinate,
                &options);

    // assert
    ScopedBitmapLockerSP copy_of_background_locker{ copy_of_background };
    ScopedBitmapLockerSP destination_locker{ destination_bitmap };
    for (size_t y = 0; y < destination_bitmap->GetHeight(); ++y)
    {
        const uint8_t* copy_of_background_line = static_cast<const uint8_t*>(copy_of_background_locker.ptrDataRoi) + y * copy_of_background_locker.stride;
        const uint8_t* destination_line = static_cast<const uint8_t*>(destination_locker.ptrDataRoi) + y * destination_locker.stride;
        for (size_t x = 0; x < destination_bitmap->GetWidth(); ++x)
        {
            const uint8_t pixel_value_composition = destination_line[x];
            if (x == 1 && y == 0 || x == 0 && y == 1)
            {
                // for those pixels, we expect the subblock#0 (=0x00)
                ASSERT_EQ(pixel_value_composition, 0);
            }
            else if ((y == 0 && (x == 0 || x == 2)) || (y == 1 && (x == 1 || x == 3)) || (y == 2 && (x == 0 || x == 2)) || (y == 3 && (x == 1 || x == 3)))
            {
                // for those pixels, we expect the (valid) pixels of subblock#1 (=0xff)
                ASSERT_EQ(pixel_value_composition, 0xff);
            }
            else
            {
                // otherwise - we expect the original pixel value
                ASSERT_EQ(pixel_value_composition, copy_of_background_line[x]);
            }
        }
    }
}

TEST(MaskAwareComposition, SingleChannelPyramidLayerAccessorWithMaskScenario1)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();

    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.5f, 0.5f, 0.5f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ 0, 0, 6, 6 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2,0 }, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 6);
    ASSERT_EQ(composition->GetHeight(), 6);

    // The expected result is a 6x6 image where:
    // - The background is gray (128,128,128).
    // - then, the first sub-block (black, 0) is drawn at (0,0) - (4,4)
    // - then, the second sub-block (white, 255) is drawn at (2,2) - (6,6) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
        0x00, 0x00, 0xff, 0x00, 0xff, 0x80,
        0x00, 0x00, 0x00, 0xff, 0x80, 0xff,
        0x80, 0x80, 0xff, 0x80, 0xff, 0x80,
        0x80, 0x80, 0x80, 0xff, 0x80, 0xff,
    };

    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 6, 6);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelPyramidLayerAccessorWithMaskScenario2)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();

    // act
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();
    options.backGroundColor = RgbFloatColor{ 0.25f, 0.25f, 0.25f };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto composition = accessor->Get(IntRect{ -1, -1, 8, 8 }, &plane_coordinate, ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2,0 }, &options);
    ASSERT_TRUE(composition);
    ASSERT_EQ(composition->GetWidth(), 8);
    ASSERT_EQ(composition->GetHeight(), 8);

    // The expected result is a 8x8 image where:
    // - The background is gray (64,64,64).
    // - then, the first sub-block (black, 0) is drawn at 1,1) - (5,5)
    // - then, the second sub-block (white, 255) is drawn at (3,3) - (7,7) with the checkerboard mask applied
    static const uint8_t expected_result[] =
    {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40,
        0x40, 0x00, 0x00, 0xff, 0x00, 0xff, 0x40, 0x40,
        0x40, 0x00, 0x00, 0x00, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40, 0x40,
        0x40, 0x40, 0x40, 0x40, 0xff, 0x40, 0xff, 0x40,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    };

    // assert
    ScopedBitmapLockerSP locker_composition{ composition };
    ASSERT_TRUE(locker_composition.ptrDataRoi != nullptr);
    for (size_t y = 0; y < composition->GetHeight(); ++y)
    {
        const uint8_t* composition_line = static_cast<const uint8_t*>(locker_composition.ptrDataRoi) + y * locker_composition.stride;
        int r = memcmp(composition_line, expected_result + y * 8, 8);
        ASSERT_EQ(r, 0);
    }
}

TEST(MaskAwareComposition, SingleChannelPyramidLayerTileAccessorWithMaskScenario3)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksGray8WithMaskData();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();

    auto destination_bitmap = CreateRandomBitmap(PixelType::Gray8, 5, 5);

    // create a copy of the original background
    auto copy_of_background = CStdBitmapData::Create(destination_bitmap->GetPixelType(), destination_bitmap->GetWidth(), destination_bitmap->GetHeight());
    {
        ScopedBitmapLockerSP lockCopy{ copy_of_background };
        ScopedBitmapLockerSP sourceLock{ destination_bitmap };
        CBitmapOperations::Copy(
            destination_bitmap->GetPixelType(),
            sourceLock.ptrDataRoi,
            sourceLock.stride,
            copy_of_background->GetPixelType(),
            lockCopy.ptrDataRoi,
            lockCopy.stride,
            destination_bitmap->GetWidth(),
            destination_bitmap->GetHeight(),
            false);
    }

    // act
    ISingleChannelPyramidLayerTileAccessor::Options options;
    options.Clear();
    // instruct to NOT clear the background, i.e. the content of 'destination_bitmap' is the background
    options.backGroundColor = RgbFloatColor{ numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN() };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    accessor->Get(
                destination_bitmap.get(),
                IntPointAndFrameOfReference{ CZIFrameOfReference::PixelCoordinateSystem,IntPoint{ 2,2 } },
                &plane_coordinate,
                ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo{ 2,0 }, // pyramid layer 0
                &options);

    // assert
    ScopedBitmapLockerSP copy_of_background_locker{ copy_of_background };
    ScopedBitmapLockerSP destination_locker{ destination_bitmap };
    for (size_t y = 0; y < destination_bitmap->GetHeight(); ++y)
    {
        const uint8_t* copy_of_background_line = static_cast<const uint8_t*>(copy_of_background_locker.ptrDataRoi) + y * copy_of_background_locker.stride;
        const uint8_t* destination_line = static_cast<const uint8_t*>(destination_locker.ptrDataRoi) + y * destination_locker.stride;
        for (size_t x = 0; x < destination_bitmap->GetWidth(); ++x)
        {
            const uint8_t pixel_value_composition = destination_line[x];
            if (x == 1 && y == 0 || x == 0 && y == 1)
            {
                // for those pixels, we expect the subblock#0 (=0x00)
                ASSERT_EQ(pixel_value_composition, 0);
            }
            else if ((y == 0 && (x == 0 || x == 2)) || (y == 1 && (x == 1 || x == 3)) || (y == 2 && (x == 0 || x == 2)) || (y == 3 && (x == 1 || x == 3)))
            {
                // for those pixels, we expect the (valid) pixels of subblock#1 (=0xff)
                ASSERT_EQ(pixel_value_composition, 0xff);
            }
            else
            {
                // otherwise - we expect the original pixel value
                ASSERT_EQ(pixel_value_composition, copy_of_background_line[x]);
            }
        }
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorMaskTooSmall)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithOneSubBlockWhereMaskDataIsTooSmall();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto accessor = reader->CreateSingleChannelTileAccessor();

    auto destination_bitmap = CreateRandomBitmap(PixelType::Gray8, 6, 6);

    // create a copy of the original background
    auto copy_of_background = CStdBitmapData::Create(destination_bitmap->GetPixelType(), destination_bitmap->GetWidth(), destination_bitmap->GetHeight());
    {
        ScopedBitmapLockerSP lockCopy{ copy_of_background };
        ScopedBitmapLockerSP sourceLock{ destination_bitmap };
        CBitmapOperations::Copy(
            destination_bitmap->GetPixelType(),
            sourceLock.ptrDataRoi,
            sourceLock.stride,
            copy_of_background->GetPixelType(),
            lockCopy.ptrDataRoi,
            lockCopy.stride,
            destination_bitmap->GetWidth(),
            destination_bitmap->GetHeight(),
            false);
    }

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    // instruct to NOT clear the background, i.e. the content of 'destination_bitmap' is the background
    options.backGroundColor = RgbFloatColor{ numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN() };
    options.maskAware = true;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    accessor->Get(
                destination_bitmap.get(),
                IntPointAndFrameOfReference{ CZIFrameOfReference::PixelCoordinateSystem,IntPoint{ 0,0 } },
                &plane_coordinate,
                &options);

    // assert
    // We expect the "pixels not covered by the mask" to be "masked out" (i.e. not copied)
    ScopedBitmapLockerSP copy_of_background_locker{ copy_of_background };
    ScopedBitmapLockerSP destination_locker{ destination_bitmap };
    for (size_t y = 0; y < destination_bitmap->GetHeight(); ++y)
    {
        const uint8_t* copy_of_background_line = static_cast<const uint8_t*>(copy_of_background_locker.ptrDataRoi) + y * copy_of_background_locker.stride;
        const uint8_t* destination_line = static_cast<const uint8_t*>(destination_locker.ptrDataRoi) + y * destination_locker.stride;
        for (size_t x = 0; x < destination_bitmap->GetWidth(); ++x)
        {
            const uint8_t pixel_value_composition = destination_line[x];
            if ((y == 0 && (x == 0 || x == 2)) || (y == 1 && (x == 1 || x == 3)) || (y == 2 && (x == 0 || x == 2)) || (y == 3 && (x == 1 || x == 3)))
            {
                // for those pixels, we expect the (valid) pixels of subblock#1 (=0xff)
                ASSERT_EQ(pixel_value_composition, 0xff);
            }
            else
            {
                // otherwise - we expect the original pixel value
                ASSERT_EQ(pixel_value_composition, copy_of_background_line[x]);
            }
        }
    }
}

TEST(MaskAwareComposition, SingleChannelTileAccessorMaskTooSmallComposeWithoutMask)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithOneSubBlockWhereMaskDataIsTooSmall();
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto accessor = reader->CreateSingleChannelTileAccessor();

    auto destination_bitmap = CreateRandomBitmap(PixelType::Gray8, 6, 6);

    // create a copy of the original background
    auto copy_of_background = CStdBitmapData::Create(destination_bitmap->GetPixelType(), destination_bitmap->GetWidth(), destination_bitmap->GetHeight());
    {
        ScopedBitmapLockerSP lockCopy{ copy_of_background };
        ScopedBitmapLockerSP sourceLock{ destination_bitmap };
        CBitmapOperations::Copy(
            destination_bitmap->GetPixelType(),
            sourceLock.ptrDataRoi,
            sourceLock.stride,
            copy_of_background->GetPixelType(),
            lockCopy.ptrDataRoi,
            lockCopy.stride,
            destination_bitmap->GetWidth(),
            destination_bitmap->GetHeight(),
            false);
    }

    // act
    ISingleChannelTileAccessor::Options options;
    options.Clear();
    // instruct to NOT clear the background, i.e. the content of 'destination_bitmap' is the background
    options.backGroundColor = RgbFloatColor{ numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN(), numeric_limits<float>::quiet_NaN() };
    options.maskAware = false;
    const CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    accessor->Get(
                destination_bitmap.get(),
                IntPointAndFrameOfReference{ CZIFrameOfReference::PixelCoordinateSystem,IntPoint{ 0,0 } },
                &plane_coordinate,
                &options);

    // assert
    // We expect the "pixels not covered by the mask" to be "masked out" (i.e. not copied)
    ScopedBitmapLockerSP copy_of_background_locker{ copy_of_background };
    ScopedBitmapLockerSP destination_locker{ destination_bitmap };
    for (size_t y = 0; y < destination_bitmap->GetHeight(); ++y)
    {
        const uint8_t* copy_of_background_line = static_cast<const uint8_t*>(copy_of_background_locker.ptrDataRoi) + y * copy_of_background_locker.stride;
        const uint8_t* destination_line = static_cast<const uint8_t*>(destination_locker.ptrDataRoi) + y * destination_locker.stride;
        for (size_t x = 0; x < destination_bitmap->GetWidth(); ++x)
        {
            const uint8_t pixel_value_composition = destination_line[x];
            if ((y < 5 && (x < 5)))
            {
                // for those pixels, we expect the (valid) pixels of subblock#1 (=0xff)
                ASSERT_EQ(pixel_value_composition, 0xff);
            }
            else
            {
                // otherwise - we expect the original pixel value
                ASSERT_EQ(pixel_value_composition, copy_of_background_line[x]);
            }
        }
    }
}
