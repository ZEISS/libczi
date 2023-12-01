// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "../libCZI_StreamsLib.h"
#include <libCZI_Config.h>
#include <memory>
#include <mutex>
#include "curlhttpinputstream.h"
#include "windowsfileinputstream.h"
#include "simplefileinputstream.h"
#include "preadfileinputstream.h"
#include "../utilities.h"

using namespace libCZI;

/*static*/const char* StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaInfo = "CurlHttp_CaInfo";
/*static*/const char* StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaPath = "CurlHttp_CaPath";

static const struct
{
    StreamsFactory::StreamClassInfo stream_class_info;

    /// Function for creating and initializing the stream-instance. Note that we have two variants here, one for UTF-8 and one for wide string.
    /// Only one must be provided (the other one can be nullptr) - in which case the string is converted to the other format by the caller.
    /// By calling the appropriate function, the caller can avoid the conversion.
    std::shared_ptr<libCZI::IStream>(*pfn_create_stream_utf8)(const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name);

    /// Function for creating and initializing the stream-instance. Note that we have two variants here, one for UTF-8 and one for wide string.
    /// Only one must be provided (the other one can be nullptr) - in which case the string is converted to the other format by the caller.
    /// By calling the appropriate function, the caller can avoid the conversion.
    std::shared_ptr<libCZI::IStream>(*pfn_create_stream_wide)(const StreamsFactory::CreateStreamInfo& stream_info, const std::wstring& file_name);
} stream_classes[] =
{
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
        {
            { "curl_http_inputstream", "curl-based http/https stream", CurlHttpInputStream::GetBuildInformation, CurlHttpInputStream::GetClassProperty },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<CurlHttpInputStream>(file_name, stream_info.property_bag);
            },
            nullptr
        },
#endif  // LIBCZI_CURL_BASED_STREAM_AVAILABLE
#if _WIN32
        {
            { "windows_file_inputstream", "stream implementation based on Windows-API", nullptr, nullptr },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<WindowsFileInputStream>(file_name);
            },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::wstring& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<WindowsFileInputStream>(file_name.c_str());
            }
        },
#endif  // _WIN32
#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
        {
            { "pread_file_inputstream", "stream implementation based on pread-API", nullptr, nullptr },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<PreadFileInputStream>(file_name);
            },
            nullptr
        },
#endif // LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
        {
            { "c_runtime_file_inputstream", "stream implementation based on C-runtime library", nullptr, nullptr },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<SimpleFileInputStream>(file_name);
            },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::wstring& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<SimpleFileInputStream>(file_name.c_str());
            }
        },
};

static std::once_flag streams_factory_already_initialized;

void libCZI::StreamsFactory::Initialize()
{
    std::call_once(streams_factory_already_initialized,
                []()
                {
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
                    // TODO(JBL): we could choose the SSL-backend here (https://curl.se/libcurl/c/curl_global_sslset.html)
                    CurlHttpInputStream::OneTimeGlobalCurlInitialization();
#endif
                });
}

bool libCZI::StreamsFactory::GetStreamInfoForClass(int index, StreamClassInfo& stream_info)
{
    if (index < 0 || index >= GetStreamClassesCount())
    {
        return false;
    }

    stream_info = stream_classes[index].stream_class_info;
    return true;
}

int libCZI::StreamsFactory::GetStreamClassesCount()
{
    return sizeof(stream_classes) / sizeof(stream_classes[0]);
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateStream(const CreateStreamInfo& stream_info, const std::string& file_identifier)
{
    for (int i = 0; i < StreamsFactory::GetStreamClassesCount(); ++i)
    {
        if (stream_info.class_name == stream_classes[i].stream_class_info.class_name)
        {
            if (stream_classes[i].pfn_create_stream_utf8)
            {
                return stream_classes[i].pfn_create_stream_utf8(stream_info, file_identifier);
            }
            else if (stream_classes[i].pfn_create_stream_wide)
            {
                return stream_classes[i].pfn_create_stream_wide(stream_info, Utilities::convertUtf8ToWchar_t(file_identifier.c_str()));
            }

            break;
        }
    }

    return {};
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateStream(const CreateStreamInfo& stream_info, const std::wstring& file_identifier)
{
    for (int i = 0; i < StreamsFactory::GetStreamClassesCount(); ++i)
    {
        if (stream_info.class_name == stream_classes[i].stream_class_info.class_name)
        {
            if (stream_classes[i].pfn_create_stream_wide)
            {
                return stream_classes[i].pfn_create_stream_wide(stream_info, file_identifier);
            }
            else if (stream_classes[i].pfn_create_stream_utf8)
            {
                return stream_classes[i].pfn_create_stream_utf8(stream_info, Utilities::convertWchar_tToUtf8(file_identifier.c_str()));
            }

            break;
        }
    }

    return {};
}

template<typename t_charactertype>
std::shared_ptr<libCZI::IStream> CreateDefaultStreamForFileGeneric(const t_charactertype* filename)
{
#if _WIN32
    // if we are on Windows, then "WindowsFileInputStream" should give best performance/features
    return std::make_shared<WindowsFileInputStream>(filename);
#elif LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
    // otherwise (and if pread is available), then "PreadFileInputStream" is the best choice
    return std::make_shared<PreadFileInputStream>(filename);
#endif

    // otherwise... we fall back to the simple fseek/fread-based one
    return std::make_shared<SimpleFileInputStream>(filename);
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateDefaultStreamForFile(const char* filename)
{
    return CreateDefaultStreamForFileGeneric<char>(filename);
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateDefaultStreamForFile(const wchar_t* filename)
{
    return CreateDefaultStreamForFileGeneric<wchar_t>(filename);
}
