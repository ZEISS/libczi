// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

/// This structure contains the XML-metadata. 
/// Note that the data in this structure must be freed by the caller using 'libCZI_Free'.
#pragma pack(push, 4)
struct MetadataAsXmlInterop
{
    void* data;             ///< The XML-metadata. The data is an UTF8-encoded string. Note that this string is NOT guaranteed to be null-terminated.
                            ///< This is a pointer to the data. This data must be freed by the caller (using libCZI_Free).
    std::uint64_t size;     ///< The size of the XML-metadata in bytes.
};
#pragma pack(pop)
