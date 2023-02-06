// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_CZIcmd_Config.h"

#if CZICMD_USE_FREETYPE == 1

#include "IBitmapGen.h"
#include "BitmapGenNull.h"

#include <ft2build.h>
#include FT_FREETYPE_H


class CBitmapGenFreetype :public IBitmapGen
{
private:
    static const std::uint8_t font_MonoMMM5[54468];
    static FT_Library library;

    FT_Face    face;
public:
    static void Initialize();
    static void Shutdown();

    CBitmapGenFreetype() = delete;
    CBitmapGenFreetype(const IBitmapGenParameters* params);
    virtual ~CBitmapGenFreetype();
    virtual std::shared_ptr<libCZI::IBitmapData> Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info);

private:
    void DrawString(int xPos, int yPos, const char* sz, const std::shared_ptr<CNullBitmapWrapper>& bm, const CNullBitmapWrapper::ColorSpecification& color);
};

#endif
