// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

/// This structure contains information about an input stream class.
/// Note that the strings in this structure must be freed by the caller using 'libCZI_Free'.
#pragma pack(push, 4)
struct InputStreamClassInfoInterop
{
    char* name;         ///< The name of the input stream class. This is a free-form string. This string must be freed by the caller (using libCZI_Free).
    char* description;  ///< The description of the input stream class. This is a free-form string. This string must be freed by the caller (using libCZI_Free).
};
#pragma pack(pop)
