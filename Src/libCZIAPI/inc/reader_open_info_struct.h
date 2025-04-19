// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include "ObjectHandles.h"

#pragma pack(push, 4)

/// This structure gather the information needed to create a reader object.
struct ReaderOpenInfoInterop
{
    InputStreamObjectHandle streamObject; ///< The input stream object to use for opening the file.
    
    // additional options go here
};


#pragma pack(pop)
