// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"

using namespace libCZI;
using namespace std;

namespace
{
    tuple<shared_ptr<void>, size_t> CreateCziDocumentWithTwoOverlappingSubblocksWithMaskData()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateGray8BitmapAndFill(4, 4, 0);

        static const char sub_block_metadata_xml[] =
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
            addSbBlkInfo.ptrSbBlkMetadata = nullptr;// sub_block_metadata_xml;
            addSbBlkInfo.sbBlkMetadataSize = 0;// sub_block_metadata_xml_size;
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
            0xc0,       // the actual mask data - a 4x4 checkerboard pattern   X_X_
            0x50,       //                                                     _X_X
            0xc0,       //                                                     X_X_
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
}

TEST(MaskAwareComposition, CompositionOfTwoSubBlocksWithMask)
{
    // arrange
    const auto czi_and_size = CreateCziDocumentWithTwoOverlappingSubblocksWithMaskData();

    //FILE* fp = fopen("D:\\test.czi", "wb");
    //if (fp != nullptr)
    //{
    //    fwrite(get<0>(czi_and_size).get(), 1, get<1>(czi_and_size), fp);
    //    fclose(fp);
    //}

    // act
    const auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    const auto reader = CreateCZIReader();
    reader->Open(inputStream);
    const auto sub_block = reader->ReadSubBlock(1);

    // assert
    auto sub_block_attachment_accessor = CreateSubBlockAttachmentAccessor(sub_block, nullptr);
    ASSERT_TRUE(sub_block_attachment_accessor->HasChunkContainer());
    const auto mask_bitonal_bitmap = sub_block_attachment_accessor->CreateBitonalBitmapFromMaskInfo();
    ASSERT_TRUE(mask_bitonal_bitmap);
    ASSERT_TRUE(mask_bitonal_bitmap->GetWidth() == 4);
    ASSERT_TRUE(mask_bitonal_bitmap->GetHeight() == 4);
}
