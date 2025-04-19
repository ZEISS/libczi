// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This structure contains the information about file-header.
struct FileHeaderInfoInterop
{
    std::uint8_t guid[16];  ///< The file-GUID of the CZI. Note: CZI defines two GUIDs, this is the "FileGuid". Multi-file containers 
                            ///< (for which the other GUID "PrimaryFileGuid" is used) are not supported by libCZI currently.
    int majorVersion;       ///< The major version.
    int minorVersion;       ///< The minor version.
};

#pragma pack(pop)
