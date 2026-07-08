// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "compresscommon.h"
#include <limits>
#include <stdexcept>
#include <sstream>

#include "libCZI_Utilities.h"

using namespace std;
using namespace libCZI;

void libCZI::detail::CompressionUtilities::CheckSourceBitmapArgumentsAndThrow(std::uint32_t source_width, std::uint32_t source_height, std::uint32_t source_stride, libCZI::PixelType source_pixeltype, const void* source)
{
    if (source_width == 0)
    {
        throw invalid_argument("width must be greater than zero");
    }

    if (source_height == 0)
    {
        throw invalid_argument("height must be greater than zero");
    }

    // note: GetBytesPerPixel will throw (an invalid_argument-exception) in case of an invalid enum value
    const size_t bytes_per_pixel = Utils::GetBytesPerPixel(source_pixeltype);
    if (static_cast<size_t>(source_width) > numeric_limits<size_t>::max() / bytes_per_pixel)
    {
        throw invalid_argument("width is too large for the given pixel type.");
    }

    const size_t min_stride = static_cast<size_t>(source_width) * bytes_per_pixel;
    if (min_stride > numeric_limits<uint32_t>::max() || static_cast<size_t>(source_stride) < min_stride)
    {
        stringstream ss;
        ss << "stride is illegal, for width=" << source_width << " and pixeltype=" << Utils::PixelTypeToInformalString(source_pixeltype) << " the minimum stride is "
            << min_stride << " whereas " << source_stride << " was specified.";
        throw invalid_argument(ss.str());
    }

    if (source == nullptr)
    {
        throw invalid_argument("source must not be null");
    }
}

void libCZI::detail::CompressionUtilities::CheckDestinationArgumentsAndThrow(const void* destination, size_t size_destination, size_t min_size_of_destination)
{
    if (destination == nullptr)
    {
        throw invalid_argument("destination must not be null.");
    }

    if (size_destination < min_size_of_destination)
    {
        stringstream ss;
        ss << "sizeDestination must be greater than or equal to " << min_size_of_destination << ", whereas " << size_destination << " was specified.";
        throw invalid_argument(ss.str());
    }
}

void libCZI::detail::CompressionUtilities::CheckTempBufferAllocArgumentsAndThrow(const std::function<void* (size_t)>& allocate_temp_buffer, const std::function<void(void*)>& free_temp_buffer)
{
    if (!allocate_temp_buffer)
    {
        throw invalid_argument("A function for allocating temp memory must be given.");
    }

    if (!free_temp_buffer)
    {
        throw invalid_argument("A function for freeing temp memory must be given.");
    }
}
