// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include "misc_types.h"

#pragma pack(push, 4)

/// This structure contains the information about a sub-block.
struct SubBlockInfoInterop
{
    std::int32_t compression_mode_raw;      ///< The (raw) compression mode identification of the sub-block. This value is not interpreted, use "GetCompressionMode" to have it
                                            ///< converted to the CompressionMode-enumeration.

    std::int32_t pixel_type;                ///< The pixel type of the bitmap contained in the sub-block.

    CoordinateInterop coordinate;           ///< The coordinate of the sub-block.

    IntRectInterop logical_rect;            ///< The rectangle where the bitmap (in this sub-block) is located (in the CZI-pixel-coordinate system).

    IntSizeInterop physical_size;           ///< The physical size of the bitmap of the sub-block (which may be different to the size of logicalRect).

    std::int32_t m_index;                   ///< The M-index. If this has the value of 'numeric_limits<int32_t>::min()', then the M-index is not valid.
};

#pragma pack(pop)
