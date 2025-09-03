// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_Pixels.h"

#include "bitmapData.h"
#include "BitmapOperationsBitonal.h"

using namespace libCZI;
using namespace std;

bool BitonalBitmapOperations::GetPixelValue(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y)
{
    return BitmapOperationsBitonal::GetPixelFromBitonal(x, y, extent.w, extent.h, lockInfo.ptrData, lockInfo.stride);
}

void BitonalBitmapOperations::SetPixelValue(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y, bool value)
{
    BitmapOperationsBitonal::SetPixelInBitonal(x, y, extent.w, extent.h, lockInfo.ptrData, lockInfo.stride, value);
}

void BitonalBitmapOperations::CopyAt(libCZI::IBitmapData* source_bitmap, libCZI::IBitonalBitmapData* mask, const libCZI::IntPoint& offset, libCZI::IBitmapData* destination_bitmap)
{
    ScopedBitmapLockerP source_locker{ source_bitmap };
    ScopedBitmapLockerP destination_locker{ destination_bitmap };
    if (mask)
    {
        ScopedBitonalBitmapLockerP mask_locker{ mask };
        BitmapOperationsBitonal::CopyWithOffsetAndMaskInfo info;
        info.xOffset = offset.x;
        info.yOffset = offset.y;
        info.srcPixelType = source_bitmap->GetPixelType();
        info.srcPtr = source_locker.ptrDataRoi;
        info.srcStride = source_locker.stride;
        info.srcWidth = source_bitmap->GetWidth();
        info.srcHeight = source_bitmap->GetHeight();
        info.dstPixelType = destination_bitmap->GetPixelType();
        info.dstPtr = destination_locker.ptrDataRoi;
        info.dstStride = destination_locker.stride;
        info.dstWidth = destination_bitmap->GetWidth();
        info.dstHeight = destination_bitmap->GetHeight();
        info.drawTileBorder = false;
        info.maskPtr = mask_locker.ptrData;
        info.maskStride = mask_locker.stride;
        info.maskWidth = mask->GetWidth();
        info.maskHeight = mask->GetHeight();
        BitmapOperationsBitonal::CopyWithOffsetAndMask(info);
    }
    else
    {
        CBitmapOperations::CopyWithOffsetInfo info;
        info.xOffset = offset.x;
        info.yOffset = offset.y;
        info.srcPixelType = source_bitmap->GetPixelType();
        info.srcPtr = source_locker.ptrDataRoi;
        info.srcStride = source_locker.stride;
        info.srcWidth = source_bitmap->GetWidth();
        info.srcHeight = source_bitmap->GetHeight();
        info.dstPixelType = destination_bitmap->GetPixelType();
        info.dstPtr = destination_locker.ptrDataRoi;
        info.dstStride = destination_locker.stride;
        info.dstWidth = destination_bitmap->GetWidth();
        info.dstHeight = destination_bitmap->GetHeight();
        info.drawTileBorder = false;
        CBitmapOperations::CopyWithOffset(info);
    }
}

/*static*/void BitonalBitmapOperations::SetAllPixels(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent, bool value)
{
    BitmapOperationsBitonal::Set(extent.w, extent.h,lockInfo.ptrData, lockInfo.stride, value);
}

/*static*/void BitonalBitmapOperations::Fill(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent, const libCZI::IntRect& roi, bool value)
{
    BitmapOperationsBitonal::Fill(extent.w, extent.h, lockInfo.ptrData, lockInfo.stride, roi, value);
}

/*static*/shared_ptr<IBitonalBitmapData> BitonalBitmapOperations::Decimate(int neighborhood_size, const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent)
{
    auto destination = CStdBitonalBitmapData::Create(extent.w / 2, extent.h / 2);
    ScopedBitonalBitmapLockerSP destination_locker{ destination};
    BitmapOperationsBitonal::BitonalDecimate(
        neighborhood_size, 
        static_cast<const uint8_t*>(lockInfo.ptrData), 
        lockInfo.stride, 
        extent.w,
        extent.h,
        static_cast<uint8_t*>(destination_locker.ptrData),
        destination_locker.stride,
        destination->GetWidth(),
        destination->GetHeight());
    return destination;
}
