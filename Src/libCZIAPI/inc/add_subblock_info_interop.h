// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include "misc_types.h"

#pragma pack(push, 4)
/// This structure is used to pass the subblock information to libCZIAPI, describing a subblock to be added to a CZI-file.
struct AddSubBlockInfoInterop
{
    CoordinateInterop coordinate;       ///< The subblock's coordinate.
    std::uint8_t m_index_valid;         ///< Whether the field 'mIndex' is valid;
    std::int32_t m_index;               ///< The M-index of the subblock.
    std::int32_t x;                     ///< The x-coordinate of the subblock.
    std::int32_t y;                     ///< The y-coordinate of the subblock.
    std::int32_t logical_width;         ///< The logical with of the subblock (in pixels).
    std::int32_t logical_height;        ///< The logical height of the subblock (in pixels).
    std::int32_t physical_width;        ///< The physical with of the subblock (in pixels).
    std::int32_t physical_height;       ///< The physical height of the subblock (in pixels).

    std::int32_t pixel_type;            ///< The pixel type of the subblock.

    /// The compression-mode (applying to the subblock-data). If using a compressed format, the data
    /// passed in must be already compressed - the writer does _not_ perform the compression.
    std::int32_t compression_mode_raw;

    std::uint32_t size_data;            ///< The size of the subblock's data in bytes.
    const void* data;                   ///< Pointer to the data to be put into the subblock.

    /// If the compression mode is set to 'Uncompressed', then this is valid and the stride of the bitmap.
    /// In this case, the line-size of the bitmap is determined by the pixel-type and the physical_width.
    /// And size_data must be large enough to hold the bitmap-data, and is validated.
    /// In other cases (compression-mode is not 'Uncompressed'), this field is ignored.
    std::uint32_t stride;

    std::uint32_t size_metadata;        ///< The size of the subblock-metadata in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).
    const void* metadata;               ///< Pointer to the subblock-metadata.

    std::uint32_t size_attachment;      ///< The size of the subblock-attachment in bytes. If this is 0, then attachment is not used (and no sub-block-metadata written).
    const void* attachment;             ///< Pointer to the subblock-attachment.
};
#pragma pack(pop)
