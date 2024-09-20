// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_CZIcmd_Config.h"
#include <string>
#include <vector>
#include "inc_libCZI.h"


std::string convertToUtf8(const std::wstring& str);
std::wstring convertUtf8ToUCS2(const std::string& str);

std::string trim(const std::string& str, const std::string& whitespace = " \t");
std::wstring trim(const std::wstring& str, const std::wstring& whitespace = L" \t");
bool icasecmp(const std::string& l, const std::string& r);
bool icasecmp(const std::wstring& l, const std::wstring& r);
std::uint8_t HexCharToInt(char c);
bool ConvertHexStringToInteger(const char* cp, std::uint32_t* value);
char LowerNibbleToHexChar(std::uint8_t v);
char UpperNibbleToHexChar(std::uint8_t v);

std::string BytesToHexString(const std::uint8_t* ptr, size_t size);
std::wstring BytesToHexWString(const std::uint8_t* ptr, size_t size);

std::vector<std::wstring> wrap(const wchar_t* text, size_t line_length/* = 72*/);
std::vector<std::string> wrap(const char* text, size_t line_length/* = 72*/);

const wchar_t* skipWhiteSpaceAndOneOfThese(const wchar_t* s, const wchar_t* charsToSkipOnce);
const char* skipWhiteSpaceAndOneOfThese(const char* s, const char* charsToSkipOnce);

std::ostream& operator<<(std::ostream& os, const libCZI::GUID& guid);

bool TryParseGuid(const std::wstring& str, libCZI::GUID* outGuid);

/// This is an utility in order to implement a "scope guard" - an object that when gets out-of-scope is executing
/// a functor which may implement any kind of clean-up - akin to a finally-clause in C#. C.f. https://www.heise.de/blog/C-Core-Guidelines-finally-in-C-4133759.html
/// or https://blog.rnstlr.ch/c-list-of-scopeguard.html.
///
/// \tparam F   Object to be executed when leaving the scope.
template <class F>
class final_act
{
public:
    explicit final_act(F f) noexcept
        : f_(std::move(f)), invoke_(true) {}

    final_act(final_act&& other) noexcept
        : f_(std::move(other.f_)),
        invoke_(other.invoke_)
    {
        other.invoke_ = false;
    }

    final_act(const final_act&) = delete;
    final_act& operator=(const final_act&) = delete;

    ~final_act() noexcept
    {
        if (invoke_) f_();
    }

private:
    F f_;
    bool invoke_;
};

template <class F>
inline final_act<F> finally(const F& f) noexcept
{
    return final_act<F>(f);
}

template <class F>
inline final_act<F> finally(F&& f) noexcept
{
    return final_act<F>(std::forward<F>(f));
}

#if CZICMD_WINDOWSAPI_AVAILABLE
/// A utility which is providing the command-line arguments (on Windows) as UTF8-encoded strings.
class CommandlineArgsWindowsHelper
{
private:
    std::vector<std::string> arguments_;
    std::vector<char*> pointers_to_arguments_;
public:
    /// Constructor.
    CommandlineArgsWindowsHelper();

    /// Gets an array of pointers to null-terminated, UTF8-encoded arguments. This size of this array is given
    /// by the "GetArgc"-method.
    /// Note that this pointer is only valid for the lifetime of this instance of the CommandlineArgsWindowsHelper-class.
    ///
    /// \returns    Pointer to an array of pointers to null-terminated, UTF8-encoded arguments.
    char** GetArgv();

    /// Gets the number of arguments.
    ///
    /// \returns    The number of arguments.
    int GetArgc();
};
#endif
