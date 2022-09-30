// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Pixels.h"

class CSingleChannelTileCompositor
{
public:
	static void Compose(libCZI::IBitmapData* dest, libCZI::IBitmapData* source, int x, int y, bool drawTileBorder);
};
