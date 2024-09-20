// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "IBitmapGen.h"
#include "BitmapGenGdiplus.h"
#include "BitmapGenNull.h"
#include "BitmapGenFreeType.h"
#include "utils.h"
#include "inc_libCZI.h"
#include "inc_CZIcmd_Config.h"

using namespace std;
using namespace libCZI;

/*static*/void BitmapGenFactory::InitializeFactory()
{
#if CZICMD_USE_GDIPLUS == 1
    CBitmapGenGdiplus::Initialize();
#endif
#if CZICMD_USE_FREETYPE == 1
    CBitmapGenFreetype::Initialize();
#endif
}

/*static*/void BitmapGenFactory::Shutdown()
{
#if CZICMD_USE_GDIPLUS == 1
    CBitmapGenGdiplus::Shutdown();
#endif
#if CZICMD_USE_FREETYPE == 1
    CBitmapGenFreetype::Shutdown();
#endif
}

/*static*/std::string BitmapGenFactory::GetDefaultBitmapGeneratorClassName()
{
#if CZICMD_USE_GDIPLUS == 1
    return "gdi";
#else
#if CZICMD_USE_FREETYPE == 1
    return "freetype";
#else
    return "null";
#endif
#endif
}

/*static*/std::shared_ptr<IBitmapGen> BitmapGenFactory::CreateDefaultBitmapGenerator(const IBitmapGenParameters* params/*= nullptr*/)
{
#if CZICMD_USE_GDIPLUS == 1
    return std::make_shared<CBitmapGenGdiplus>(params);
#else
#if CZICMD_USE_FREETYPE == 1
    return std::make_shared<CBitmapGenFreetype>(params);
#else
    return std::make_shared<CBitmapGenNull>();
#endif
#endif
}

/*static*/std::shared_ptr<IBitmapGen> BitmapGenFactory::CreateBitmapGenerator(const char* className, const IBitmapGenParameters* params/*= nullptr*/)
{
    if (icasecmp(className, "null"))
    {
        return std::make_shared<CBitmapGenNull>();
    }
#if CZICMD_USE_GDIPLUS == 1
    if (icasecmp(className, "gdi"))
    {
        return std::make_shared<CBitmapGenGdiplus>();
    }
#endif
#if CZICMD_USE_FREETYPE == 1
    if (icasecmp(className, "freetype"))
    {
        return std::make_shared<CBitmapGenFreetype>(params);
    }
#endif
    if (icasecmp(className, "default"))
    {
        return CreateDefaultBitmapGenerator(params);
    }

    return nullptr;
}

/*static*/void BitmapGenFactory::EnumBitmapGenerator(const std::function<bool(int no, std::tuple<std::string, std::string, bool>)>& enumGenerators)
{
    static const struct
    {
        std::string className;
        std::string explanation;
    } BitmapGenerators[] =
    {
        { "null", "creating just black images" },
#if CZICMD_USE_GDIPLUS == 1
        { "gdi", "based on GDI+, provides text-rendering" },
#endif
#if CZICMD_USE_FREETYPE == 1
        { "freetype", "based on the Freetype-library, provides text-rendering" },
#endif
    };

    const auto defaultClassName = BitmapGenFactory::GetDefaultBitmapGeneratorClassName();
    for (int i = 0; i < static_cast<int>(sizeof(BitmapGenerators) / sizeof(BitmapGenerators[0])); ++i)
    {
        bool isDefault = BitmapGenerators[i].className == defaultClassName;
        if (!enumGenerators(i, make_tuple(BitmapGenerators[i].className, BitmapGenerators[i].explanation, isDefault)))
        {
            break;
        }
    }
}

//-----------------------------------------------------------------------------

/*static*/std::string IBitmapGen::CreateTextA(const BitmapGenInfo& info)
{
    return convertToUtf8(IBitmapGen::CreateTextW(info));
}

/*static*/std::wstring IBitmapGen::CreateTextW(const BitmapGenInfo& info)
{
    wstringstream ss;
    ss << L"COORD";
    if (info.coord != nullptr)
    {
        ss << L": " << convertUtf8ToUCS2(Utils::DimCoordinateToString(info.coord));
    }
    else
    {
        ss << L": <unspecified>";
    }

    if (info.mValid)
    {
        ss << L" M=" << info.mIndex;
    }

    ss << L"  X=" << get<0>(info.tilePixelPosition) << " Y=" << get<1>(info.tilePixelPosition);
    return ss.str();
}

