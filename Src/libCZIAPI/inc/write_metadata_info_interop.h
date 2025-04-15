// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)
struct WriteMetadataInfoInterop
{
    std::uint32_t size_metadata;
    const void* metadata;
};
#pragma pack(pop)
