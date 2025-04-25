// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <libCZI.h>

class SimpleBitmap : public libCZI::IBitmapData
{
private:
    void* ptrData;
    libCZI::PixelType pixeltype;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t stride;
public:
    SimpleBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height)
        : pixeltype(pixeltype), width(width), height(height)
    {
        const int bytesPerPel = libCZI::Utils::GetBytesPerPixel(pixeltype);
        this->stride = ((width * bytesPerPel + 3) / 4) * 4;
        size_t s = static_cast<size_t>(this->stride) * height;
        this->ptrData = malloc(s);
    }

    ~SimpleBitmap() override
    {
        free(this->ptrData);
    }

    libCZI::PixelType GetPixelType() const override
    {
        return this->pixeltype;
    }

    libCZI::IntSize	GetSize() const override
    {
        return libCZI::IntSize{ this->width, this->height };
    }

    libCZI::BitmapLockInfo	Lock() override
    {
        libCZI::BitmapLockInfo bitmapLockInfo;
        bitmapLockInfo.ptrData = this->ptrData;
        bitmapLockInfo.ptrDataRoi = this->ptrData;
        bitmapLockInfo.stride = this->stride;
        bitmapLockInfo.size = static_cast<uint64_t>(this->stride) * this->height;
        return bitmapLockInfo;
    }

    void Unlock() override
    {
    }

    int GetLockCount() const override
    {
        return 0;
    }
};
