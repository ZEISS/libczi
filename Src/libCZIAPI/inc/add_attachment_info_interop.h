// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)
struct AddAttachmentInfoInterop
{
    std::uint8_t guid[16];              ///< The GUID of the attachment.

    std::uint8_t contentFileType[8];

    std::uint8_t name[80];

    std::uint32_t size_attachment_data;
    const void* attachment_data;
};
#pragma pack(pop)
