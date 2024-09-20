// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "../libCZI_StreamsLib.h"
#include <libCZI_Config.h>
#include <memory>
#include <mutex>
#include <assert.h>
#include "curlhttpinputstream.h"
#include "windowsfileinputstream.h"
#include "simplefileinputstream.h"
#include "preadfileinputstream.h"
#include "azureblobinputstream.h"
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
#if LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE
        {
            { "azure_blob_inputstream", "Azure-SDK-based stream", AzureBlobInputStream::GetBuildInformation, nullptr },
            [](const StreamsFactory::CreateStreamInfo& stream_info, const std::string& file_name) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<AzureBlobInputStream>(file_name, stream_info.property_bag);
            },
            nullptr
        },
#endif  // LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE
#if LIBCZI_WINDOWSAPI_AVAILABLE
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
#endif  // LIBCZI_WINDOWSAPI_AVAILABLE
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

/*static*/const libCZI::StreamsFactory::StreamPropertyBagPropertyInfo* libCZI::StreamsFactory::GetStreamPropertyBagPropertyInfo(int* count)
{
    static const StreamsFactory::StreamPropertyBagPropertyInfo kStreamPropertyBagPropertyInfo[] =
    {
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
        {"CurlHttp_Proxy", StreamsFactory::StreamProperties::kCurlHttp_Proxy, StreamsFactory::Property::Type::String},
        {"CurlHttp_UserAgent", StreamsFactory::StreamProperties::kCurlHttp_UserAgent, StreamsFactory::Property::Type::String},
        {"CurlHttp_Timeout", StreamsFactory::StreamProperties::kCurlHttp_Timeout, StreamsFactory::Property::Type::Int32},
        {"CurlHttp_ConnectTimeout", StreamsFactory::StreamProperties::kCurlHttp_ConnectTimeout, StreamsFactory::Property::Type::Int32},
        {"CurlHttp_Xoauth2Bearer", StreamsFactory::StreamProperties::kCurlHttp_Xoauth2Bearer, StreamsFactory::Property::Type::String},
        {"CurlHttp_Cookie", StreamsFactory::StreamProperties::kCurlHttp_Cookie, StreamsFactory::Property::Type::String},
        {"CurlHttp_SslVerifyPeer", StreamsFactory::StreamProperties::kCurlHttp_SslVerifyPeer, StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_SslVerifyHost", StreamsFactory::StreamProperties::kCurlHttp_SslVerifyHost, StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_FollowLocation", StreamsFactory::StreamProperties::kCurlHttp_FollowLocation, StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_MaxRedirs", StreamsFactory::StreamProperties::kCurlHttp_MaxRedirs, StreamsFactory::Property::Type::Int32},
        {"CurlHttp_CaInfo", StreamsFactory::StreamProperties::kCurlHttp_CaInfo, StreamsFactory::Property::Type::String},
        {"CurlHttp_CaInfoBlob", StreamsFactory::StreamProperties::kCurlHttp_CaInfoBlob, StreamsFactory::Property::Type::String},
#endif
#if LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE
        {"AzureBlob_AuthenticationMode", StreamsFactory::StreamProperties::kAzureBlob_AuthenticationMode, StreamsFactory::Property::Type::String},
#endif
        {nullptr, 0, StreamsFactory::Property::Type::Invalid},
    };

    if (count != nullptr)
    {
        static_assert(sizeof(kStreamPropertyBagPropertyInfo) / sizeof(kStreamPropertyBagPropertyInfo[0]) > 0, "kStreamPropertyBagPropertyInfo must contain at least one element.");
        *count = sizeof(kStreamPropertyBagPropertyInfo) / sizeof(kStreamPropertyBagPropertyInfo[0]) - 1;
    }

    return kStreamPropertyBagPropertyInfo;
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
#if LIBCZI_WINDOWSAPI_AVAILABLE
    // if we are on Windows, then "WindowsFileInputStream" should give the best performance/features
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
