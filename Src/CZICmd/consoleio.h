// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <string>
#include <memory>

class ILog
{
public:
    virtual void WriteLineStdOut(const char* sz) = 0;
    virtual void WriteLineStdOut(const wchar_t* sz) = 0;
    virtual void WriteLineStdErr(const char* sz) = 0;
    virtual void WriteLineStdErr(const wchar_t* sz) = 0;

    virtual void WriteStdOut(const char* sz) = 0;
    virtual void WriteStdOut(const wchar_t* sz) = 0;
    virtual void WriteStdErr(const char* sz) = 0;
    virtual void WriteStdErr(const wchar_t* sz) = 0;

    void WriteLineStdOut(const std::string& str)
    {
        this->WriteLineStdOut(str.c_str());
    }

    void WriteLineStdOut(const std::wstring& str)
    {
        this->WriteLineStdOut(str.c_str());
    }

    void WriteLineStdErr(const std::string& str)
    {
        this->WriteLineStdErr(str.c_str());
    }

    void WriteLineStdErr(const std::wstring& str)
    {
        this->WriteLineStdErr(str.c_str());
    }

    void WriteStdOut(const std::string& str)
    {
        this->WriteStdOut(str.c_str());
    }

    void WriteStdOut(const std::wstring& str)
    {
        this->WriteStdOut(str.c_str());
    }

    void WriteStdErr(const std::string& str)
    {
        this->WriteStdErr(str.c_str());
    }

    void WriteStdErr(const std::wstring& str)
    {
        this->WriteStdErr(str.c_str());
    }

    virtual ~ILog() = default;
};

class CConsoleLog : public ILog
{
public:
    static std::shared_ptr<ILog> CreateInstance();

    void WriteLineStdOut(const char* sz) override;
    void WriteLineStdOut(const wchar_t* sz) override;
    void WriteLineStdErr(const char* sz) override;
    void WriteLineStdErr(const wchar_t* sz) override;

    void WriteStdOut(const char* sz) override;
    void WriteStdOut(const wchar_t* sz) override;
    void WriteStdErr(const char* sz) override;
    void WriteStdErr(const wchar_t* sz) override;
};
