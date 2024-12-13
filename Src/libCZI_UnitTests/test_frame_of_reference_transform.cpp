// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include <array>

using namespace libCZI;
using namespace std;

namespace
{
    struct TileInfo
    {
        int32_t x;
        int32_t y;
        uint8_t gray8_value;
    };

    struct MosaicInfo
    {
        int tile_width;
        int tile_height;
        std::vector<TileInfo> tiles;
    };

    tuple<shared_ptr<void>, size_t> CreateMosaicCzi(const MosaicInfo& mosaic_info)
    {
        auto writer = libCZI::CreateCZIWriter();
        auto mem_output_stream = make_shared<CMemOutputStream>(0);

        auto spWriterInfo = make_shared<libCZI::CCziWriterInfo>(
            libCZI::GUID{ 0x1234567, 0x89ab, 0xcdef, {1, 2, 3, 4, 5, 6, 7, 8} },  // NOLINT
            libCZI::CDimBounds{ {libCZI::DimensionIndex::C, 0, 1} },  // set a bounds for C
            0, static_cast<int>(mosaic_info.tiles.size() - 1));  // set a bounds M : 0<=m<=0
        writer->Create(mem_output_stream, spWriterInfo);

        for (size_t i = 0; i < mosaic_info.tiles.size(); ++i)
        {
            auto bitmap = CreateGray8BitmapAndFill(mosaic_info.tile_width, mosaic_info.tile_height, mosaic_info.tiles[i].gray8_value);
            libCZI::AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::C, 0);
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = static_cast<int>(i);
            addSbBlkInfo.x = mosaic_info.tiles[i].x;
            addSbBlkInfo.y = mosaic_info.tiles[i].y;
            addSbBlkInfo.logicalWidth = static_cast<int>(bitmap->GetWidth());
            addSbBlkInfo.logicalHeight = static_cast<int>(bitmap->GetHeight());
            addSbBlkInfo.physicalWidth = static_cast<int>(bitmap->GetWidth());
            addSbBlkInfo.physicalHeight = static_cast<int>(bitmap->GetHeight());
            addSbBlkInfo.PixelType = bitmap->GetPixelType();
            {
                const libCZI::ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
                addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
                addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
                writer->SyncAddSubBlock(addSbBlkInfo);
            }
        }

        const libCZI::PrepareMetadataInfo prepare_metadata_info;
        auto meta_data_builder = writer->GetPreparedMetadata(prepare_metadata_info);

        // NOLINTNEXTLINE: uninitialized struct is OK b/o Clear()
        libCZI::WriteMetadataInfo write_metadata_info;
        write_metadata_info.Clear();
        const auto& strMetadata = meta_data_builder->GetXml();
        write_metadata_info.szMetadata = strMetadata.c_str();
        write_metadata_info.szMetadataSize = strMetadata.size() + 1;
        write_metadata_info.ptrAttachment = nullptr;
        write_metadata_info.attachmentSize = 0;
        writer->SyncWriteMetadata(write_metadata_info);
        writer->Close();
        writer.reset();

        size_t czi_document_size = 0;
        const shared_ptr<void> czi_document_data = mem_output_stream->GetCopy(&czi_document_size);
        return make_tuple(czi_document_data, czi_document_size);
    }

    bool Check2x2Gray8Bitmap(IBitmapData* bitmap, const array<uint8_t,4>& expected)
    {
        if (bitmap->GetWidth() != 2 || bitmap->GetHeight() != 2 || bitmap->GetPixelType() != PixelType::Gray8)
        {
            return false;
        }

        ScopedBitmapLockerP locker_bitmap{ bitmap };
        const uint8_t* p = static_cast<const uint8_t*>(locker_bitmap.ptrDataRoi);
        if (p[0] != expected[0] || p[1] != expected[1])
        {
            return false;
        }

        p += locker_bitmap.stride;
        if (p[0] != expected[2] || p[1] != expected[3])
        {
            return false;
        }

        return true;
    }
}  // namespace

TEST(FrameOfReferenceTransform, UseCziWhichIsNotZeroAlignedAndCallCheckTransformPoint)
{
    // create a 2x2 mosaics, with 1x1-pixel tiles, with subblocks at (-1, -1), (0, -1), (-1, 0), (0, 0)
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);
    auto reader = libCZI::CreateCZIReader();
    auto mem_input_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));

    // we use the defaults here, which means that the reader's default frame-of-reference is "raw subblock coordinate system"
    reader->Open(mem_input_stream);

    IntPointAndFrameOfReference point_and_frame_of_reference;
    point_and_frame_of_reference.point = { 0, 0 };
    point_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    auto transformed_point = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_point.point.x, -1);
    EXPECT_EQ(transformed_point.point.y, -1);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    transformed_point = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);
    EXPECT_EQ(transformed_point.point.x, 0);
    EXPECT_EQ(transformed_point.point.y, 0);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);

    transformed_point = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::Default);
    EXPECT_EQ(transformed_point.point.x, -1);
    EXPECT_EQ(transformed_point.point.y, -1);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    EXPECT_ANY_THROW(reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::Invalid));
}

TEST(FrameOfReferenceTransform, UseCziWhichIsNotZeroAlignedAndCallCheckTransformRectangle)
{
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);
    auto reader = libCZI::CreateCZIReader();
    auto mem_input_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));

    // we use the defaults here, which means that the reader's default frame-of-reference is "raw subblock coordinate system"
    reader->Open(mem_input_stream);

    IntRectAndFrameOfReference rect_and_frame_of_reference;
    rect_and_frame_of_reference.rectangle = { 0, 0, 1, 1 };
    rect_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    auto transformed_rect = reader->TransformRectangle(rect_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_rect.rectangle.x, -1);
    EXPECT_EQ(transformed_rect.rectangle.y, -1);
    EXPECT_EQ(transformed_rect.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    transformed_rect = reader->TransformRectangle(rect_and_frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);
    EXPECT_EQ(transformed_rect.rectangle.x, 0);
    EXPECT_EQ(transformed_rect.rectangle.y, 0);
    EXPECT_EQ(transformed_rect.frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);

    transformed_rect = reader->TransformRectangle(rect_and_frame_of_reference, CZIFrameOfReference::Default);
    EXPECT_EQ(transformed_rect.rectangle.x, -1);
    EXPECT_EQ(transformed_rect.rectangle.y, -1);
    EXPECT_EQ(transformed_rect.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    EXPECT_ANY_THROW(reader->TransformRectangle(rect_and_frame_of_reference, CZIFrameOfReference::Invalid));
}

TEST(FrameOfReferenceTransform, SetDefaultFrameOfReferenceToPixelCoordinateSystemAndCheckTransform)
{
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);
    auto reader = libCZI::CreateCZIReader();
    auto mem_input_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));

    ICZIReader::OpenOptions options;
    options.SetDefault();
    options.default_frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    reader->Open(mem_input_stream, &options);

    IntPointAndFrameOfReference point_and_frame_of_reference;
    point_and_frame_of_reference.point = { 0, 0 };
    point_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::RawSubBlockCoordinateSystem;
    auto transformed_point_pixel_coordinate_system = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);
    EXPECT_EQ(transformed_point_pixel_coordinate_system.point.x, 1);
    EXPECT_EQ(transformed_point_pixel_coordinate_system.point.y, 1);
    EXPECT_EQ(transformed_point_pixel_coordinate_system.frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);

    auto transformed_point_default_coordinate_system = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::Default);
    EXPECT_EQ(transformed_point_default_coordinate_system.point.x, 1);
    EXPECT_EQ(transformed_point_default_coordinate_system.point.y, 1);
    EXPECT_EQ(transformed_point_default_coordinate_system.frame_of_reference, CZIFrameOfReference::PixelCoordinateSystem);
}

TEST(FrameOfReferenceTransform, SetDefaultFrameOfReferenceToRawSubblockCoordinateSystemAndCheckTransform)
{
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);
    auto reader = libCZI::CreateCZIReader();
    auto mem_input_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));

    ICZIReader::OpenOptions options;
    options.SetDefault();
    options.default_frame_of_reference = CZIFrameOfReference::RawSubBlockCoordinateSystem;
    reader->Open(mem_input_stream, &options);

    IntPointAndFrameOfReference point_and_frame_of_reference;
    point_and_frame_of_reference.point = { 0, 0 };
    point_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    auto transformed_rect_pixel_coordinate_system = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_rect_pixel_coordinate_system.point.x, -1);
    EXPECT_EQ(transformed_rect_pixel_coordinate_system.point.y, -1);
    EXPECT_EQ(transformed_rect_pixel_coordinate_system.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    auto transformed_point_default_coordinate_system = reader->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::Default);
    EXPECT_EQ(transformed_point_default_coordinate_system.point.x, -1);
    EXPECT_EQ(transformed_point_default_coordinate_system.point.y, -1);
    EXPECT_EQ(transformed_point_default_coordinate_system.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
}

TEST(FrameOfReferenceTransform, GetTileCompositeAndCheckResult)
{
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);
    auto reader = libCZI::CreateCZIReader();
    auto mem_input_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));

    ICZIReader::OpenOptions options;
    options.SetDefault();
    options.default_frame_of_reference = CZIFrameOfReference::RawSubBlockCoordinateSystem;
    reader->Open(mem_input_stream, &options);

    auto accessor = reader->CreateSingleChannelTileAccessor();
    IntRectAndFrameOfReference rect_and_frame_of_reference;
    rect_and_frame_of_reference.rectangle = { 0, 0, 2, 2 };
    rect_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    ISingleChannelTileAccessor::Options accessor_options;
    accessor_options.Clear();
    accessor_options.backGroundColor = RgbFloatColor{ 0, 0, 0 };
    CDimCoordinate plane_coordinate{ {DimensionIndex::C, 0} };
    auto tile_composite_bitmap = accessor->Get(rect_and_frame_of_reference, &plane_coordinate, &accessor_options);
    EXPECT_TRUE(Check2x2Gray8Bitmap(tile_composite_bitmap.get(), array<uint8_t, 4>{10, 20, 30, 40}));

    rect_and_frame_of_reference.rectangle = { -1, -1, 2, 2 };
    rect_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::RawSubBlockCoordinateSystem;
    tile_composite_bitmap = accessor->Get(rect_and_frame_of_reference, &plane_coordinate, &accessor_options);
    EXPECT_TRUE(Check2x2Gray8Bitmap(tile_composite_bitmap.get(), array<uint8_t, 4>{10, 20, 30, 40}));

    rect_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::Default;
    tile_composite_bitmap = accessor->Get(rect_and_frame_of_reference, &plane_coordinate, &accessor_options);
    EXPECT_TRUE(Check2x2Gray8Bitmap(tile_composite_bitmap.get(), array<uint8_t, 4>{10, 20, 30, 40}));
}

TEST(FrameOfReferenceTransform, UseReaderWriterAndCallAndCheckTransformPoint)
{
    MosaicInfo mosaic_info;
    mosaic_info.tile_width = 1;
    mosaic_info.tile_height = 1;
    mosaic_info.tiles = { {-1, -1, 10}, {0, -1, 20}, {-1, 0, 30}, {0, 0, 40} };
    auto czi_document_as_blob = CreateMosaicCzi(mosaic_info);

    const auto in_out_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader_writer = CreateCZIReaderWriter();
    reader_writer->Create(in_out_stream);

    IntPointAndFrameOfReference point_and_frame_of_reference;
    point_and_frame_of_reference.point = { 0, 0 };
    point_and_frame_of_reference.frame_of_reference = CZIFrameOfReference::PixelCoordinateSystem;
    auto transformed_point = reader_writer->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_point.point.x, -1);
    EXPECT_EQ(transformed_point.point.y, -1);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    // ok, now we remove the subblocks at (-1, -1) and (0, -1) - the bounding-box is now (0, -1, 1, 1), so the point (0, 0) should be transformed to (0, -1)
    reader_writer->RemoveSubBlock(0);
    reader_writer->RemoveSubBlock(2);
    transformed_point = reader_writer->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_point.point.x, 0);
    EXPECT_EQ(transformed_point.point.y, -1);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);

    // and now, add a subblock at (-5,-5) and check the transformation
    auto bitmap = CreateGray8BitmapAndFill(mosaic_info.tile_width, mosaic_info.tile_height, 50);
    libCZI::AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 4;
    addSbBlkInfo.x = -5;
    addSbBlkInfo.y = -5;
    addSbBlkInfo.logicalWidth = static_cast<int>(bitmap->GetWidth());
    addSbBlkInfo.logicalHeight = static_cast<int>(bitmap->GetHeight());
    addSbBlkInfo.physicalWidth = static_cast<int>(bitmap->GetWidth());
    addSbBlkInfo.physicalHeight = static_cast<int>(bitmap->GetHeight());
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    {
        const libCZI::ScopedBitmapLockerSP lock_info_bitmap{ bitmap };
        addSbBlkInfo.ptrBitmap = lock_info_bitmap.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lock_info_bitmap.stride;
        reader_writer->SyncAddSubBlock(addSbBlkInfo);
    }

    transformed_point = reader_writer->TransformPoint(point_and_frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
    EXPECT_EQ(transformed_point.point.x, -5);
    EXPECT_EQ(transformed_point.point.y, -5);
    EXPECT_EQ(transformed_point.frame_of_reference, CZIFrameOfReference::RawSubBlockCoordinateSystem);
}
