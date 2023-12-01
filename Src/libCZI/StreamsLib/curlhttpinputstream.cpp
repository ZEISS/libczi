// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

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

/*static*/std::string CurlHttpInputStream::GetBuildInformation()
{
    const auto version_info = curl_version_info(CURLVERSION_NOW);

    stringstream string_stream;
    string_stream << "Version:" << version_info->version;
    string_stream << " SSL:" << (version_info->ssl_version != nullptr ? version_info->ssl_version : "none");
    if (version_info->age >= CURLVERSION_ELEVENTH)
    {
        for (size_t i = 0;; ++i)
        {
            if (version_info->feature_names[i] != nullptr)
            {
                if (i == 0)
                {
                    string_stream << " Features:";
                }
                else
                {
                    string_stream << ',';
                }

                string_stream << version_info->feature_names[i];
            }
            else
            {
                break;
            }
        }
    }

    return string_stream.str();
}

/*static*/libCZI::StreamsFactory::Property CurlHttpInputStream::GetClassProperty(const char* property_name)
{
    if (property_name != nullptr)
    {
        if (strcmp(property_name, StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaInfo) == 0)
        {
            const auto version_info = curl_version_info(CURLVERSION_NOW);
            if (version_info->cainfo != nullptr)
            {
                return StreamsFactory::Property(version_info->cainfo);
            }
        }
        else if (strcmp(property_name, StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaPath) == 0)
        {
            const auto version_info = curl_version_info(CURLVERSION_NOW);
            if (version_info->capath != nullptr)
            {
                return StreamsFactory::Property(version_info->capath);
            }
        }
    }

    return {};
}

CurlHttpInputStream::CurlHttpInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    /* init the curl session */
    CURL* curl_handle = curl_easy_init();
    if (curl_handle == nullptr)
    {
        throw std::runtime_error("curl_easy_init() failed");
    }

    // put the handle into a smart-pointer with a custom deleter (so that we don't have to worry about calling curl_easy_cleanup in the lines below in case of error)
    unique_ptr<CURL, void(*)(CURL*)> up_curl_handle(curl_handle, [](CURL* h)->void {curl_easy_cleanup(h); });

    // construct a "curl-URL-handle" (an object that can be used to parse and manipulate URLs) -> https://curl.se/libcurl/c/libcurl-url.html
    CURLU* curl_url_handle = curl_url();
    if (curl_url_handle == nullptr)
    {
        throw std::runtime_error("curl_url() failed");
    }

    // put the url-handle also into a smart-pointer with a custom deleter
    unique_ptr<CURLU, void(*)(CURLU*)> up_curl_url_handle(curl_url_handle, [](CURLU* h)->void {curl_url_cleanup(h); });

    // Now, we parse the URL provided - the benefit of doing it this way is that at this point the URL is parsed for 
    //  syntactical correctness, and we can be sure that the URL is valid. If we simply set the URL with CURLOPT_URL,
    //  this would happen later on (when actually performing the request). I'd guess it is beneficial to catch this
    //  kind of error early on.
    // TODO(JBL): - we may want to restrict the schemes that are allowed 
    //            - dealing with non-ASCII characters in the URL may be tricky (https://curl.se/libcurl/c/CURLOPT_URL.html, https://curl.se/libcurl/c/curl_url_set.html)
    // Regarding the topic of "sanitizing the URL" - currently, we just pass the string we get to libcurl as is, and
    // the string we get here is conceptually in UTF-8 encoding. So, we rely on that the caller has made all necessary
    // adjustments before passing in the URL. Should necessity arise, it might be a good idea to revisit this topic
    // (and maybe use libcurl respective functionality for URL-sanitization here).
    // Btw, this topic of sanitization may also arise with some of the items from the property-bag (e.g. 'kCurlHttp_UserAgent').
    const CURLUcode return_code_url = curl_url_set(up_curl_url_handle.get(), CURLUPART_URL, url.c_str(), 0);
    if (return_code_url != CURLUE_OK)
    {
        stringstream string_stream;
        string_stream << "curl_url_set(..) failed with error code " << return_code_url << " (" << curl_url_strerror(return_code_url) << ")";
        throw std::runtime_error(string_stream.str());
    }

    CURLcode return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_CURLU, up_curl_url_handle.get());
    ThrowIfCurlSetOptError(return_code, "CURLOPT_CURLU");

    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_VERBOSE, 0L/*1L*/);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_VERBOSE");

    /* disable progress meter, set to 0L to enable it */
    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_NOPROGRESS, 1L);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_NOPROGRESS");

    return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_TCP_KEEPALIVE, 1L);
    ThrowIfCurlSetOptError(return_code, "CURLOPT_TCP_KEEPALIVE");

    // set the "write function" - to this function curl will deliver the payload data
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

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_FollowLocation);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_FOLLOWLOCATION, property->second.GetAsBoolOrThrow() ? 1L : 0L);
        ThrowIfCurlSetOptError(return_code, "CURLOPT_FOLLOWLOCATION");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_MaxRedirs);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_MAXREDIRS, property->second.GetAsInt32OrThrow());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_MAXREDIRS");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_CaInfo);
    if (property != property_bag.end())
    {
        return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_CAINFO, property->second.GetAsStringOrThrow().c_str());
        ThrowIfCurlSetOptError(return_code, "CURLOPT_CAINFO");
    }

    property = property_bag.find(StreamsFactory::StreamProperties::kCurlHttp_CaInfoBlob);
    if (property != property_bag.end())
    {
        string ca_info_blob = property->second.GetAsStringOrThrow();
        if (!ca_info_blob.empty())
        {
            struct curl_blob blob;
            blob.data = &ca_info_blob[0];
            blob.len = ca_info_blob.size();
            blob.flags = CURL_BLOB_COPY;
            return_code = curl_easy_setopt(up_curl_handle.get(), CURLOPT_CAINFO_BLOB, &blob);
            ThrowIfCurlSetOptError(return_code, "CURLOPT_CAINFO_BLOB");
        }
    }

    this->curl_handle_ = up_curl_handle.release();
    this->curl_url_handle_ = up_curl_url_handle.release();
}

/*virtual*/void CurlHttpInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    stringstream ss;
    ss << offset << "-" << offset + size - 1;

    {
        std::lock_guard<std::mutex> lck(this->request_mutex_);

        // TODO(JBL): We may be able to use a "header-function" (https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html) in order to find out
        //             whether the server accepted our "Range-Request". According to https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests,
        //             we can expect to have a line "something like 'Accept-Ranges: bytes'" in the response header with a server that supports range
        //             requests (and a line 'Accept-Ranges: none') would tell us explicitly that range requests are *not* supported.

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
            ss = stringstream{};
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

    if (this->curl_url_handle_ != nullptr)
    {
        curl_url_cleanup(this->curl_url_handle_);
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
