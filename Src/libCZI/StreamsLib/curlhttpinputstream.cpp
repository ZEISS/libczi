#include "curlhttpinputstream.h"
#if LIBCZI_CURL_BASED_STREAM_AVAILABLE
#include <cstdint>
#include <string>
#include <sstream>
#include "../libCZI_StreamsLib.h"
#include <curl/curl.h>

using namespace std;
using namespace libCZI;

/*static*/void CurlHttpInputStream::OneTimeGlobalCurlInitialization()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

CurlHttpInputStream::CurlHttpInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    /* init the curl session */
    const HANDLE curl_handle = curl_easy_init();
    if (curl_handle == nullptr)
    {
        throw std::runtime_error("curl_easy_init() failed");
    }

    // put the handle into a smart-pointer with a custom deleter (so that we don't have to worry about calling curl_easy_cleanup in the lines below in case of error)
    unique_ptr<CURL, void(*)(CURL*)> up_curl_handle(curl_handle, [](CURL* h)->void {curl_easy_cleanup(h); });

    /* set URL to get here */
    CURLcode return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_URL, url.c_str());
    ThrowIfCurlSetOptError(return_code, "CURLOPT_URL");

    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_VERBOSE, 0L/*1L*/);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_VERBOSE");

    /* disable progress meter, set to 0L to enable it */
    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_NOPROGRESS, 1L);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_NOPROGRESS");

    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_TCP_KEEPALIVE, 1L);
    //ThrowIfCurlSetOptError(return_code, "CURLOPT_TCP_KEEPALIVE");

    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_WRITEFUNCTION, CurlHttpInputStream::WriteData);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_WRITEFUNCTION");

    auto property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_Proxy);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_PROXY, property->second.GetAsStringOrThrow().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_PROXY");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_UserAgent);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_USERAGENT, property->second.GetAsStringOrThrow().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_USERAGENT");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_Timeout);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_TIMEOUT, static_cast<long>(property->second.GetAsInt32OrThrow()));
        ThrowIfCurlSetOptError(return_code, "CURLOPT_TIMEOUT");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_ConnectTimeout);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_CONNECTTIMEOUT, static_cast<long>(property->second.GetAsInt32OrThrow()));
        ThrowIfCurlSetOptError(return_code, "CURLOPT_CONNECTTIMEOUT");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_Xoauth2Bearer);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_XOAUTH2_BEARER, property->second.GetAsStringOrThrow().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_XOAUTH2_BEARER");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_Cookie);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_COOKIE, property->second.GetAsStringOrThrow().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_COOKIE");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_SslVerifyPeer);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_SSL_VERIFYPEER, property->second.GetAsBoolOrThrow() ? 1L : 0L);
        ThrowIfCurlSetOptError(return_code, "CURLOPT_SSL_VERIFYPEER");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_SslVerifyHost);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_SSL_VERIFYHOST, property->second.GetAsBoolOrThrow() ? 1L : 0L);
        ThrowIfCurlSetOptError(return_code, "CURLOPT_SSL_VERIFYHOST");
    }

    // curl_easy_setopt(up_curl_handle.get(), CURLOPT_FOLLOWLOCATION, 0L);
     //curl_easy_setopt(up_curl_handle.get(), CURLOPT_PROXY, "https://127.0.0.1:8080");

     //curl_easy_setopt(up_curl_handle.get(), CURLOPT_SSL_VERIFYPEER, 0L);

    this->curl_handle_ = up_curl_handle.release();
}

/*virtual*/void CurlHttpInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    stringstream ss;
    ss << offset << "-" << offset + size - 1;

    {
        std::lock_guard<std::mutex> lck(this->request_mutex_);

        // https://curl.se/libcurl/c/CURLOPT_RANGE.html states that the range may be ignored by the server, and it would then
        //  deliver the entire document. And, it says, that there is no way to detect that the range was ignored. We take precautions
        //  that we only accept as many bytes as we have requested, and otherwise the "curl_easy_perform" should report an error.
        CURLcode return_code = curl_easy_setopt(curl_handle_, CURLOPT_RANGE, ss.str().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_RANGE");

        WriteDataContext write_data_context;
        write_data_context.data = pv;
        write_data_context.size = size;
        return_code = curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &write_data_context);
        ThrowIfCurlSetOptError(return_code, "CURLOPT_WRITEDATA");

        return_code = curl_easy_perform(this->curl_handle_);
        if (return_code != CURLE_OK)
        {
            stringstream ss;
            ss << "curl_easy_perform() failed with error code " << return_code << " (" << curl_easy_strerror(return_code) << ")";
            throw runtime_error(ss.str());
        }

        if (ptrBytesRead != nullptr)
        {
            *ptrBytesRead = write_data_context.count_data_received;
        }
    }
}

CurlHttpInputStream::~CurlHttpInputStream()
{
    if (this->curl_handle_ != nullptr)
    {
        curl_easy_cleanup(this->curl_handle_);
    }
}

/*static*/size_t CurlHttpInputStream::WriteData(void* ptr, size_t size, size_t nmemb, void* user_data)
{
    WriteDataContext* write_data_context = static_cast<WriteDataContext*>(user_data);
    const size_t total_size = size * nmemb;

    if (write_data_context->count_data_received + total_size > write_data_context->size)
    {
        return CURLE_WRITE_ERROR;
    }

    memcpy(
        static_cast<uint8_t*>(write_data_context->data) + write_data_context->count_data_received,
        ptr,
        total_size);

    write_data_context->count_data_received += total_size;
    return total_size;
}

void CurlHttpInputStream::ThrowIfCurlSetOptError(CURLcode return_code, const char* curl_option_name)
{
    if (return_code != CURLE_OK)
    {
        stringstream ss;
        ss << "curl_easy_setopt(" << curl_option_name << ") failed with error code " << return_code << " (" << curl_easy_strerror(return_code) << ")";
        throw std::runtime_error(ss.str());
    }
}
#endif