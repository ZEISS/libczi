// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This structure is used to pass the metadata information to libCZIAPI.
struct WriteMetadataInfoInterop
{
    std::uint32_t size_metadata;    ///< The size of the data pointed to by metadata.
    const void* metadata;           ///< Pointer to the metadata. This data is expected to be XML in UTF8-encoding.
};
#pragma pack(pop)
