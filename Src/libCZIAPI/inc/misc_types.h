// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

// those constants are used in the DimBoundsInterop struct to indicate a dimension

const std::uint32_t kDimensionInvalid = 0;  ///< Invalid dimension index.

const std::uint32_t kDimensionZ = 1;        ///< The Z-dimension.
const std::uint32_t kDimensionC = 2;        ///< The C-dimension ("channel").
const std::uint32_t kDimensionT = 3;        ///< The T-dimension ("time").
const std::uint32_t kDimensionR = 4;        ///< The R-dimension ("rotation").
const std::uint32_t kDimensionS = 5;        ///< The S-dimension ("scene").
const std::uint32_t kDimensionI = 6;        ///< The I-dimension ("illumination").
const std::uint32_t kDimensionH = 7;        ///< The H-dimension ("phase").
const std::uint32_t kDimensionV = 8;        ///< The V-dimension ("view").
const std::uint32_t kDimensionB = 9;        ///< The B-dimension ("block") - its use is deprecated.

const std::uint32_t kDimensionMinValue = 1; ///< This enum must have the value of the lowest (valid) dimension index.
const std::uint32_t kDimensionMaxValue = 9; ///< This enum must have the value of the highest (valid) dimension index.

const std::uint8_t kMaxDimensionCount = kDimensionMaxValue - kDimensionMinValue + 1;

#pragma pack(push, 4)

/// This structure describes a rectangle, given by its top-left corner and its width and height.
struct IntRectInterop
{
    std::int32_t x; ///< The x-coordinate of the top-left corner.
    std::int32_t y; ///< The y-coordinate of the top-left corner.
    std::int32_t w; ///< The width of the rectangle.
    std::int32_t h; ///< The height of the rectangle.
};

/// This structure describes a size, given by its width and height.
struct IntSizeInterop
{
    std::int32_t w; ///< The width.
    std::int32_t h; ///< The height.
};

/// This structure gives the bounds for a set of dimensions.
/// The bit at position `i` in `dimensions_valid` indicates whether the interval for dimension `i+1` is valid. So, bit 0
/// is corresponding to dimension 1 (=Z), bit 1 to dimension 2 (=C), and so on.
/// In the fixed-sized arrays `start` and `size`, the start and size values for the dimensions are stored. The elements at
/// position 0 corresponds to the first valid dimension, the element at position 1 to the second valid dimension, and so on.
/// An example would be: `dimensions_valid` = 0b00000011, `start` = { 0, 2 }, `size` = { 5, 6 }. This would mean that the
/// dimension 'Z' is valid, and the interval is [0, 5], and the dimension 'C' is valid, and the interval is [2, 8].
struct DimBoundsInterop
{
    std::uint32_t dimensions_valid; ///< Bitfield indicating which dimensions are valid. Bit-position `i` corresponds to dimension `i+1`.
    std::int32_t start[kMaxDimensionCount]; ///< The start values, the element 0 corresponds the first set flag in dimensions_valid and so on.
    std::int32_t size[kMaxDimensionCount]; ///< The size values, the element 0 corresponds the first set flag in dimensions_valid and so on.
};

/// This structure gives the coordinates (of a sub-block) for a set of dimension.
/// The bit at position `i` in `dimensions_valid` indicates whether the coordinate for dimension `i+1` is valid. So, bit 0
/// is corresponding to dimension 1 (=Z), bit 1 to dimension 2 (=C), and so on.
/// In the fixed-sized array `value`, the coordinate for the dimensions is stored. The element at
/// position 0 corresponds to the first valid dimension, the element at position 1 to the second valid dimension, and so on.
/// An example would be: `dimensions_valid` = 0b00000011, `value` = { 0, 2 }. This would mean that the
/// dimension 'Z' is valid, and the coordinate for 'Z' is 0, and the dimension 'C' is valid, and the coordinate for 'C' is 2.
struct CoordinateInterop
{
    std::uint32_t dimensions_valid; ///< Bitfield indicating which dimensions are valid. Bit-position `i` corresponds to dimension `i+1`.
    std::int32_t value[kMaxDimensionCount]; ///< The coordinate values, the element 0 corresponds the first set flag in dimensions_valid and so on.
};

/// This structure contains the bounding boxes for a scene.
struct BoundingBoxesInterop
{
    /// Zero-based index of the scene (for which the following bounding boxes apply).
    std::int32_t sceneIndex;
    
    /// The bounding box of the scene (calculated including pyramid-tiles).
    IntRectInterop bounding_box;

    /// The bounding box of the scene (calculated excluding pyramid-tiles).
    IntRectInterop bounding_box_layer0_only;
};

#pragma pack(pop)
