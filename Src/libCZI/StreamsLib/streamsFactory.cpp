#include "../libCZI_StreamsLib.h"
#include <libCZI_Config.h>
#include "curlhttpinputstream.h"
#include "windowsfileinputstream.h"
#include "simplefileinputstream.h"
#include "../utilities.h"
#include <memory>

using namespace libCZI;

static const struct
{
    StreamsFactory::StreamClassInfo stream_class_info;
    std::shared_ptr<libCZI::IStream>(*pfn_create_stream)(const StreamsFactory::CreateStreamInfo& stream_info);
} stream_classes[] =
{
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
        {
            { "curl_http_inputstream", "curl-based http/https stream" },
            [](const StreamsFactory::CreateStreamInfo& stream_info) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<CurlHttpInputStream>(stream_info.filename, stream_info.property_bag);
            }
        },
#endif
#if _WIN32
        {
            { "windows_file_inputstream", "stream implementation based on Windows-API" },
            [](const StreamsFactory::CreateStreamInfo& stream_info) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<WindowsFileInputStream>(stream_info.filename);
            }
        },
#endif
        {
            { "c_runtime_file_inputstream", "stream implementation based on C-runtime library" },
            [](const StreamsFactory::CreateStreamInfo& stream_info) -> std::shared_ptr<libCZI::IStream>
            {
                return std::make_shared<SimpleFileInputStream>(stream_info.filename);
            }
        },

};

void libCZI::StreamsFactory::Initialize()
{
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
    // TODO(JBL): we could choose the SSL-backend here (https://curl.se/libcurl/c/curl_global_sslset.html)
    CurlHttpInputStream::OneTimeGlobalCurlInitialization();
#endif
}

bool libCZI::StreamsFactory::GetStreamInfoForClass(int index, StreamClassInfo& stream_info)
{
    if (index < 0 || index >= GetStreamInfoCount())
    {
        return false;
    }

    stream_info = stream_classes[index].stream_class_info;
    return true;
}

int libCZI::StreamsFactory::GetStreamInfoCount()
{
    return sizeof(stream_classes) / sizeof(stream_classes[0]);
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateStream(const CreateStreamInfo& stream_info)
{
    for (int i = 0; i < StreamsFactory::GetStreamInfoCount(); ++i)
    {
        if (stream_info.class_name == stream_classes[i].stream_class_info.class_name)
        {
            return stream_classes[i].pfn_create_stream(stream_info);
        }
    }

    return {};
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateDefaultStreamForFile(const char* filename)
{
#if _WIN32
    return std::make_shared<WindowsFileInputStream>(filename);
#endif

    return std::make_shared<SimpleFileInputStream>(filename);
}

std::shared_ptr<libCZI::IStream> libCZI::StreamsFactory::CreateDefaultStreamForFile(const wchar_t* filename)
{
#if _WIN32
    return std::make_shared<WindowsFileInputStream>(filename);
#endif

    return std::make_shared<SimpleFileInputStream>(filename);
}
