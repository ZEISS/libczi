// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)
/// This structure is used to pass the attachment information to libCZIAPI, describing an attachment to be added to a CZI-file.
struct AddAttachmentInfoInterop
{
    std::uint8_t guid[16];              ///< The GUID of the attachment.

    std::uint8_t contentFileType[8];    ///< The content file type.

    std::uint8_t name[80];              ///< The name of the attachment.

    std::uint32_t size_attachment_data; ///< The size of the attachment data (in bytes).
    const void* attachment_data;        ///< Pointer to the attachment data.
};
#pragma pack(pop)
