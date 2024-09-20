// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "consoleio.h"
#include <iostream>

using namespace std;

/*static*/std::shared_ptr<ILog> CConsoleLog::CreateInstance()
{
    return std::make_shared<CConsoleLog>();
}

void CConsoleLog::WriteLineStdOut(const char* sz)
{
    std::cout << sz << endl;
}

void CConsoleLog::WriteLineStdOut(const wchar_t* sz)
{
    std::wcout << sz << endl;
}

void CConsoleLog::WriteLineStdErr(const char* sz)
{
    std::cout << sz << endl;
}

void CConsoleLog::WriteLineStdErr(const wchar_t* sz)
{
    std::wcout << sz << endl;
}

void CConsoleLog::WriteStdOut(const char* sz)
{
    std::cout << sz;
}

void CConsoleLog::WriteStdOut(const wchar_t* sz)
{
    std::wcout << sz;
}

void CConsoleLog::WriteStdErr(const char* sz)
{
    std::cout << sz;
}

void CConsoleLog::WriteStdErr(const wchar_t* sz)
{
    std::wcout << sz;
}
