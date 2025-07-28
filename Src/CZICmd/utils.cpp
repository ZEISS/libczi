// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "inc_CZIcmd_Config.h"
#include "utils.h"
#include <cwctype>
#include <iomanip>
#include <regex>

#if CZICMD_WINDOWSAPI_AVAILABLE
#include <windows.h>
#endif

#if defined(HAS_CODECVT)
#include <locale>
#include <codecvt>
#include <stdlib.h>
#endif

using namespace std;

std::string convertToUtf8(const std::wstring& str)
{
#if CZICMD_WINDOWSAPI_AVAILABLE
    if (str.empty())
    {
        return {};
    }

    // Determine the size of the resulting UTF-8 string
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0)
    {
        throw runtime_error("Error in WideCharToMultiByte");
    }

    // Allocate buffer for the UTF-8 string
    string result(sizeNeeded, '\0');

    // Perform the conversion from wide string (UTF-16) to UTF-8
    if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &result[0], sizeNeeded, NULL, NULL) <= 0)
    {
        throw runtime_error("Error in WideCharToMultiByte");
    }

    // Remove the null terminator added by WideCharToMultiByte
    result.resize(sizeNeeded - 1);

    return result;
#else

#if defined(HAS_CODECVT)
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    std::string conv = utf8_conv.to_bytes(str);
    return conv;
#else
    size_t requiredSize = std::wcstombs(nullptr, str.c_str(), 0);
    std::string conv(requiredSize, 0);
    conv.resize(std::wcstombs(&conv[0], str.c_str(), requiredSize));
    return conv;
#endif

#endif
}

std::wstring convertUtf8ToWide(const std::string& str)
{
#if CZICMD_WINDOWSAPI_AVAILABLE
    if (str.empty())
    {
        return {};
    }

    const int buffer_size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (buffer_size <= 0)
    {
        throw runtime_error("Error in MultiByteToWideChar");
    }

    wstring result(buffer_size, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], buffer_size) <= 0)
    {
        throw runtime_error("Error in MultiByteToWideChar");
    }

    // Remove the null terminator added by MultiByteToWideChar
    result.resize(buffer_size - 1);
    return result;
#else

#if defined(HAS_CODECVT)
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8conv;
    std::wstring conv = utf8conv.from_bytes(str);
    return conv;
#else
    std::wstring conv(str.size(), 0);
    size_t size = std::mbstowcs(&conv[0], str.c_str(), str.size());
    conv.resize(size);
    return conv;
#endif

#endif
}

std::string trim(const std::string& str, const std::string& whitespace /*= " \t"*/)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::wstring trim(const std::wstring& str, const std::wstring& whitespace /*= " \t"*/)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::wstring::npos)
        return L""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

bool icasecmp(const std::string& l, const std::string& r)
{
    return l.size() == r.size()
        && equal(l.cbegin(), l.cend(), r.cbegin(),
        [](std::string::value_type l1, std::string::value_type r1)
        { return toupper(l1) == toupper(r1); });
}

bool icasecmp(const std::wstring& l, const std::wstring& r)
{
    return l.size() == r.size()
        && equal(l.cbegin(), l.cend(), r.cbegin(),
        [](std::wstring::value_type l1, std::wstring::value_type r1)
        { return towupper(l1) == towupper(r1); });
}

std::uint8_t HexCharToInt(char c)
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

bool ConvertHexStringToInteger(const char* cp, std::uint32_t* value)
{
    if (*cp == '\0')
    {
        return false;
    }

    std::uint32_t v = 0;
    int cntOfSignificantDigits = 0;
    for (; *cp != '\0'; ++cp)
    {
        std::uint8_t x = HexCharToInt(*cp);
        if (x == 0xff)
        {
            return false;
        }

        if (v > 0)
        {
            if (++cntOfSignificantDigits > 7)
            {
                return false;
            }
        }

        v = v * 16 + x;
    }

    if (value != nullptr)
    {
        *value = v;
    }

    return true;
}

char LowerNibbleToHexChar(std::uint8_t v)
{
    static char Hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    return Hex[v & 0xf];
}
char UpperNibbleToHexChar(std::uint8_t v)
{
    return LowerNibbleToHexChar(v >> 4);
}

std::string BytesToHexString(const std::uint8_t* ptr, size_t size)
{
    std::string s;
    s.reserve(size * 2);
    for (size_t i = 0; i < size; ++i)
    {
        s.push_back(UpperNibbleToHexChar(ptr[i]));
        s.push_back(LowerNibbleToHexChar(ptr[i]));
    }

    return s;
}

std::wstring BytesToHexWString(const std::uint8_t* ptr, size_t size)
{
    std::wstring s;
    s.reserve(size * 2);
    for (size_t i = 0; i < size; ++i)
    {
        s.push_back(UpperNibbleToHexChar(ptr[i]));
        s.push_back(LowerNibbleToHexChar(ptr[i]));
    }

    return s;
}

std::vector<std::wstring> wrap(const wchar_t* text, size_t line_length/* = 72*/)
{
    wistringstream iss(text);
    std::wstring word, line;
    std::vector<std::wstring> vec;

    for (; ;)
    {
        iss >> word;
        if (!iss)
        {
            break;
        }

        // '\n' before a word means "insert a linebreak", and "\N' means "insert a linebreak and one more empty line"
        if (word.size() > 2 && word[0] == L'\\' && (word[1] == L'n' || word[1] == L'N'))
        {
            line.erase(line.size() - 1);    // remove trailing space
            vec.push_back(line);
            if (word[1] == L'N')
            {
                vec.push_back(L"");
            }

            line.clear();
            word.erase(0, 2);
        }

        if (line.length() + word.length() > line_length)
        {
            line.erase(line.size() - 1);
            vec.push_back(line);
            line.clear();
        }

        line += word + L' ';
    }

    if (!line.empty())
    {
        line.erase(line.size() - 1);
        vec.push_back(line);
    }

    return vec;
}

std::vector<std::string> wrap(const char* text, size_t line_length/* = 72*/)
{
    istringstream iss(text);
    std::string word, line;
    std::vector<std::string> vec;

    for (; ;)
    {
        iss >> word;
        if (!iss)
        {
            break;
        }

        // '\n' before a word means "insert a linebreak", and "\N' means "insert a linebreak and one more empty line"
        if (word.size() > 2 && word[0] == '\\' && (word[1] == 'n' || word[1] == 'N'))
        {
            line.erase(line.size() - 1);    // remove trailing space
            vec.push_back(line);
            if (word[1] == 'N')
            {
                vec.push_back("");
            }

            line.clear();
            word.erase(0, 2);
        }

        if (line.length() + word.length() > line_length)
        {
            line.erase(line.size() - 1);
            vec.push_back(line);
            line.clear();
        }

        line += word + ' ';
    }

    if (!line.empty())
    {
        line.erase(line.size() - 1);
        vec.push_back(line);
    }

    return vec;
}

const char* skipWhiteSpaceAndOneOfThese(const char* s, const char* charsToSkipOnce)
{
    bool delimiter_already_skipped = false;
    for (; *s != '\0'; ++s)
    {
        if (isspace(*s))
        {
            continue;
        }

        if (delimiter_already_skipped == false && charsToSkipOnce != nullptr && std::strchr(charsToSkipOnce, *s) != nullptr)
        {
            delimiter_already_skipped = true;
            continue;
        }

        return s;
    }

    return s;
}

const wchar_t* skipWhiteSpaceAndOneOfThese(const wchar_t* s, const wchar_t* charsToSkipOnce)
{
    bool delimiteralreadyskipped = false;
    for (; *s != L'\0'; ++s)
    {
        if (iswspace(*s))
        {
            continue;
        }

        if (delimiteralreadyskipped == false && charsToSkipOnce != nullptr && std::wcschr(charsToSkipOnce, *s) != nullptr)
        {
            delimiteralreadyskipped = true;
            continue;
        }

        return s;
    }

    return s;
}

std::ostream& operator<<(std::ostream& os, const libCZI::GUID& guid)
{
    os << std::uppercase;
    os.width(8);
    os.fill('0');
    os << std::hex << guid.Data1 << '-';

    os.width(4);
    os << std::hex << guid.Data2 << '-';

    os.width(4);
    os << std::hex << guid.Data3 << '-';

    // editorconfig-checker-disable
    os << std::hex
        << std::setw(2) << static_cast<short>(guid.Data4[0])
        << std::setw(2) << static_cast<short>(guid.Data4[1])
        << '-'
        << std::setw(2) << static_cast<short>(guid.Data4[2])
        << std::setw(2) << static_cast<short>(guid.Data4[3])
        << std::setw(2) << static_cast<short>(guid.Data4[4])
        << std::setw(2) << static_cast<short>(guid.Data4[5])
        << std::setw(2) << static_cast<short>(guid.Data4[6])
        << std::setw(2) << static_cast<short>(guid.Data4[7]);
    // editorconfig-checker-enable
    os << std::nouppercase;
    return os;
}

/// Attempts to parse a GUID from the given std::wstring. The string has to have the form 
/// "cfc4a2fe-f968-4ef8-b685-e73d1b77271a" or "{cfc4a2fe-f968-4ef8-b685-e73d1b77271a}".
///
/// \param          str     The string.
/// \param [in,out] outGuid If non-null, the Guid will be put here if successful.
///
/// \return True if it succeeds, false if it fails.
bool TryParseGuid(const std::wstring& str, libCZI::GUID* outGuid)
{
    auto strTrimmed = trim(str);
    if (strTrimmed.empty() || strTrimmed.length() < 2)
    {
        return false;
    }

    if (strTrimmed[0] == L'{' && strTrimmed[strTrimmed.length() - 1] == L'}')
    {
        strTrimmed = strTrimmed.substr(1, strTrimmed.length() - 2);
    }

    std::wregex guidRegex(LR"([0-9A-Fa-f]{8}[-]([0-9A-Fa-f]{4}[-]){3}[0-9A-Fa-f]{12})");
    if (std::regex_match(strTrimmed, guidRegex))
    {
        libCZI::GUID g;
        uint32_t value;
        char sz[9];
        for (int i = 0; i < 8; ++i)
        {
            sz[i] = static_cast<char>(strTrimmed[i]);
        }

        sz[8] = '\0';
        bool b = ConvertHexStringToInteger(sz, &value);
        if (!b) { return false; }
        g.Data1 = value;

        for (int i = 0; i < 4; ++i)
        {
            sz[i] = (char)strTrimmed[i + 9];
        }

        sz[4] = '\0';
        b = ConvertHexStringToInteger(sz, &value);
        if (!b) { return false; }
        g.Data2 = static_cast<unsigned short>(value);

        for (int i = 0; i < 4; ++i)
        {
            sz[i] = (char)strTrimmed[i + 14];
        }

        b = ConvertHexStringToInteger(sz, &value);
        if (!b) { return false; }
        g.Data3 = static_cast<unsigned short>(value);

        sz[2] = '\0';
        static const uint8_t positions[] = { 19,21,24,26,  28,30,32,34 };

        for (int p = 0; p < 8; ++p)
        {
            for (int i = 0; i < 2; ++i)
            {
                sz[i] = static_cast<char>(strTrimmed[i + positions[p]]);
            }

            b = ConvertHexStringToInteger(sz, &value);
            if (!b) { return false; }
            g.Data4[p] = static_cast<unsigned char>(value);
        }

        if (outGuid != nullptr)
        {
            *outGuid = g;
        }

        return true;
    }

    return false;
}

#if CZICMD_WINDOWSAPI_AVAILABLE
CommandlineArgsWindowsHelper::CommandlineArgsWindowsHelper()
{
    int number_arguments;
    const unique_ptr<LPWSTR, decltype(LocalFree)*> wide_argv
    {
        CommandLineToArgvW(GetCommandLineW(), &number_arguments),
        &LocalFree
    };

    this->pointers_to_arguments_.reserve(number_arguments);
    this->arguments_.reserve(number_arguments);

    for (int i = 0; i < number_arguments; ++i)
    {
        this->arguments_.emplace_back(convertToUtf8(wide_argv.get()[i]));
    }

    for (int i = 0; i < number_arguments; ++i)
    {
        this->pointers_to_arguments_.emplace_back(
            const_cast<char*>(this->arguments_[i].c_str()));
    }
}

char** CommandlineArgsWindowsHelper::GetArgv()
{
    return this->pointers_to_arguments_.data();
}

int CommandlineArgsWindowsHelper::GetArgc()
{
    return static_cast<int>(this->pointers_to_arguments_.size());
}
#endif
