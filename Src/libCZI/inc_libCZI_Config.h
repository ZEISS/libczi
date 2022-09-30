// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// If building with CMake, there will be a file "libCZI_Config.h" created/configured by
// CMake. In order to enable building with the .sln-file in VisualStudio we use a somewhat
// lame approach - we use some preprocessor define in order to use definitions which are
// static (not generated) here.

#if defined(VISUALSTUDIO_PROJECT_BUILD)

#define LIBCZICONFIG_STRINGIFY(s) #s

#define LIBCZI_VERSION_MAJOR 0
#define LIBCZI_VERSION_MINOR 40

#define LIBCZI_ISBIGENDIANHOST 0
#define LIBCZI_HAVE_ENDIAN_H 0

#define LIBCZI_CXX_COMPILER_IDENTIFICATION  LIBCZICONFIG_STRINGIFY(_MSC_FULL_VER)

#else

#include <libCZI_Config.h>

#endif
