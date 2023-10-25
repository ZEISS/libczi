#pragma once

#include "../include/streamsFactory.h"
#include <curl/curl.h>

class CurlHttpInputStream : public libCZI::IStream
{
private:
    CURL* curl_handle;  
public:
    CurlHttpInputStream(const std::string& url, const std::map<int, libCZIStreamsLib::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    ~CurlHttpInputStream() override;
private:
    struct WriteDataContext
    {
        void* ptr_data;
        std::uint64_t size;
    };

    static size_t Write_data(void* ptr, size_t size, size_t nmemb, void* stream);
};
