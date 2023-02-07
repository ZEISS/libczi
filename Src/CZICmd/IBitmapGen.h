// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

struct BitmapGenInfo
{
    const libCZI::IDimCoordinate* coord;
    bool mValid;
    int mIndex;
    std::tuple<std::uint32_t, std::uint32_t> tilePixelPosition;

    void Clear()
    {
        this->coord = nullptr;
        this->mValid = false;
        this->tilePixelPosition = std::make_tuple(0, 0);
    }
};

class IBitmapGenParameters
{
public:
    /// Gets the TTF-font-filename (only used for the FreeType-Bitmap-Generator). If empty, we use the embedded TTF-font "MonoMMM5".
    ///
    /// \return The TTF-font-filename.
    virtual std::string GetFontFilename() const = 0;

    virtual int GetFontHeight() const = 0;
};

class CBitmapGenParameters :public IBitmapGenParameters
{
private:
    std::string fontFilename;
    int fontHeight = -1;
public:
    virtual std::string GetFontFilename() const { return this->fontFilename; }
    virtual int GetFontHeight() const { return this->fontHeight; }

    void SetFontFilename(const std::string& str) { this->fontFilename = str; }
    void SetFontHeight(int height) { this->fontHeight = height; }
};

class IBitmapGen
{
public:
    virtual std::shared_ptr<libCZI::IBitmapData> Create(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, const BitmapGenInfo& info) = 0;

    virtual ~IBitmapGen() = default;

    static std::string CreateTextA(const BitmapGenInfo& info);
    static std::wstring CreateTextW(const BitmapGenInfo& info);
};

class BitmapGenFactory
{
public:
    static void InitializeFactory();
    static void Shutdown();
    static std::string GetDefaultBitmapGeneratorClassName();
    static std::shared_ptr<IBitmapGen> CreateDefaultBitmapGenerator(const IBitmapGenParameters* params = nullptr);
    static std::shared_ptr<IBitmapGen> CreateBitmapGenerator(const char* className, const IBitmapGenParameters* params = nullptr);
    static void EnumBitmapGenerator(const std::function<bool(int no, std::tuple<std::string, std::string, bool>)>& enumGenerators);
};
