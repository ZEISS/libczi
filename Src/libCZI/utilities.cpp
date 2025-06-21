// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "utilities.h"
#include "inc_libCZI_Config.h"
#include <locale>
#include <codecvt>
#include <sstream>
#include <cstring>
#include <array>
#if LIBCZI_WINDOWSAPI_AVAILABLE
#include <windows.h>
#else
#include <random>
#endif
#if LIBCZI_HAVE_ENDIAN_H
#include "endian.h"
#endif

using namespace std;

/*static*/std::uint8_t Utilities::HexCharToInt(char c)
{
    switch (c)
    {
    case '0':return 0;
    case '1':return 1;
    case '2':return 2;
    case '3':return 3;
    case '4':return 4;
    case '5':return 5;
    case '6':return 6;
    case '7':return 7;
    case '8':return 8;
    case '9':return 9;
    case 'A':case 'a':return 10;
    case 'B':case 'b':return 11;
    case 'C':case 'c':return 12;
    case 'D':case 'd':return 13;
    case 'E':case 'e':return 14;
    case 'F':case 'f':return 15;
    }

    return 0xff;
}

/*static*/char Utilities::NibbleToHexChar(std::uint8_t nibble)
{
    static const char* hex_digits = "0123456789ABCDEF";
    return hex_digits[nibble & 0x0f];
}

template <class tString>
tString trimImpl(const tString& str, const tString& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == tString::npos)
        return tString(); // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

/*static*/std::string Utilities::Trim(const std::string& str, const std::string& whitespace /*= " \t"*/)
{
    return trimImpl(str, whitespace);
}

/*static*/std::wstring Utilities::Trim(const std::wstring& str, const std::wstring& whitespace /*= L" \t"*/)
{
    return trimImpl(str, whitespace);
}


/*static*/bool Utilities::icasecmp(const std::string& l, const std::string& r)
{
    return l.size() == r.size()
        && equal(l.cbegin(), l.cend(), r.cbegin(),
        [](std::string::value_type l1, std::string::value_type r1)
        { return toupper(l1) == toupper(r1); });
}

/*static*/bool Utilities::icasecmp(const std::wstring& l, const std::wstring& r)
{
    return l.size() == r.size()
        && equal(l.cbegin(), l.cend(), r.cbegin(),
        [](std::wstring::value_type l1, std::wstring::value_type r1)
        { return towupper(l1) == towupper(r1); });
}

/*static*/std::wstring Utilities::convertUtf8ToWchar_t(const char* sz)
{
#if LIBCZI_WINDOWSAPI_AVAILABLE
    if (*sz == '\0')
    {
        return {};
    }

    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, sz, -1, nullptr, 0);
    if (size_needed <= 0) 
    {
        throw runtime_error("MultiByteToWideChar failed: " + std::to_string(GetLastError()));
    }

    wstring wide_string;
    wide_string.resize(size_needed - 1); // Exclude the null terminator

    if (MultiByteToWideChar(CP_UTF8, 0, sz, -1, &wide_string[0], size_needed) == 0) 
    {
        throw runtime_error("MultiByteToWideChar conversion failed: " + std::to_string(GetLastError()));
    }

    return wide_string;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8conv;
    std::wstring conv = utf8conv.from_bytes(sz);
    return conv;
#endif
}

/*static*/std::string Utilities::convertWchar_tToUtf8(const wchar_t* szw)
{
#if LIBCZI_WINDOWSAPI_AVAILABLE
    if (*szw == L'\0')
    {
        return {};
    }

    // Calculate the required buffer size
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, szw, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) 
    {
        throw runtime_error("WideCharToMultiByte failed: " + std::to_string(GetLastError()));
    }

    // Allocate buffer for the UTF-8 string
    string utf8_str;
    utf8_str.resize(size_needed - 1); // Exclude the null terminator

    if (WideCharToMultiByte(CP_UTF8, 0, szw, -1, &utf8_str[0], size_needed, nullptr, nullptr) == 0) 
    {
        throw runtime_error("WideCharToMultiByte conversion failed: " + std::to_string(GetLastError()));
    }

    return utf8_str;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    std::string conv = utf8_conv.to_bytes(szw);
    return conv;
#endif
}

/*static*/void Utilities::Tokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters)
{
    size_t start = 0, end = 0;

    while (end != std::wstring::npos)
    {
        end = str.find_first_of(delimiters, start);

        // Push the token, which could be empty if start == end
        tokens.push_back(str.substr(start, end - start));

        // Update start to one character after the found delimiter
        start = end + 1;
    }
}

/*static*/libCZI::GUID Utilities::GenerateNewGuid()
{
#if LIBCZI_WINDOWSAPI_AVAILABLE
    ::GUID guid;
    CoCreateGuid(&guid);
    libCZI::GUID guid_value
    {
        guid.Data1,
            guid.Data2,
            guid.Data3,
        { guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7] } };
    return guid_value;
#else
    std::mt19937 rng;
    rng.seed(std::random_device()());
    uniform_int_distribution<uint32_t> distu32;
    libCZI::GUID g;
    g.Data1 = distu32(rng);
    auto r = distu32(rng);
    g.Data2 = (uint16_t)r;
    g.Data3 = (uint16_t)(r >> 16);

    r = distu32(rng);
    for (int i = 0; i < 4; ++i)
    {
        g.Data4[i] = (uint8_t)r;
        r >>= 8;
    }

    r = distu32(rng);
    for (int i = 4; i < 8; ++i)
    {
        g.Data4[i] = (uint8_t)r;
        r >>= 8;
    }

    return g;
#endif
}

/*static*/std::string Utilities::Rgb8ColorToString(const libCZI::Rgb8Color& color)
{
    string color_text;
    color_text.reserve(10);
    color_text += '#';
    color_text += "FF";
    color_text += Utilities::NibbleToHexChar(color.r >> 4);
    color_text += Utilities::NibbleToHexChar(color.r);
    color_text += Utilities::NibbleToHexChar(color.g >> 4);
    color_text += Utilities::NibbleToHexChar(color.g);
    color_text += Utilities::NibbleToHexChar(color.b >> 4);
    color_text += Utilities::NibbleToHexChar(color.b);
    return color_text;
}

/*static*/bool Utilities::TryGetRgb8ColorFromString(const std::wstring& strXml, libCZI::Rgb8Color& color)
{
    const auto str = Utilities::Trim(strXml);
    if (str.size() > 1 && str[0] == '#')
    {
        // TODO: do we have to deal with shorter string (e.g. without alpha, or just '#FF')?
        std::uint8_t r = 0, g = 0, b = 0, a = 0;
        for (size_t i = 1; i < (std::max)(static_cast<size_t>(9), str.size()); ++i)
        {
            if (!isxdigit(str[i]))
            {
                return false;
            }

            switch (i)
            {
            case 1: a = Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 2: a = static_cast<uint8_t>(a << 4) | Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 3: r = Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 4: r = static_cast<uint8_t>(r << 4) | Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 5: g = Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 6: g = static_cast<uint8_t>(g << 4) | Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 7: b = Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            case 8: b = static_cast<uint8_t>(b << 4) | Utilities::HexCharToInt(static_cast<char>(str[i])); break;
            }
        }

        color = libCZI::Rgb8Color{ r,g,b };
        return true;
    }

    return false;
}

/*static*/bool Utilities::IsGuidNull(const libCZI::GUID& g)
{
    return g.Data1 == 0 && g.Data2 == 0 && g.Data3 == 0 &&
        g.Data4[0] == 0 && g.Data4[1] == 0 && g.Data4[2] == 0 && g.Data4[3] == 0 &&
        g.Data4[4] == 0 && g.Data4[5] == 0 && g.Data4[6] == 0 && g.Data4[7] == 0;
}

/*static*/void Utilities::ConvertInt16ToHostByteOrder(std::int16_t* p)
{
#if LIBCZI_ISBIGENDIANHOST
#if LIBCZI_HAVE_ENDIAN_H
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    int16_t x;
    memcpy(&x, p, sizeof(int16_t));
    x = le16toh(x);
    memcpy(p, &x, sizeof(int16_t));
#else
    * p = le16toh(*p);
#endif
#else
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    int16_t v;
    memcpy(&v, p, sizeof(int16_t));
#else
    int16_t v = *p;
#endif
    v = ((v & 0x00ff) << 8) | ((v & 0xff00) >> 8);
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    memcpy(p, &v, sizeof(int16_t));
#else
    * p = v;
#endif
#endif
#endif
}

/*static*/void Utilities::ConvertInt32ToHostByteOrder(std::int32_t* p)
{
#if LIBCZI_ISBIGENDIANHOST
#if LIBCZI_HAVE_ENDIAN_H
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    int32_t x;
    memcpy(&x, p, sizeof(int32_t));
    x = le32toh(x);
    memcpy(p, &x, sizeof(int32_t));
#else
    * p = le32toh(*p);
#endif
#else
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    int32_t v;
    memcpy(&v, p, sizeof(int32_t));
#else
    int32_t v = *p;
#endif
    v = ((((v) & 0xff000000u) >> 24) | (((v) & 0x00ff0000u) >> 8)
        | (((v) & 0x0000ff00u) << 8) | (((v) & 0x000000ffu) << 24));
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    memcpy(p, &v, sizeof(int32_t));
#else
    * p = v;
#endif
#endif
#endif
}

/*static*/void Utilities::ConvertInt64ToHostByteOrder(std::int64_t* p)
{
#if LIBCZI_ISBIGENDIANHOST
#if LIBCZI_HAVE_ENDIAN_H
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    int64_t x;
    memcpy(&x, p, sizeof(int64_t));
    x = le64toh(x);
    memcpy(p, &x, sizeof(int64_t));
#else
    * p = le64toh(*p);
#endif
#else
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    uint64_t v;
    memcpy(&v, p, sizeof(uint64_t));
#else
    uint64_t v = *((uint64_t*)p);
#endif
    v = ((((v) & 0xff00000000000000ull) >> 56)
        | (((v) & 0x00ff000000000000ull) >> 40)
        | (((v) & 0x0000ff0000000000ull) >> 24)
        | (((v) & 0x000000ff00000000ull) >> 8)
        | (((v) & 0x00000000ff000000ull) << 8)
        | (((v) & 0x0000000000ff0000ull) << 24)
        | (((v) & 0x000000000000ff00ull) << 40)
        | (((v) & 0x00000000000000ffull) << 56));
    *p = (int64_t)v;
#if LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS
    memcpy(p, &v, sizeof(uint64_t));
#else
    * p = (int64_t)v;
#endif
#endif
#endif
}

/*static*/void Utilities::ConvertGuidToHostByteOrder(libCZI::GUID* p)
{
#if LIBCZI_ISBIGENDIANHOST
    Utilities::ConvertInt32ToHostByteOrder((int32_t*)&(p->Data1));
    Utilities::ConvertInt16ToHostByteOrder((int16_t*)&(p->Data2));
    Utilities::ConvertInt16ToHostByteOrder((int16_t*)&(p->Data3));
#endif
}

/*static*/std::map<std::wstring, std::wstring> Utilities::TokenizeAzureUriString(const std::wstring& input)
{
    std::map<std::wstring, std::wstring> tokens;  // Map to store key-value pairs
    std::wstring key, value;                      // Temporary storage for current key and value
    bool inValue = false;                         // Flag to track if we are processing the value part
    bool foundEquals = false;                     // Track if we've encountered an '=' for a valid key-value pair

    for (size_t i = 0; i < input.length(); ++i)
    {
        const wchar_t c = input[i];

        // Handle escape sequences for semicolons (\;) and equals signs (\=)
        if (c == L'\\' && i + 1 < input.length())
        {
            const wchar_t next = input[i + 1];
            if (next == L';')
            {
                // Escaped semicolon (\;)
                if (inValue)
                {
                    value += L';';
                }
                else
                {
                    key += L';';
                }

                ++i;  // Skip the semicolon after the backslash
            }
            else if (next == L'=')
            {
                // Escaped equals sign (\=)
                if (inValue)
                {
                    value += L'=';
                }
                else
                {
                    key += L'=';
                }

                ++i;  // Skip the equals sign after the backslash
            }
            else
            {
                // If it's not an escape sequence we care about, treat the backslash literally
                if (inValue)
                {
                    value += c;  // Add the backslash to the value
                }
                else
                {
                    key += c;    // Add the backslash to the key
                }
            }
        }
        else if (c == L'=' && !inValue)
        {
            // Start processing the value part when we hit an unescaped '='
            inValue = true;
            foundEquals = true;
        }
        else if (c == L';' && inValue)
        {
            // If we hit ';' while processing the value, it's the end of the current key-value pair
            if (key.empty())
            {
                throw std::invalid_argument("Found a value without a corresponding key.");
            }

            tokens[key] = value;  // Store the key-value pair
            key.clear();          // Reset key and value for the next pair
            value.clear();
            inValue = false;
            foundEquals = false;
        }
        else
        {
            // Collect characters for the key or value depending on the state
            if (inValue)
            {
                value += c;
            }
            else
            {
                key += c;
            }
        }
    }

    // Handle the case where no '=' was found in the input (invalid input)
    if (!foundEquals)
    {
        throw std::invalid_argument("Input does not contain a valid key-value pair.");
    }

    // Add the last key-value pair (if any exists after the loop)
    if (!key.empty())
    {
        if (value.empty() && inValue)
        {
            throw std::invalid_argument("Found a key without a corresponding value.");
        }

        tokens[key] = value;
    }

    // Check if we have at least one valid key-value pair; if not, it's an error
    if (tokens.empty())
    {
        throw std::invalid_argument("No complete key-value pair found in the input.");
    }

    return tokens;
}

//-----------------------------------------------------------------------------

/*static*/void LoHiBytePackUnpack::CheckLoHiByteUnpackArgumentsAndThrow(std::uint32_t width, std::uint32_t stride, const void* source, void* dest)
{
    if (source == nullptr)
    {
        throw invalid_argument("'source' must not be null.");
    }

    if (dest == nullptr)
    {
        throw invalid_argument("'dest' must not be null.");
    }

    if (stride < width * 2)
    {
        stringstream ss;
        ss << "For a width of " << width << " pixels, the stride must be >= " << width * 2 << ".";
        throw invalid_argument(ss.str());
    }
}

/*static*/void LoHiBytePackUnpack::CheckLoHiBytePackArgumentsAndThrow(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    if (ptrSrc == nullptr)
    {
        throw invalid_argument("'ptrSrc' must not be null.");
    }

    if (dest == nullptr)
    {
        throw invalid_argument("'dest' must not be null.");
    }

    if (sizeSrc < static_cast<size_t>(width) * height * 2)
    {
        stringstream ss;
        ss << "For a width of " << width << " pixels and a height of " << height << ", the 'sizeSrc' must be >= " << static_cast<size_t>(width) * height * 2 << ".";
        throw invalid_argument(ss.str());
    }

    if (stride < width * 2)
    {
        stringstream ss;
        ss << "For a width of " << width << " pixels, the stride must be >= " << width * 2 << ".";
        throw invalid_argument(ss.str());
    }
}

/*static*/void LoHiBytePackUnpack::LoHiByteUnpackStrided_C(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst)
{
    uint8_t* pDst = static_cast<uint8_t*>(ptrDst);

    const size_t halfLength = (static_cast<size_t>(wordCount) * 2 * lineCount) / 2;
    for (uint32_t y = 0; y < lineCount; ++y)
    {
        const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(ptrSrc) + y * static_cast<size_t>(stride));
        for (uint32_t x = 0; x < wordCount; ++x)
        {
            const uint16_t v = *pSrc++;
            *pDst = static_cast<uint8_t>(v);
            *(pDst + halfLength) = static_cast<uint8_t>(v >> 8);
            pDst++;
        }
    }
}

/*static*/void LoHiBytePackUnpack::LoHiBytePackStrided_C(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    const uint8_t* pSrc = static_cast<const uint8_t*>(ptrSrc);

    const size_t halfLength = sizeSrc / 2;

    for (uint32_t y = 0; y < height; ++y)
    {
        uint16_t* pDst = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(dest) + static_cast<size_t>(y) * stride);
        for (uint32_t x = 0; x < width; ++x)
        {
            const uint16_t v = *pSrc | (static_cast<uint16_t>(*(pSrc + halfLength)) << 8);
            *pDst++ = v;
            ++pSrc;
        }
    }
}

#if !LIBCZI_HAS_NEOININTRINSICS && !LIBCZI_HAS_AVXINTRINSICS
/*static*/void LoHiBytePackUnpack::LoHiByteUnpackStrided(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst)
{
    LoHiBytePackUnpack::CheckLoHiByteUnpackArgumentsAndThrow(wordCount, stride, ptrSrc, ptrDst);
    LoHiByteUnpackStrided_C(ptrSrc, wordCount, stride, lineCount, ptrDst);
}

/*static*/void LoHiBytePackUnpack::LoHiBytePackStrided(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    LoHiBytePackUnpack::CheckLoHiBytePackArgumentsAndThrow(ptrSrc, sizeSrc, width, height, stride, dest);
    LoHiBytePackStrided_C(ptrSrc, sizeSrc, width, height, stride, dest);
}
#endif

void RectangleCoverageCalculator::AddRectangle(const libCZI::IntRect& rectangle)
{
    if (!rectangle.IsValid())
    {
        return;
    }

    if (this->splitters_.empty())
    {
        this->splitters_.push_back(rectangle);
    }
    else
    {
        for (auto splitter = this->splitters_.begin(); splitter != this->splitters_.end(); ++splitter)
        {
            // does the rectangle intersect with one of existing rectangles?
            if (splitter->IntersectsWith(rectangle) == true)
            {
                // check if it is completely contained in the current existing rectangle
                if (RectangleCoverageCalculator::IsCompletelyContained(*splitter, rectangle) == true)
                {
                    // ok, in this case we have nothing to do!
                    return;
                }

                // check if the existing rectangle is completely contained in the new one
                if (RectangleCoverageCalculator::IsCompletelyContained(rectangle, *splitter) == true)
                {
                    // in this case we remove the (smaller) rect splitters[i] (which is fully
                    // contained in rectangle) from our list and add rectangle
                    this->splitters_.erase(splitter);
                    this->AddRectangle(rectangle);
                    return;
                }

                // ok, the rectangle overlap only partially... then let's cut the new rectangle into pieces
                // (which do not intersect with the currently investigated rectangle) and try
                // to add those pieces
                std::array<libCZI::IntRect, 4> splitUpRects;
                const int number_of_split_up_rects = SplitUpIntoNonOverlapping(*splitter, rectangle, splitUpRects);
                for (int n = 0; n < number_of_split_up_rects; ++n)
                {
                    this->AddRectangle(splitUpRects[n]);
                }

                return;
            }
        }

        // if we end up here this means that the new rectangle does not
        // overlap with an existing one -> we can happily add it now!
        this->splitters_.push_back(rectangle);
    }
}

/*static*/int RectangleCoverageCalculator::SplitUpIntoNonOverlapping(const libCZI::IntRect& rectangle_a, const libCZI::IntRect& rectangle_b, std::array<libCZI::IntRect, 4>& result)
{
    // precondition: rectangle_b is not completely contained in rectangle_a (and rectangle_a not completely contained in rectangle_b)!
    int result_index = 0;
    if (rectangle_b.x >= rectangle_a.x && rectangle_b.x + rectangle_b.w <= rectangle_a.x + rectangle_a.w)
    {
        if (rectangle_a.y > rectangle_b.y)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_b.y, rectangle_b.w, rectangle_a.y - rectangle_b.y };
        }

        if (rectangle_b.y + rectangle_b.h > rectangle_a.y + rectangle_a.h)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_a.y + rectangle_a.h, rectangle_b.w, rectangle_b.y + rectangle_b.h - rectangle_a.y - rectangle_a.h };
        }
    }
    else if (rectangle_b.x < rectangle_a.x && rectangle_b.x + rectangle_b.w <= rectangle_a.x + rectangle_a.w)
    {
        result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_b.y, rectangle_a.x - rectangle_b.x, rectangle_b.h };
        if (rectangle_b.y < rectangle_a.y)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_a.x, rectangle_b.y, rectangle_b.x + rectangle_b.w - rectangle_a.x, rectangle_a.y - rectangle_b.y };
        }

        if (rectangle_b.y + rectangle_b.h > rectangle_a.y + rectangle_a.h)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_a.x, rectangle_a.y + rectangle_a.h, rectangle_b.x + rectangle_b.w - rectangle_a.x, rectangle_b.y + rectangle_b.h - rectangle_a.y - rectangle_a.h };
        }
    }
    else if (rectangle_b.x >= rectangle_a.x && rectangle_b.x + rectangle_b.w > rectangle_a.x + rectangle_a.w)
    {
        result[result_index++] = libCZI::IntRect{ rectangle_a.x + rectangle_a.w, rectangle_b.y, rectangle_b.x + rectangle_b.w - rectangle_a.x - rectangle_a.w, rectangle_b.h };

        if (rectangle_b.y < rectangle_a.y)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_b.y, rectangle_a.x + rectangle_a.w - rectangle_b.x, rectangle_a.y - rectangle_b.y };
        }

        if (rectangle_b.y + rectangle_b.h > rectangle_a.y + rectangle_a.h)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_a.y + rectangle_a.h, rectangle_a.x + rectangle_a.w - rectangle_b.x, rectangle_b.y + rectangle_b.h - rectangle_a.y - rectangle_a.h };
        }
    }
    else if (rectangle_b.x <= rectangle_a.x && rectangle_b.x + rectangle_b.w >= rectangle_a.x + rectangle_a.w)
    {
        result[result_index++] = libCZI::IntRect{ rectangle_b.x, rectangle_b.y, rectangle_a.x - rectangle_b.x, rectangle_b.h };
        result[result_index++] = libCZI::IntRect{ rectangle_a.x + rectangle_a.w, rectangle_b.y, rectangle_b.x + rectangle_b.w - rectangle_a.x - rectangle_a.w, rectangle_b.h };

        if (rectangle_a.y > rectangle_b.y)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_a.x, rectangle_b.y, rectangle_a.w, rectangle_a.y - rectangle_b.y };
        }
        else if (rectangle_a.y + rectangle_a.h > rectangle_b.y && rectangle_a.y + rectangle_a.h < rectangle_b.y + rectangle_b.h)
        {
            result[result_index++] = libCZI::IntRect{ rectangle_a.x, rectangle_a.y + rectangle_a.h, rectangle_a.w, rectangle_b.y + rectangle_b.h - rectangle_a.y - rectangle_a.h };
        }
    }

    return result_index;
}


/*static*/bool RectangleCoverageCalculator::IsCompletelyContained(const libCZI::IntRect& outer, const libCZI::IntRect& inner)
{
    return inner.x >= outer.x && inner.x + inner.w <= outer.x + outer.w &&
        inner.y >= outer.y && inner.y + inner.h <= outer.y + outer.h;
}

std::int64_t RectangleCoverageCalculator::CalcAreaOfIntersectionWithRectangle(const libCZI::IntRect& query_rectangle) const
{
    if (!query_rectangle.IsValid())
    {
        return 0;
    }

    int64_t area = 0;
    for (const auto& r : this->splitters_)
    {
        auto intersection = r.Intersect(query_rectangle);
        if (intersection.IsValid())
        {
            area += intersection.w * static_cast<int64_t>(intersection.h);
        }
    }

    return area;
}

bool RectangleCoverageCalculator::IsCompletelyCovered(const libCZI::IntRect& query_rectangle) const
{
    if (!query_rectangle.IsValid())
    {
        return true;
    }

    return this->CalcAreaOfIntersectionWithRectangle(query_rectangle) == static_cast<int64_t>(query_rectangle.w) * query_rectangle.h;
}
