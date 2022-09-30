// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <cwctype>
#include <functional>
#include "libCZI_Pixels.h"

class Utilities
{
public:
    static inline void Split(const std::wstring& text, wchar_t sep, const std::function<bool(const std::wstring&)>& funcToken)
    {
        std::size_t start = 0, end = 0;
        while (static_cast<std::size_t>(end = text.find(sep, start)) != static_cast<std::size_t>(std::wstring::npos))
        {
            std::wstring temp = text.substr(start, end - start);
            if (!temp.empty())
            {
                if (funcToken(temp) == false)
                    return;
            }

            start = end + 1;
        }

        const std::wstring temp = text.substr(start);
        if (!temp.empty())
        {
            funcToken(temp);
        }
    }

    static inline void RemoveSpaces(std::wstring& str)
    {
        str.erase(std::remove_if(str.begin(), str.end(), std::iswspace), str.end());
    }

    static inline  libCZI::IntRect Intersect(const libCZI::IntRect& a, const libCZI::IntRect& b)
    {
        const int x1 = (std::max)(a.x, b.x);
        const int x2 = (std::min)(a.x + a.w, b.x + b.w);
        const int y1 = (std::max)(a.y, b.y);
        const int y2 = (std::min)(a.y + a.h, b.y + b.h);

        if (x2 >= x1 && y2 >= y1)
        {
            return libCZI::IntRect{ x1, y1, x2 - x1, y2 - y1 };
        }

        return libCZI::IntRect{ 0,0,0,0 };
    }

    static inline bool DoIntersect(const libCZI::IntRect& a, const libCZI::IntRect& b)
    {
        const auto r = Intersect(a, b);
        return ((r.w > 0) && (r.h > 0));
    }

    static inline std::uint8_t clampToByte(float f)
    {
        if (f <= 0)
        {
            return 0;
        }
        else if (f >= 255)
        {
            return 255;
        }

        return static_cast<std::uint8_t>(f + .5f);
    }

    static inline std::uint16_t clampToUShort(float f)
    {
        if (f <= 0)
        {
            return 0;
        }
        else if (f >= 65535)
        {
            return 65535;
        }

        return static_cast<std::uint16_t>(f + .5f);
    }

    static std::uint8_t HexCharToInt(char c);

    static std::string Trim(const std::string& str, const std::string& whitespace = " \t");
    static std::wstring Trim(const std::wstring& str, const std::wstring& whitespace = L" \t");

    static bool icasecmp(const std::string& l, const std::string& r);
    static bool icasecmp(const std::wstring& l, const std::wstring& r);

    template<typename t>
    inline static t clamp(t v, t min, t max)
    {
        if (v < min)
        {
            return min;
        }
        else if (v > max)
        {
            return max;
        }

        return v;
    }

    static std::wstring convertUtf8ToWchar_t(const char* sz);

    static void Tokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L" ");

    static GUID GenerateNewGuid();

    static bool IsGuidNull(const GUID& g);

    static void ConvertInt16ToHostByteOrder(std::int16_t* p);
    static void ConvertInt32ToHostByteOrder(std::int32_t* p);
    static void ConvertInt64ToHostByteOrder(std::int64_t* p);
    static void ConvertGuidToHostByteOrder(GUID* p);

    static bool TryGetRgb8ColorFromString(const std::wstring& strXml, libCZI::Rgb8Color& color);
};

class LoHiBytePackUnpack
{
public:
    static void LoHiByteUnpackStrided(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst);
    static void LoHiBytePackStrided(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest);
protected:
    static void LoHiByteUnpackStrided_C(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst);
    static void LoHiBytePackStrided_C(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest);
    static void CheckLoHiBytePackArgumentsAndThrow(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest);
    static void CheckLoHiByteUnpackArgumentsAndThrow(std::uint32_t width, std::uint32_t stride, const void* source, void* dest);
};

template <typename t>
struct Nullable
{
    Nullable() :isValid(false) {};

    bool isValid;
    t	 value;

    bool TryGet(t* p) const
    {
        if (this->isValid == true)
        {
            if (p != nullptr)
            {
                *p = this->value;
            }

            return true;
        }

        return false;
    }

    void Set(const t& x)
    {
        this->value = x;
        this->isValid = true;
    }
};

template <typename t>
struct ParseEnumHelper
{
    struct EnumValue
    {
        const wchar_t* text;
        t              value;
    };

    static bool TryParseEnum(const EnumValue* values, int count, const wchar_t* str, t* pEnumValue)
    {
        for (int i = 0; i < count; ++i, ++values)
        {
            if (wcscmp(str, values->text) == 0)
            {
                if (pEnumValue != nullptr)
                {
                    *pEnumValue = values->value;
                    return true;
                }
            }
        }

        return false;
    }
};

