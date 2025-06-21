// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "inc_CZIcmd_Config.h"
#include "consoleio.h"
#include "cmdlineoptions.h"
#include "execute.h"
#include "inc_libCZI.h"
#include <clocale>
#if CZICMD_WINDOWSAPI_AVAILABLE
#include <windows.h>
#endif

class CLibCZISite : public libCZI::ISite
{
    libCZI::ISite* pSite;
    const CCmdLineOptions& options;
public:
    explicit CLibCZISite(const CCmdLineOptions& opts) : options(opts)
    {
#if CZICMD_WINDOWSAPI_AVAILABLE
        if (options.GetUseWICJxrDecoder())
        {
            this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithWICDecoder);
        }
        else
        {
            this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithJxrDecoder);
        }
#else
        this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::Default);
#endif
    }

    bool IsEnabled(int logLevel) override
    {
        return this->options.IsLogLevelEnabled(logLevel);
    }

    void Log(int level, const char* szMsg) override
    {
        this->options.GetLog()->WriteLineStdOut(szMsg);
    }

    std::shared_ptr<libCZI::IDecoder> GetDecoder(libCZI::ImageDecoderType type, const char* arguments) override
    {
        return this->pSite->GetDecoder(type, arguments);
    }

    void TerminateProgram(TerminationReason reason, const char* message) override
    {
        std::stringstream ss;
        ss << "libCZI terminated the program -> reason: " << static_cast<int>(reason) << ", message: \"" << message << "\"";
        this->options.GetLog()->WriteLineStdErr(ss.str());
        std::abort();
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t extraRows, std::uint32_t extraColumns) override
    {
        return this->pSite->CreateBitmap(pixeltype, width, height, stride, extraRows, extraColumns);
    }
};

int main(int argc, char** _argv)
{
#if CZICMD_WINDOWSAPI_AVAILABLE
    CoInitialize(NULL);
    CommandlineArgsWindowsHelper args_helper;
#else
    setlocale(LC_CTYPE, "");
#endif

    int retVal = 0;
    auto log = CConsoleLog::CreateInstance();
    try
    {
        CCmdLineOptions options(log);
#if CZICMD_WINDOWSAPI_AVAILABLE
        auto cmdLineParseResult = options.Parse(args_helper.GetArgc(), args_helper.GetArgv());
#else
        auto cmdLineParseResult = options.Parse(argc, _argv);
#endif
        if (cmdLineParseResult == CCmdLineOptions::ParseResult::OK)
        {
            if (options.GetCommand() != Command::Invalid)
            {
                // Important: We have to ensure that the object passed in here has a lifetime greater than
                // any usage of the libCZI.
                CLibCZISite site(options);
                libCZI::SetSiteObject(&site);

                execute(options);
            }
        }
        else if (cmdLineParseResult == CCmdLineOptions::ParseResult::Error)
        {
            log->WriteLineStdErr("");
            log->WriteLineStdErr("There were errors parsing the arguments -> exiting.");
            retVal = 2;
        }
        else if (cmdLineParseResult == CCmdLineOptions::ParseResult::Exit)
        {
            retVal = -1;
        }
    }
    catch (std::exception& excp)
    {
        std::stringstream ss;
        ss << "Exception caught -> \"" << excp.what() << "\"";
        log->WriteLineStdErr(ss.str());
        retVal = 1;
    }

#if CZICMD_WINDOWSAPI_AVAILABLE
    CoUninitialize();
#endif

    return retVal;
}
