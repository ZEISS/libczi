// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include "utils.h"

void Message_Box()
{
    ::MessageBoxW(nullptr, L"Hello World", L"Hello World", MB_OK);
}
#else
void Message_Box()
{
}
#endif
