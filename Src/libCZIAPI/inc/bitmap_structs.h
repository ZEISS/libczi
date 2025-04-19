// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// Information about the bitmap represented by a bitmap-object.
struct BitmapInfoInterop
{
    std::uint32_t width;    ///< The width of the bitmap in pixels.
    std::uint32_t height;   ///< The height of the bitmap in pixels.
    std::int32_t pixelType; ///< The pixel type of the bitmap.
};

/// This structure contains information about a locked bitmap-object, allowing direct
/// access to the pixel data.
struct BitmapLockInfoInterop
{
    void* ptrData;              ///< Not currently used, to be ignored.
    void* ptrDataRoi;           ///< The pointer to the first (top-left) pixel of the bitmap.
    std::uint32_t stride;       ///< The stride of the bitmap data (pointed to by `ptrDataRoi`).
    std::uint64_t size;         ///< The size of the bitmap data (pointed to by `ptrDataRoi`) in bytes.
};

#pragma pack(pop)
