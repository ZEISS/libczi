// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// If building with CMake, there will be a file "libCZI_Config.h" created/configured by
// CMake. In order to enable building with the .sln-file in VisualStudio we use a somewhat
// lame approach - we use some preprocessor define in order to use definitions which are
// static (not generated) here.

#include <CZIcmd_Config.h>
