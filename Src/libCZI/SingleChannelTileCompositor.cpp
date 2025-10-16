// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI.h"
#include "SingleChannelTileCompositor.h"
#include "BitmapOperations.h"
#include "BitmapOperationsBitonal.h"

using namespace libCZI;
using namespace libCZI::detail;

/*static*/void CSingleChannelTileCompositor::Compose(libCZI::IBitmapData* dest, libCZI::IBitmapData* source, int x, int y, bool drawTileBorder)
{
    const ScopedBitmapLockerP source_locker{ source };
    const ScopedBitmapLockerP destination_locker{ dest };
    CBitmapOperations::CopyWithOffsetInfo info;
    info.xOffset = x;
    info.yOffset = y;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locker.ptrDataRoi;
    info.srcStride = source_locker.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();

    info.dstPixelType = dest->GetPixelType();
    info.dstPtr = destination_locker.ptrDataRoi;
    info.dstStride = destination_locker.stride;
    info.dstWidth = dest->GetWidth();
    info.dstHeight = dest->GetHeight();

    info.drawTileBorder = drawTileBorder;

    CBitmapOperations::CopyWithOffset(info);
}

/*static*/void CSingleChannelTileCompositor::ComposeMaskAware(libCZI::IBitmapData* dest, libCZI::IBitmapData* source, libCZI::IBitonalBitmapData* sourceMask, int x, int y, bool drawTileBorder)
{
    if (sourceMask == nullptr)
    {
        // No mask - just do a normal compose
        CSingleChannelTileCompositor::Compose(dest, source, x, y, drawTileBorder);
        return;
    }

    const ScopedBitmapLockerP source_locker{ source };
    const ScopedBitmapLockerP destination_locker{ dest };
    const ScopedBitonalBitmapLockerP mask_locker{ sourceMask };
    BitmapOperationsBitonal::CopyWithOffsetAndMaskInfo info;
    info.xOffset = x;
    info.yOffset = y;
    info.srcPixelType = source->GetPixelType();
    info.srcPtr = source_locker.ptrDataRoi;
    info.srcStride = source_locker.stride;
    info.srcWidth = source->GetWidth();
    info.srcHeight = source->GetHeight();

    info.dstPixelType = dest->GetPixelType();
    info.dstPtr = destination_locker.ptrDataRoi;
    info.dstStride = destination_locker.stride;
    info.dstWidth = dest->GetWidth();
    info.dstHeight = dest->GetHeight();

    info.drawTileBorder = drawTileBorder;

    info.maskPtr = mask_locker.ptrData;
    info.maskStride = mask_locker.stride;
    info.maskWidth = sourceMask->GetWidth();
    info.maskHeight = sourceMask->GetHeight();

    BitmapOperationsBitonal::CopyWithOffsetAndMask(info);
}

/*-----------------------------------------------------------------------------------------------*/

/*static*/void libCZI::Compositors::ComposeSingleChannelTiles(
    const std::function<bool(int, std::shared_ptr<libCZI::IBitmapData>&, int&, int&)>& getTiles,
    libCZI::IBitmapData* dest,
    int xPos,
    int yPos,
    const ComposeSingleTileOptions* pOptions)
{
    if (pOptions == nullptr)
    {
        ComposeSingleTileOptions options;
        options.Clear();
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

/*static*/void libCZI::Compositors::ComposeSingleChannelTilesMaskAware(
    const std::function<bool(int, std::shared_ptr<libCZI::IBitmapData>&, std::shared_ptr<libCZI::IBitonalBitmapData>&, int&, int&)>& getTilesAndMask,
    libCZI::IBitmapData* dest,
    int xPos,
    int yPos,
    const ComposeSingleTileOptions* pOptions)
{
    if (pOptions == nullptr)
    {
        ComposeSingleTileOptions options;
        options.Clear();
        ComposeSingleChannelTilesMaskAware(getTilesAndMask, dest, xPos, yPos, &options);
        return;
    }

    for (int i = 0;; ++i)
    {
        int posXTile, posYTile;
        std::shared_ptr<libCZI::IBitmapData> src;
        std::shared_ptr<IBitonalBitmapData> src_mask;
        const bool b = getTilesAndMask(i, src, src_mask, posXTile, posYTile);
        if (b != true)
        {
            break;
        }

        // TODO: check return values?
        CSingleChannelTileCompositor::ComposeMaskAware(dest, src.get(), src_mask.get(), posXTile - xPos, posYTile - yPos, pOptions->drawTileBorder);
    }
}
