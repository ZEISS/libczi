// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <libCZI.h>
#include <libCZIApi.h>
#include <tuple>
#include <vector>

class Utilities
{
public:
    static std::shared_ptr<libCZI::IBitmapData> CreateGray8BitmapAndFill(std::uint32_t width, std::uint32_t height, std::uint8_t value);
    static libCZI::CDimBounds ConvertDimBoundsInterop(const DimBoundsInterop& dim_bounds_interop);
    static libCZI::CDimCoordinate ConvertCoordinateInterop(const CoordinateInterop& coordinate_interop);

    struct TileInfo
    {
        int32_t x;
        int32_t y;
        std::uint8_t gray8_value;
    };

    struct MosaicInfo
    {
        int tile_width;
        int tile_height;
        std::vector<TileInfo> tiles;
    };

    static std::tuple<std::shared_ptr<void>, size_t> CreateMosaicCzi(const MosaicInfo& mosaic_info);

    static std::tuple<std::shared_ptr<void>, size_t> CreateMultiSceneMosaicCzi(const std::map<int, MosaicInfo>& per_scene_mosaic_info);
};
