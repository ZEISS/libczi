// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_CURL_BASED_STREAM_AVAILABLE

#include <mutex>
#include <cstdint>
#include <string>
#include "../libCZI.h"
#include <curl/curl.h>

/// A simplistic implementation of a stream which uses the curl library to read from an http or https stream.
/// It uses the libcurl-easy-interface and is operating in a blocking mode and serialized (i.e. only one request at a time).
class CurlHttpInputStream : public libCZI::IStream
{
private:
    CURL* curl_handle_{nullptr};        ///< The curl-handle.
    CURLU* curl_url_handle_{nullptr};   ///< The curl-url-handle.
    std::mutex request_mutex_;          ///< Mutex to serialize the requests.
public:
    CurlHttpInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    ~CurlHttpInputStream() override;

    static void OneTimeGlobalCurlInitialization();

    static std::string GetBuildInformation();
    static libCZI::StreamsFactory::Property GetClassProperty(const char* property_name);
private:
    /// This struct is passed to the WriteData function as user-data.
    struct WriteDataContext
    {
        void* data{nullptr};                         ///< Pointer to the destination buffer (where the data is to be delivered to).
        std::uint64_t size{0};                       ///< The size of the destination buffer.
        std::uint64_t count_data_received{ 0 };      ///< The number of bytes received so far.
    };

    /// This function is the write-callback-function used by curl to write the data into the buffer.
    /// C.f. https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html for more information.
    /// Note that this function is usually called multiple times for a single request, where each invocation
    /// delivers a another chunk of data.
    ///
    /// \param [in]     ptr         Pointer to the data.
    /// \param          size        The size of an element (of the data) in bytes. This is documented to be always 1.
    /// \param          nmemb       The number of 'elements'.
    /// \param [in]     user_data   The user-data pointer (which was set with the CURLOPT_WRITEDATA option).
    ///
    /// \returns    The  number of bytes actually taken care of. If that amount differs from the amount passed to your 
    ///             callback function, it signals an error condition to the library. This causes the transfer to get 
    ///             aborted and the libcurl function used returns CURLE_WRITE_ERROR. One  can also abort the transfer 
    ///             by returning CURL_WRITEFUNC_ERROR (added in 7.87.0), which makes CURLE_WRITE_ERROR get returned.
    static size_t WriteData(void* ptr, size_t size, size_t nmemb, void* user_data);

    static void ThrowIfCurlSetOptError(CURLcode return_code, const char* curl_option_name);
};

#endif
