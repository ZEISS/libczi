// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "utilities.h"
#include <tuple>
#include "MemoryOutputStream.h"
#include "SimpleBitmap.h"

using namespace std;
using namespace libCZI;

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

std::shared_ptr<libCZI::IBitmapData> Utilities::CreateGray8BitmapAndFill(std::uint32_t width, std::uint32_t height, uint8_t value)
{
    auto bm = make_shared<SimpleBitmap>(PixelType::Gray8, width, height);
    ScopedBitmapLockerSP lckBm{ bm };
    uint8_t* data = reinterpret_cast<uint8_t*>(lckBm.ptrDataRoi);
    for (uint32_t y = 0; y < height; ++y)
    {
        uint8_t* dst = data + (static_cast<size_t>(lckBm.stride) * y);
        memset(dst, value, width);
    }

    return bm;
}

tuple<shared_ptr<void>, size_t> Utilities::CreateMosaicCzi(const MosaicInfo& mosaic_info)
{
    auto writer = libCZI::CreateCZIWriter();
    auto mem_output_stream = make_shared<MemoryOutputStream>(0);

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

/*static*/std::tuple<std::shared_ptr<void>, size_t> Utilities::CreateMultiSceneMosaicCzi(const std::map<int,MosaicInfo>& per_scene_mosaic_info)
{
    auto writer = libCZI::CreateCZIWriter();
    auto mem_output_stream = make_shared<MemoryOutputStream>(0);

    auto spWriterInfo = make_shared<libCZI::CCziWriterInfo>(
        libCZI::GUID{ 0x1234567, 0x89ab, 0xcdef, {1, 2, 3, 4, 5, 6, 7, 8} });// ,  // NOLINT

    writer->Create(mem_output_stream, spWriterInfo);

    for (const auto& scene_mosaic_info : per_scene_mosaic_info)
    {
        for (size_t i = 0; i < scene_mosaic_info.second.tiles.size(); ++i)
        {
            auto bitmap = CreateGray8BitmapAndFill(scene_mosaic_info.second.tile_width, scene_mosaic_info.second.tile_height, scene_mosaic_info.second.tiles[i].gray8_value);
            libCZI::AddSubBlockInfoStridedBitmap addSbBlkInfo;
            addSbBlkInfo.Clear();
            addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::C, 0);
            addSbBlkInfo.coordinate.Set(libCZI::DimensionIndex::S, scene_mosaic_info.first);
            addSbBlkInfo.mIndexValid = true;
            addSbBlkInfo.mIndex = static_cast<int>(i);
            addSbBlkInfo.x = scene_mosaic_info.second.tiles[i].x;
            addSbBlkInfo.y = scene_mosaic_info.second.tiles[i].y;
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

//-------------------------------------------------------------------------------------------------

/*static*/libCZI::CDimBounds Utilities::ConvertDimBoundsInterop(const DimBoundsInterop& dim_bounds_interop)
{
    libCZI::CDimBounds dim_bounds;
    int dim_bounds_index = 0;
    for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
    {
        const int index = i - static_cast<int>(libCZI::DimensionIndex::MinDim);
        if (dim_bounds_interop.dimensions_valid & (1 << index))
        {
            dim_bounds.Set(static_cast<libCZI::DimensionIndex>(i), dim_bounds_interop.start[dim_bounds_index], dim_bounds_interop.size[dim_bounds_index]);
            ++dim_bounds_index;
        }
    }

    return dim_bounds;
}

/*static*/libCZI::CDimCoordinate Utilities::ConvertCoordinateInterop(const CoordinateInterop& coordinate_interop)
{
    libCZI::CDimCoordinate coordinate;
    int coordinate_index = 0;
    for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
    {
        const int index = i - static_cast<int>(libCZI::DimensionIndex::MinDim);
        if (coordinate_interop.dimensions_valid & (1 << index))
        {
            coordinate.Set(static_cast<libCZI::DimensionIndex>(i), coordinate_interop.value[coordinate_index]);
            ++coordinate_index;
        }
    }

    return coordinate;
}
