#include "../include/streamsFactory.h"
#include "curlhttpinputstream.h"
#include <memory>

using namespace libCZIStreamsLib;

static const struct 
{
    StreamClassInfo stream_class_info;
    std::shared_ptr<libCZI::IStream>(*pfn_create_stream)(const CreateStreamInfo& stream_info);
} stream_classes[] =
{
    {
        { "curl_http_inputstream", "curl-based http/https stream" },
        [](const CreateStreamInfo& stream_info) -> std::shared_ptr<libCZI::IStream>
        {
            return std::make_shared<CurlHttpInputStream>(stream_info.filename, stream_info.property_bag);
        }
    }
};

bool libCZIStreamsLib::GetStreamInfoForClass(int index, StreamClassInfo& stream_info)
{
    if (index < 0 || index >= GetStreamInfoCount())
    {
        return false;
    }

    stream_info = stream_classes[index].stream_class_info;
    return true;
}

int libCZIStreamsLib::GetStreamInfoCount()
{
    return sizeof(stream_classes) / sizeof(stream_classes[0]);
}

std::shared_ptr<libCZI::IStream> libCZIStreamsLib::CreateStream(const CreateStreamInfo& stream_info)
{
    for (int i= 0; i < GetStreamInfoCount(); ++i)
    {
        if (stream_info.class_name == stream_classes[i].stream_class_info.class_name)
        {
            return stream_classes[i].pfn_create_stream(stream_info);
        }
    }

    return {};
}
