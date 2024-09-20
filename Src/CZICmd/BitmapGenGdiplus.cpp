// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BitmapGenGdiplus.h"
#include "BitmapGenNull.h"
#include "utils.h"
#include "inc_libCZI.h"

#if CZICMD_USE_GDIPLUS == 1

#include <gdiplus.h>

using namespace Gdiplus;
using namespace std;
using namespace libCZI;

class CGdiplusBitmapWrapper : public libCZI::IBitmapData
{
private:
    shared_ptr<Bitmap> bitmap;
    BitmapData bd;
public:
    CGdiplusBitmapWrapper(shared_ptr<Bitmap> bitmap) :bitmap(bitmap) {}
    virtual ~CGdiplusBitmapWrapper() = default;

    virtual libCZI::PixelType GetPixelType() const
    {
        switch (this->bitmap->GetPixelFormat())
        {
        case PixelFormat24bppRGB:
            return libCZI::PixelType::Bgr24;
        case PixelFormat16bppGrayScale:
            return libCZI::PixelType::Gray16;
        default:
            return libCZI::PixelType::Invalid;
        }
    }

    virtual libCZI::IntSize	GetSize() const
    {
        return libCZI::IntSize{ this->bitmap->GetWidth(), this->bitmap->GetHeight() };
    }

    virtual libCZI::BitmapLockInfo Lock()
    {
        auto gdiplusPxlFmt = this->bitmap->GetPixelFormat();
        libCZI::BitmapLockInfo bitmapLockInfo;
        Rect rect(0, 0, static_cast<INT>(this->bitmap->GetWidth()), static_cast<INT>(this->bitmap->GetHeight()));
        this->bitmap->LockBits(&rect, ImageLockModeRead, gdiplusPxlFmt, &this->bd);
        bitmapLockInfo.ptrData = this->bd.Scan0;
        bitmapLockInfo.ptrDataRoi = this->bd.Scan0;
        bitmapLockInfo.stride = this->bd.Stride;
        bitmapLockInfo.size = static_cast<size_t>(this->bd.Stride) * this->bd.Height;
        return bitmapLockInfo;
    }

    virtual void Unlock()
    {
        this->bitmap->UnlockBits(&this->bd);
    }
};

/*static*/ULONG_PTR CBitmapGenGdiplus::gdiplusToken;

/*static*/void CBitmapGenGdiplus::Initialize()
{
    GdiplusStartupInput gdiplusStartupInput;
    Status st = GdiplusStartup(&CBitmapGenGdiplus::gdiplusToken, &gdiplusStartupInput, NULL);
}

/*static*/void CBitmapGenGdiplus::Shutdown()
{
    GdiplusShutdown(CBitmapGenGdiplus::gdiplusToken);
}

CBitmapGenGdiplus::CBitmapGenGdiplus() : CBitmapGenGdiplus(nullptr)
{
}

CBitmapGenGdiplus::CBitmapGenGdiplus(const IBitmapGenParameters* params) : fontheight(36)
{
    if (params != nullptr)
    {
        this->fontname = convertUtf8ToUCS2(params->GetFontFilename());
        this->fontheight = params->GetFontHeight();
    }

    if (this->fontname.empty())
    {
        this->fontname = L"Arial Narrow";
    }

    if (this->fontheight <= 0)
    {
        this->fontheight = 36;
    }
}

/*virtual*/CBitmapGenGdiplus::~CBitmapGenGdiplus()
{
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CBitmapGenGdiplus::Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info)
{
    PixelFormat pxlFmt;
    switch (pixeltype)
    {
    case PixelType::Bgr24:
    case PixelType::Gray16:
    case PixelType::Bgr48:
    case PixelType::Gray8:
        pxlFmt = PixelFormat24bppRGB;
        break;
    default:
        throw std::runtime_error("unsupported pixelformat");
    }

    shared_ptr<Bitmap> bitmap = make_shared<Bitmap>(width, height, pxlFmt);
    Graphics g(bitmap.get());
    g.Clear(Color(255, 0, 0));

    Font font(this->fontname.c_str(), static_cast<REAL>(this->fontheight), FontStyle::FontStyleBold, UnitPoint);
    SolidBrush brush(Color(0, 0, 0));

    auto text = IBitmapGen::CreateTextW(info);

    g.DrawString(
        text.c_str(),
        -1,
        &font,
        PointF(100, 300),
        &brush);

    if (pixeltype == PixelType::Gray16)
    {
        auto bw = make_shared<CNullBitmapWrapper>(PixelType::Gray16, width, height);
        Rect rect(0, 0, width, height);
        BitmapData bd;
        bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
        auto _ = finally([&] { bitmap->UnlockBits(&bd); });

        ScopedBitmapLockerSP dstBm{ bw };
        for (uint32_t y = 0; y < height; ++y)
        {
            const std::uint8_t* src = static_cast<const std::uint8_t*>(bd.Scan0) + y * static_cast<size_t>(bd.Stride);
            std::uint16_t* dst = reinterpret_cast<std::uint16_t*>(static_cast<std::uint8_t*>(dstBm.ptrDataRoi) + y * static_cast<size_t>(dstBm.stride));
            for (uint32_t x = 0; x < width; ++x)
            {
                uint8_t r = src[0];
                uint8_t g = src[1];
                uint8_t b = src[2];
                uint16_t p;
                if (r == 0 && g == 0 && b == 0)
                {
                    p = 0;
                }
                else
                {
                    p = 0xffff;
                }

                *dst = p;

                src += 3;
                ++dst;
            }
        }

        return bw;
    }
    else if (pixeltype == PixelType::Gray8)
    {
        auto bw = make_shared<CNullBitmapWrapper>(PixelType::Gray8, width, height);
        Rect rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
        BitmapData bd;
        bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
        auto _ = finally([&] { bitmap->UnlockBits(&bd); });	// execute "UnlockBits" when leaving scope

        ScopedBitmapLockerSP dstBm{ bw };
        for (uint32_t y = 0; y < height; ++y)
        {
            const std::uint8_t* src = static_cast<const std::uint8_t*>(bd.Scan0) + y * static_cast<size_t>(bd.Stride);
            std::uint8_t* dst = static_cast<std::uint8_t*>(dstBm.ptrDataRoi) + y * static_cast<size_t>(dstBm.stride);
            for (uint32_t x = 0; x < width; ++x)
            {
                uint8_t r = src[0];
                uint8_t g = src[1];
                uint8_t b = src[2];
                uint8_t p;
                if (r == 0 && g == 0 && b == 0)
                {
                    p = 0;
                }
                else
                {
                    p = 0xff;
                }

                *dst = p;

                src += 3;
                ++dst;
            }
        }

        return bw;
    }
    else if (pixeltype == PixelType::Bgr48)
    {
        auto bw = make_shared<CNullBitmapWrapper>(PixelType::Bgr48, width, height);
        Rect rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
        BitmapData bd;
        bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bd);
        auto _ = finally([&] { bitmap->UnlockBits(&bd); });

        ScopedBitmapLockerSP dstBm{ bw };
        for (uint32_t y = 0; y < height; ++y)
        {
            const std::uint8_t* src = static_cast<const std::uint8_t*>(bd.Scan0) + y * static_cast<size_t>(bd.Stride);
            std::uint16_t* dst = reinterpret_cast<std::uint16_t*>(static_cast<std::uint8_t*>(dstBm.ptrDataRoi) + static_cast<size_t>(y) * dstBm.stride);
            for (uint32_t x = 0; x < width; ++x)
            {
                uint8_t r = src[0];
                uint8_t g = src[1];
                uint8_t b = src[2];
                uint16_t p;
                if (r == 0 && g == 0 && b == 0)
                {
                    p = 0;
                }
                else
                {
                    p = 0xffff;
                }

                *dst = p;
                *(dst + 1) = p;
                *(dst + 2) = p;

                src += 3;
                dst += 3;
            }
        }

        return bw;
    }

    return make_shared<CGdiplusBitmapWrapper>(bitmap);
}

/*static*/void CBitmapGenGdiplus::ConvertRgb24ToBgr24(std::uint32_t w, std::uint32_t h, std::uint32_t stride, void* ptrData)
{
    for (std::uint32_t y = 0; y < h; y++)
    {
        std::uint8_t* p = static_cast<std::uint8_t*>(ptrData) + y * static_cast<size_t>(stride);
        for (std::uint32_t x = 0; x < w; x++)
        {
            std::swap(*p, *(p + 2));
            p += 3;
        }
    }
}

#endif
