// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define __STDC_WANT_LIB_EXT1__ 1

#include "platform_defines.h"

#if defined(LINUXENV)
#define _LARGEFILE64_SOURCE
#endif

#include "targetver.h"

#include <cstdio>

#include <string>
#include <memory>
#include <sstream>

// TODO: reference additional headers your program requires here
