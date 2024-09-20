// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BitmapGenNull.h"
#include "utils.h"
#include "inc_libCZI.h"

CNullBitmapWrapper::CNullBitmapWrapper(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height) :pixeltype(pixeltype), width(width), height(height)
{
    std::uint8_t bytesPerPel;
    switch (pixeltype)
    {
    case libCZI::PixelType::Gray8:bytesPerPel = 1; break;
    case libCZI::PixelType::Gray16:bytesPerPel = 2; break;
    case libCZI::PixelType::Gray32Float:bytesPerPel = 4; break;
    case libCZI::PixelType::Bgr24:bytesPerPel = 3; break;
    case libCZI::PixelType::Bgr48:bytesPerPel = 6; break;
    default: throw std::runtime_error("not implemented");
    }

    this->stride = width * bytesPerPel;
    size_t s = static_cast<size_t>(this->stride) * height;
    this->ptrData = malloc(s);
}

void CNullBitmapWrapper::Clear()
{
    memset(this->ptrData, 0, static_cast<size_t>(this->stride) * height);
}

void CNullBitmapWrapper::Clear(const ColorSpecification& color)
{
    switch (this->pixeltype)
    {
    case libCZI::PixelType::Gray8:
        for (std::uint32_t y = 0; y < this->height; ++y)
        {
            memset(static_cast<std::uint8_t*>(this->ptrData) + static_cast<size_t>(y) * this->stride, color.Gray8.value, this->width);
        }

        break;
    case libCZI::PixelType::Bgr24:
        for (std::uint32_t y = 0; y < this->height; ++y)
        {
            std::uint8_t* ptr = static_cast<std::uint8_t*>(this->ptrData) + static_cast<size_t>(y) * this->stride;
            for (std::uint32_t x = 0; x < this->height; ++x)
            {
                *ptr++ = color.Bgr24.b;
                *ptr++ = color.Bgr24.g;
                *ptr++ = color.Bgr24.r;
            }
        }

        break;
    case libCZI::PixelType::Gray16:
        for (std::uint32_t y = 0; y < this->height; ++y)
        {
            std::uint16_t* ptr = reinterpret_cast<std::uint16_t*>(static_cast<std::uint8_t*>(this->ptrData) + static_cast<size_t>(y) * this->stride);
            for (std::uint32_t x = 0; x < this->height; ++x)
            {
                *ptr++ = color.Gray16.value;
            }
        }

        break;
    case libCZI::PixelType::Bgr48:
        for (std::uint32_t y = 0; y < this->height; ++y)
        {
            std::uint16_t* ptr = static_cast<std::uint16_t*>(this->ptrData) + static_cast<size_t>(y) * this->stride;
            for (std::uint32_t x = 0; x < this->height; ++x)
            {
                *ptr++ = color.Bgr48.b;
                *ptr++ = color.Bgr48.g;
                *ptr++ = color.Bgr48.r;
            }
        }

        break;
    case libCZI::PixelType::Gray32Float:
    default: throw std::runtime_error("not implemented");
    }
}

/*virtual*/CNullBitmapWrapper::~CNullBitmapWrapper()
{
    free(this->ptrData);
}

/*virtual*/libCZI::PixelType CNullBitmapWrapper::GetPixelType() const
{
    return this->pixeltype;
}

/*virtual*/libCZI::IntSize  CNullBitmapWrapper::GetSize() const
{
    return libCZI::IntSize{ this->width, this->height };
}

/*virtual*/libCZI::BitmapLockInfo CNullBitmapWrapper::Lock()
{
    libCZI::BitmapLockInfo bitmapLockInfo;
    bitmapLockInfo.ptrData = this->ptrData;
    bitmapLockInfo.ptrDataRoi = this->ptrData;
    bitmapLockInfo.stride = this->stride;
    bitmapLockInfo.size = static_cast<uint64_t>(this->stride) * this->height;
    return bitmapLockInfo;
}

/*virtual*/void CNullBitmapWrapper::Unlock()
{
}

void CNullBitmapWrapper::CopyMonochromeBitmap(int posX, int posY, const void* ptrData, int stride, int width, int height, const ColorSpecification& color)
{
    switch (this->pixeltype)
    {
    case libCZI::PixelType::Bgr24:
    {
        struct SetPixelBgr24
        {
        private:
            const ColorSpecification& color;
        public:
            SetPixelBgr24(const ColorSpecification& color) :color(color) {}
            void setPixel(std::uint8_t* ptr)
            {
                *(ptr + 0) = this->color.Bgr24.b;
                *(ptr + 1) = this->color.Bgr24.g;
                *(ptr + 2) = this->color.Bgr24.r;
            }
        } setPixelBgr24(color);

        this->InternalCopyMonochromeBitmap<SetPixelBgr24, 3>(posX, posY, ptrData, stride, width, height, setPixelBgr24);
        break;
    }
    case libCZI::PixelType::Bgr48:
    {
        struct SetPixelBgr48
        {
        private:
            const ColorSpecification& color;
        public:
            SetPixelBgr48(const ColorSpecification& color) :color(color) {}
            void setPixel(std::uint8_t* ptr)
            {
                auto ptrUshort = reinterpret_cast<uint16_t*>(ptr);
                *(ptrUshort + 0) = this->color.Bgr48.b;
                *(ptrUshort + 1) = this->color.Bgr48.g;
                *(ptrUshort + 2) = this->color.Bgr48.r;
            }
        } setPixelBgr48(color);

        this->InternalCopyMonochromeBitmap<SetPixelBgr48, 6>(posX, posY, ptrData, stride, width, height, setPixelBgr48);
        break;
    }
    case libCZI::PixelType::Gray8:
    {
        struct SetPixelGray8
        {
        private:
            const ColorSpecification& color;
        public:
            SetPixelGray8(const ColorSpecification& color) :color(color) {}
            void setPixel(std::uint8_t* ptr)
            {
                *ptr = this->color.Gray8.value;
            }
        } setPixelGray8(color);

        this->InternalCopyMonochromeBitmap<SetPixelGray8, 1>(posX, posY, ptrData, stride, width, height, setPixelGray8);
        break;
    }
    case libCZI::PixelType::Gray16:
    {
        struct SetPixelGray16
        {
        private:
            const ColorSpecification& color;
        public:
            SetPixelGray16(const ColorSpecification& color) :color(color) {}
            void setPixel(std::uint8_t* ptr)
            {
                *reinterpret_cast<uint16_t*>(ptr) = this->color.Gray16.value;
            }
        } setPixelGray16(color);

        this->InternalCopyMonochromeBitmap<SetPixelGray16, 2>(posX, posY, ptrData, stride, width, height, setPixelGray16);
        break;
    }

    default: throw std::runtime_error("not implemented");
    }
}

//------------------------------------------------------------------------------------------------

CBitmapGenNull::CBitmapGenNull()
{

}

/*virtual*/CBitmapGenNull::~CBitmapGenNull()
{

}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CBitmapGenNull::Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info)
{
    auto bm = std::make_shared<CNullBitmapWrapper>(pixeltype, width, height);
    bm->Clear();
    return bm;
}
