// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_Pixels.h"

#include "bitmapData.h"
#include "BitmapOperationsBitonal.h"

using namespace libCZI;
using namespace std;

namespace
{
    void CheckLockInfoAndThrow(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent)
    {
        if (lockInfo.ptrData == nullptr)
        {
            throw std::invalid_argument("lockInfo.ptrData must not be null.");
        }

        if (extent.w == 0 || extent.h == 0)
        {
            throw std::invalid_argument("extent must have non-zero width and height.");
        }

        const size_t minimal_stride = (extent.w + 7) / 8;
        if (lockInfo.stride < minimal_stride)
        {
            throw std::invalid_argument("lockInfo.stride is too small for the specified stride.");
        }

        const std::uint64_t min_size = static_cast<std::uint64_t>(lockInfo.stride) * static_cast<std::uint64_t>(extent.h - 1) + minimal_stride;
        if (lockInfo.size < min_size)
        {
            throw std::invalid_argument("lockInfo.size is too small for the specified extent and stride.");
        }
    }
}

bool BitonalBitmapOperations::GetPixelValue(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y)
{
    CheckLockInfoAndThrow(lock_info, extent);
    return BitmapOperationsBitonal::GetPixelFromBitonal(x, y, extent.w, extent.h, lock_info.ptrData, lock_info.stride);
}

void BitonalBitmapOperations::SetPixelValue(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y, bool value)
{
    CheckLockInfoAndThrow(lock_info, extent);
    BitmapOperationsBitonal::SetPixelInBitonal(x, y, extent.w, extent.h, lock_info.ptrData, lock_info.stride, value);
}

void BitonalBitmapOperations::CopyAt(libCZI::IBitmapData* source_bitmap, libCZI::IBitonalBitmapData* mask, const libCZI::IntPoint& offset, libCZI::IBitmapData* destination_bitmap)
{
    if (source_bitmap == nullptr)
    {
        throw std::invalid_argument("source_bitmap must not be null.");
    }

    if (destination_bitmap == nullptr)
    {
        throw std::invalid_argument("destination_bitmap must not be null.");
    }

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

/*static*/void BitonalBitmapOperations::SetAllPixels(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, bool value)
{
    CheckLockInfoAndThrow(lock_info, extent);
    BitmapOperationsBitonal::Set(extent.w, extent.h, lock_info.ptrData, lock_info.stride, value);
}

/*static*/void BitonalBitmapOperations::Fill(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, const libCZI::IntRect& roi, bool value)
{
    CheckLockInfoAndThrow(lock_info, extent);
    BitmapOperationsBitonal::Fill(extent.w, extent.h, lock_info.ptrData, lock_info.stride, roi, value);
}

/*static*/shared_ptr<IBitonalBitmapData> BitonalBitmapOperations::Decimate(int neighborhood_size, const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent)
{
    CheckLockInfoAndThrow(lock_info, extent);
    auto destination = CStdBitonalBitmapData::Create(extent.w / 2, extent.h / 2);
    ScopedBitonalBitmapLockerSP destination_locker{ destination };
    BitmapOperationsBitonal::BitonalDecimate(
        neighborhood_size,
        static_cast<const uint8_t*>(lock_info.ptrData),
        lock_info.stride,
        extent.w,
        extent.h,
        static_cast<uint8_t*>(destination_locker.ptrData),
        destination_locker.stride,
        destination->GetWidth(),
        destination->GetHeight());
    return destination;
}
