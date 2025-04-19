// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This structure contains the information about an attachment.
/// Note that performance reasons we use a fixed-size array for the name. In the rare case that the name is too long to fit into the
/// fixed-size array, the 'overflow' field is set to true. In this case, the name is truncated and the 'overflow' field is set to true.
/// In addition, the field 'name_in_case_of_overflow' then contains the full text, allocated with 'libCZI_AllocateString' (and responsibility
/// for releasing the memory is with the caller).
struct AttachmentInfoInterop
{
    std::uint8_t guid[16];              ///< The GUID of the attachment.
    std::uint8_t content_file_type[9];  ///< A null-terminated character array identifying the content of the attachment.
    char name[255];                     ///< A zero-terminated string (in UTF8-encoding) identifying the content of the attachment.
    bool name_overflow;                 ///< True if the name is too long to fit into the 'name' field.
    void* name_in_case_of_overflow;     ///< If 'name_overflow' is true, then this field contains the name (in UTF8-encoding and zero terminated) of the attachment. This memory must be freed using 'libCZI_Free'.
};

#pragma pack(pop)
