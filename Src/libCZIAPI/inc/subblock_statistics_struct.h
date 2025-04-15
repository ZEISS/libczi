// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include "misc_types.h"

#pragma pack(push, 4)

/// This structure contains basic statistics about an CZI-document.
struct SubBlockStatisticsInterop
{
    std::int32_t sub_block_count; ///< The number of sub-blocks.

    std::int32_t min_m_index; ///< The minimum M-index.
    std::int32_t max_m_index; ///< The maximum M-index.

    IntRectInterop bounding_box; ///< The bounding-box determined from all sub-blocks.

    IntRectInterop bounding_box_layer0; ///< The minimal axis-aligned-bounding box determined only from the logical coordinates of the sub-blocks on pyramid-layer0 in the document.

    DimBoundsInterop dim_bounds; ///< The dimension bounds.
};

/// This structure extends on the basic statistics about an CZI-document, and includes per-scene statistics.
struct SubBlockStatisticsInteropEx
{
    std::int32_t sub_block_count; ///< The number of sub-blocks.
    std::int32_t min_m_index; ///< The minimum M-index.
    std::int32_t max_m_index; ///< The maximum M-index.
    IntRectInterop bounding_box; ///< The bounding-box determined from all sub-blocks.
    IntRectInterop bounding_box_layer0; ///< The minimal axis-aligned-bounding box determined only from the logical coordinates of the sub-blocks on pyramid-layer0 in the document.
    DimBoundsInterop dim_bounds; ///< The dimension bounds.

    std::int32_t number_of_per_scenes_bounding_boxes; ///< The number of per-scene bounding boxes that are following here.
    BoundingBoxesInterop per_scenes_bounding_boxes[]; ///< The per-scene bounding boxes. The number of elements in this array is given by 'number_of_per_scenes_bounding_boxes'.
};

#pragma pack(pop)
