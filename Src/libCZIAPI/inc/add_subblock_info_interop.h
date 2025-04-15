// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include "misc_types.h"

#pragma pack(push, 4)
struct AddSubBlockInfoInterop
{
    CoordinateInterop coordinate;
    std::uint8_t m_index_valid;
    std::int32_t m_index;            
    std::int32_t x;                 
    std::int32_t y;                 
    std::int32_t logical_width;      
    std::int32_t logical_height;     
    std::int32_t physical_width;     
    std::int32_t physical_height;    

    std::int32_t pixel_type;
    std::int32_t compression_mode_raw;

    std::uint32_t size_data;
    const void* data;

    /// If the compression mode is set to 'Uncompressed', then this is valid and the stride of the bitmap.
    /// In this case, the line-size of the bitmap is determined by the pixel-type and the physical_width.
    /// And size_data must be large enough to hold the bitmap-data, and is validated.
    /// In other cases (compression-mode is not 'Uncompressed'), this field is ignored.
    std::uint32_t stride;

    std::uint32_t size_metadata;
    const void* metadata;

    std::uint32_t size_attachment;
    const void* attachment;
};
#pragma pack(pop)
