// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#if  defined(_WIN32)||defined(_WIN64)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <Windows.h>
#include <Objbase.h>
#else
#include "priv_guiddef.h"
#endif



// TODO: reference additional headers your program requires here

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <sstream>