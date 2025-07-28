// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_CZIcmd_Config.h"
#include "IBitmapGen.h"

#if CZICMD_USE_GDIPLUS == 1
#include <windows.h>

class CBitmapGenGdiplus :public IBitmapGen
{
private:
    static ULONG_PTR gdiplusToken;

    std::wstring fontname;
    int fontheight;
public:
    static void Initialize();
    static void Shutdown();

    CBitmapGenGdiplus();
    CBitmapGenGdiplus(const IBitmapGenParameters* params);
    virtual ~CBitmapGenGdiplus();
    virtual std::shared_ptr<libCZI::IBitmapData> Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info);
private:
    static void ConvertRgb24ToBgr24(std::uint32_t w, std::uint32_t h, std::uint32_t stride, void* ptrData);
};

#endif
