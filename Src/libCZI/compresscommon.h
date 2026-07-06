// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

#include "libCZI_Pixels.h"

namespace libCZI
{
    namespace detail
    {
        /// \brief Utility class collecting operations and helpers that are common across compression and decompression implementations.
        ///
        /// This class serves as a shared foundation for the various codec implementations, centralising
        /// recurring logic such as argument validation so that individual codecs do not need to duplicate it.
        class CompressionUtilities
        {
        public:
            /// Validates the source bitmap arguments and throws if any of them are invalid.
            /// The following conditions are checked: width and height must be greater than zero,
            /// stride must be large enough to hold a full row of pixels for the given pixel type,
            /// and the source pointer must not be null.
            ///
            /// \exception std::invalid_argument Thrown if any argument fails the validation checks.
            ///
            /// \param source_width     The width of the source bitmap in pixels.
            /// \param source_height    The height of the source bitmap in pixels.
            /// \param source_stride    The stride (row size in bytes) of the source bitmap.
            /// \param source_pixeltype The pixel type of the source bitmap.
            /// \param source           Pointer to the source bitmap data; must not be null.
            static void CheckSourceBitmapArgumentsAndThrow(std::uint32_t source_width, std::uint32_t source_height, std::uint32_t source_stride, libCZI::PixelType source_pixeltype, const void* source);

            /// Validates the destination buffer arguments and throws if any of them are invalid.
            /// The destination pointer must not be null, and the provided buffer size must be at least
            /// as large as the required minimum size.
            ///
            /// \exception std::invalid_argument Thrown if any argument fails the validation checks.
            ///
            /// \param destination             Pointer to the destination buffer; must not be null.
            /// \param size_destination        The size of the destination buffer in bytes.
            /// \param min_size_of_destination The minimum required size of the destination buffer in bytes.
            static void CheckDestinationArgumentsAndThrow(const void* destination, size_t size_destination, size_t min_size_of_destination);

            /// Validates the temporary buffer allocation function arguments and throws if any of them are invalid.
            /// Both the allocate and free function objects must be non-empty (i.e. they must wrap a callable).
            ///
            /// \exception std::invalid_argument Thrown if either function object is empty.
            ///
            /// \param allocate_temp_buffer Function used to allocate a temporary buffer of a given size in bytes.
            /// \param free_temp_buffer     Function used to free a temporary buffer previously allocated by \p allocate_temp_buffer.
            static void CheckTempBufferAllocArgumentsAndThrow(const std::function<void* (size_t)>& allocate_temp_buffer, const std::function<void(void*)>& free_temp_buffer);
        };
    }
}
