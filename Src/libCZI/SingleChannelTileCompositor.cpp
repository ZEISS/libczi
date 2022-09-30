// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "libCZI.h"
#include "SingleChannelTileCompositor.h"
#include "BitmapOperations.h"

using namespace libCZI;

/*static*/void CSingleChannelTileCompositor::Compose(libCZI::IBitmapData* dest, libCZI::IBitmapData* source, int x, int y, bool drawTileBorder)
{
	ScopedBitmapLockerP srcLck{ source };
	ScopedBitmapLockerP dstLck{ dest };
	CBitmapOperations::CopyOffsetedInfo info;
	info.xOffset = x;
	info.yOffset = y;
	info.srcPixelType = source->GetPixelType();
	info.srcPtr = srcLck.ptrDataRoi;
	info.srcStride = srcLck.stride;
	info.srcWidth = source->GetWidth();
	info.srcHeight = source->GetHeight();

	info.dstPixelType = dest->GetPixelType();
	info.dstPtr = dstLck.ptrDataRoi;
	info.dstStride = dstLck.stride;
	info.dstWidth = dest->GetWidth();
	info.dstHeight = dest->GetHeight();

	info.drawTileBorder = drawTileBorder;

	CBitmapOperations::CopyOffseted(info);
}

/*-----------------------------------------------------------------------------------------------*/

/*static*/void libCZI::Compositors::ComposeSingleChannelTiles(
	std::function<bool(int, std::shared_ptr<libCZI::IBitmapData>&, int&, int&)> getTiles,
	libCZI::IBitmapData* dest,
	int xPos,
	int yPos,
	const ComposeSingleTileOptions* pOptions)
{
	if (pOptions == nullptr)
	{
		ComposeSingleTileOptions options; options.Clear();
		ComposeSingleChannelTiles(getTiles, dest, xPos, yPos, &options);
		return;
	}

	for (int i = 0;; ++i)
	{
		int posXTile, posYTile;
		std::shared_ptr<libCZI::IBitmapData> src;
        const bool b = getTiles(i, src, posXTile, posYTile);
		if (b != true)
		{
			break;
		}

		// TODO: check return values?
		CSingleChannelTileCompositor::Compose(dest, src.get(), posXTile - xPos, posYTile - yPos, pOptions->drawTileBorder);
	}
}
