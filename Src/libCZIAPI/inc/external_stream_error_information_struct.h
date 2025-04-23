// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "ObjectHandles.h"

#pragma pack(push, 4)

const std::int32_t kStreamErrorCode_UnspecifiedError = 1;   ///< Error code for "an unspecified error occurred".


/// This structure gives additional information about an error that occurred in the external stream.
struct ExternalStreamErrorInfoInterop
{
    std::int32_t error_code;                        ///< The error code - possible values are the constants kStreamErrorCode_XXX.
    MemoryAllocationObjectHandle error_message;     ///< The error message (zero-terminated UTF8-encoded string). This string must be allocated with 'libCZI_AllocateString'.
};

#pragma pack(pop)
