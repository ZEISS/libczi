//******************************************************************************
// 
// libCZIrw is a reader// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "consoleio.h"
#include "cmdlineoptions.h"
#include "execute.h"
#include "inc_libCZI.h"

#if defined(LINUXENV)
#include <clocale>
#endif

#if defined(WIN32ENV)
#define NOMINMAX
#include <Windows.h>
#endif

class CLibCZISite : public libCZI::ISite
{
		libCZI::ISite* pSite;
		const CCmdLineOptions& options;
public:
		explicit CLibCZISite(const CCmdLineOptions& opts) : options(opts)
		{
#if defined(WIN32ENV)
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

		std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t extraRows, std::uint32_t extraColumns) override
		{
				return this->pSite->CreateBitmap(pixeltype, width, height, stride, extraRows, extraColumns);
		}
};

int main(int argc, char** _argv)
{
#if defined(WIN32ENV)
		/*wchar_t** argv;
		CoInitialize(NULL);
		argv = CommandLineToArgvW(GetCommandLineW(), &argc);*/
		CommandlineArgsWindowsHelper args_helper;
#endif
#if defined(LINUXENV)
		setlocale(LC_CTYPE, "");
		char** argv = _argv;
#endif

		int retVal = 0;
		auto log = CConsoleLog::CreateInstance();
		try
		{
				CCmdLineOptions options(log);
#if defined(WIN32ENV)
				bool cmdLineParsedOk = options.Parse(args_helper.GetArgc(), args_helper.GetArgv());
#else
				bool cmdLineParsedOk = options.Parse(argc, argv);
#endif
				if (cmdLineParsedOk == true)
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
				else
				{
						log->WriteLineStdErr("There were errors parsing the arguments -> exiting.");
				}
		}
		catch (std::exception& excp)
		{
				std::stringstream ss;
				ss << "Exception caught -> \"" << excp.what() << "\"";
				log->WriteLineStdErr(ss.str());
				retVal = 1000;
		}

#if defined(WIN32ENV)
		CoUninitialize();
		//LocalFree(argv);
#endif

		return retVal;
}
