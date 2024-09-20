// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "IBitmapGen.h"

class CNullBitmapWrapper : public libCZI::IBitmapData
{
public:
    union ColorSpecification
    {
        struct
        {
            std::uint8_t value;
        } Gray8;

        struct
        {
            std::uint8_t r; ///< The red component.
            std::uint8_t g; ///< The green component.
            std::uint8_t b; ///< The blue component.
        } Bgr24;

        struct
        {
            std::uint16_t value;
        } Gray16;

        struct
        {
            std::uint16_t r; ///< The red component.
            std::uint16_t g; ///< The green component.
            std::uint16_t b; ///< The blue component.
        } Bgr48;
    };
private:
    void* ptrData;
    libCZI::PixelType pixeltype;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t stride;
public:
    CNullBitmapWrapper(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height);
    virtual ~CNullBitmapWrapper();
    virtual libCZI::PixelType GetPixelType() const;
    virtual libCZI::IntSize GetSize() const;
    virtual libCZI::BitmapLockInfo  Lock();
    virtual void Unlock();

    void Clear();
    void Clear(const ColorSpecification& color);
    void CopyMonochromeBitmap(int posX, int posY, const void* ptrData, int stride, int width, int height, const ColorSpecification& color);
private:
    template<typename t, int tBytesPerPel>
    void InternalCopyMonochromeBitmap(int posX, int posY, const void* ptrData, int stride, int width, int height, t& setPixel)
    {
        // TODO: posX/posY must be positive for this code to work correctly
        for (int y = posY; y < posY + height; ++y)
        {
            if (y >= static_cast<int>(this->height))
            {
                break;
            }

            const std::uint8_t* ptr = static_cast<const std::uint8_t*>(ptrData) + static_cast<size_t>(y - posY) * stride;
            std::uint8_t* ptrDst = static_cast<std::uint8_t*>(this->ptrData) + static_cast<size_t>(y) * this->stride + tBytesPerPel * static_cast<size_t>(posX);
            int v = 0x80;
            for (int x = posX; x < posX + width; ++x)
            {
                if (x >= static_cast<int>(this->width))
                {
                    break;
                }

                const bool pixel = (*ptr & v);
                if (pixel)
                {
                    setPixel.setPixel(ptrDst);
                }

                v >>= 1;
                if (v == 0)
                {
                    v = 0x80;
                    ++ptr;
                }

                ptrDst += tBytesPerPel;
            }
        }
    }
};

class CBitmapGenNull :public IBitmapGen
{
public:
    CBitmapGenNull();
    virtual ~CBitmapGenNull();
    virtual std::shared_ptr<libCZI::IBitmapData> Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info);
};
