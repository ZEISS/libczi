// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <cwctype>
#include <functional>
#include <vector>
#include <map>
#include "libCZI_Pixels.h"
#include "libCZI_Utilities.h"

class Utilities
{
public:
    static void Split(const std::wstring& text, wchar_t sep, const std::function<bool(const std::wstring&)>& funcToken)
    {
        std::size_t start = 0, end;
        while ((end = text.find(sep, start)) != std::wstring::npos)
        {
            std::wstring temp = text.substr(start, end - start);
            if (!temp.empty())
            {
                if (funcToken(temp) == false)
                {
                    return;
                }
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

    static libCZI::IntRect Intersect(const libCZI::IntRect& a, const libCZI::IntRect& b)
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

    static bool DoIntersect(const libCZI::IntRect& a, const libCZI::IntRect& b)
    {
        const auto r = Intersect(a, b);
        return ((r.w > 0) && (r.h > 0));
    }

    static std::uint8_t clampToByte(float f)
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

    static std::uint16_t clampToUShort(float f)
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
    static char NibbleToHexChar(std::uint8_t nibble);

    static std::string Trim(const std::string& str, const std::string& whitespace = " \t");
    static std::wstring Trim(const std::wstring& str, const std::wstring& whitespace = L" \t");

    static bool icasecmp(const std::string& l, const std::string& r);
    static bool icasecmp(const std::wstring& l, const std::wstring& r);

    template<typename t>
    static t clamp(t v, t min, t max)
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
    static std::string convertWchar_tToUtf8(const wchar_t* szw);

    /// Split the specified string at the specified delimiter characters, and add the individual tokens (=
    /// parts between delimiters or between start/end and a delimiter) to the specified vector.
    /// Note that:
    /// - the function will add an empty string (to the result vector) in case of two consecutive delimiters  
    /// - the result vector will contain the tokens in the order they appear in the input string, and the  
    ///    vector is not cleared before adding the tokens.
    /// - for an empty string, the result is one token, which is an empty string.
    ///
    /// \param          str         The string to tokenize.
    /// \param [in,out] tokens      The vector to which the tokens are added.
    /// \param          delimiters  A string defining the delimiter-characters.
    static void Tokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L" ");

    static libCZI::GUID GenerateNewGuid();

    static bool IsGuidNull(const libCZI::GUID& g);

    static void ConvertInt16ToHostByteOrder(std::int16_t* p);
    static void ConvertInt32ToHostByteOrder(std::int32_t* p);
    static void ConvertInt64ToHostByteOrder(std::int64_t* p);
    static void ConvertGuidToHostByteOrder(libCZI::GUID* p);

    static bool TryGetRgb8ColorFromString(const std::wstring& strXml, libCZI::Rgb8Color& color);
    static std::string Rgb8ColorToString(const libCZI::Rgb8Color& color);

    static std::map<std::wstring, std::wstring> TokenizeAzureUriString(const std::wstring& input);
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
    Nullable() :isValid(false) {}

    bool isValid;
    t    value;

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

/// This class allows to calculate the area covered by a set of rectangles. The mode of operation
/// is:
/// - Create an instance of the class and add the rectangles to it (using AddRectangle) .
/// - Then, for a given rectangle, call CalcAreaOfIntersectionWithRectangle in order to get the area of the intersection  
///    of this rectangle with the union of the rectangles added before.
/// The rectangles being added do not have to follow any order, or are required to be non-overlapping.
class RectangleCoverageCalculator
{
private:
    /// This vector contains the rectangles added to the state of the instance. The rectangles
    /// in this vector are guaranteed to be non-overlapping. That is why the name 'splitters' is used -
    /// if a rectangles is added which is overlapping with the existing ones, the rectangle is split into smaller 
    /// rectangles (which we call the 'splitters') which are non-overlapping with the existing ones.
    std::vector<libCZI::IntRect> splitters_;    
public:
    /// Adds a rectangle to the state. The runtime of this method increases with the number of
    /// splitters in the state of the instance and the number of rectangles this rectangle
    /// needs to be split into. For a modest number of rectangles, the runtime is for
    /// sure negligible.
    /// A pathologic case to be aware of is when there are many existing and then a large rectangle
    /// is added (which is overlapping with many of the existing ones). In this case, it would be
    /// greatly beneficial to add the large rectangle first, and then the smaller ones.
    ///
    /// \param  rectangle   The rectangle to be added.
    void AddRectangle(const libCZI::IntRect& rectangle);

    /// Adds the rectangles given by the iterator to the state of the instance.
    ///
    /// \typeparam  tIterator   Type of the iterator.
    /// \param  begin   The begin.
    /// \param  end     The end.
    template <typename tIterator>
    void AddRectangles(tIterator begin, tIterator end)
    {
        for (tIterator it = begin; it != end; ++it) 
        {
            this->AddRectangle(*it);
        }
    }

    /// Calculates the area of intersection of the specified rectangle with the
    /// union of the rectangles added before.
    /// If the query_rectangle rectangle is invalid, the return value is 0.
    ///
    /// \param  query_rectangle   The query rectangle.
    ///
    /// \returns    The calculated area of intersection of the specified rectangle with the
    ///             union of the rectangles added before.
    std::int64_t CalcAreaOfIntersectionWithRectangle(const libCZI::IntRect& query_rectangle) const;

    /// Query if 'rectQuery' is completely covered is completely covered by the union of the rectangles added before.
    /// If the query_rectangle rectangle is invalid, the return value is true.
    ///
    /// \param  query_rectangle   The query rectangle.
    ///
    /// \returns    True if completely covered; false otherwise.
    bool IsCompletelyCovered(const libCZI::IntRect& query_rectangle) const;
private:
    /// Test whether the rectangle 'inner' is completely contained in the rectangle 'outer'.
    ///
    /// \param  outer   The outer rectangle.
    /// \param  inner   The inner rectangle.
    ///
    /// \returns    True if completely contained, false if not.
    static bool IsCompletelyContained(const libCZI::IntRect& outer, const libCZI::IntRect& inner);

    /// The area of rectangle_b which does not overlap with rectangle_a is determined and
    /// we return this part as a list of 4 rectangles at most. The return value is the number
    /// of rectangles in the result array.
    ///
    /// \param          rectangle_a The rectangle a.
    /// \param          rectangle_b The rectangle b.
    /// \param [out]    result      The resulting splitters are put into this array. There will be 4 rectangles at most.
    ///
    /// \returns    The number of valid rectangles in the 'result' array.
    static int SplitUpIntoNonOverlapping(const libCZI::IntRect& rectangle_a, const libCZI::IntRect& rectangle_b, std::array<libCZI::IntRect, 4>& result);
};
