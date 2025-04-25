// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This struct contains the version information of the libCZIApi-library. For versioning libCZI, SemVer2 (<https://semver.org/>) is used.
/// Note that the value of the tweak version number does not have a meaning (as far as SemVer2 is concerned).
struct LibCZIVersionInfoInterop
{
    int32_t major;  ///< The major version number.
    int32_t minor;  ///< The minor version number.
    int32_t patch;  ///< The patch version number.
    int32_t tweak;  ///< The tweak version number.
};

/// This struct gives information about the build of the libCZIApi-library.
/// Note that all strings must be freed by the caller (using libCZI_Free).
struct LibCZIBuildInformationInterop
{
    char* compilerIdentification; ///< If non-null, the compiler identification. This is a free-form string. This string must be freed by the caller (using libCZI_Free).
    char* repositoryUrl;          ///< If non-null, the URL of the repository. This string must be freed by the caller (using libCZI_Free).
    char* repositoryBranch;       ///< If non-null, the branch of the repository. This string must be freed by the caller (using libCZI_Free).
    char* repositoryTag;          ///< If non-null, the tag of the repository. This string must be freed by the caller (using libCZI_Free).
};

#pragma pack(pop)
