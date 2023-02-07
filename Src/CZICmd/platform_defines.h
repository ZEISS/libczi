// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#if defined(_WIN32)
// this means that the Win32-API is available, and the Windows-SDK is available
#define WIN32ENV (1)
#endif

#if defined(__GNUC__)
#define LINUXENV (1)
#endif
