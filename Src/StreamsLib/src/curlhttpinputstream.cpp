#include "curlhttpinputstream.h"
#include <cstdint>
#include <string>
#include <sstream>
#include <curl/curl.h>

using namespace std;

CurlHttpInputStream::CurlHttpInputStream(const std::string& url, const std::map<int, libCZIStreamsLib::Property>& property_bag)
{
    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    this->curl_handle = curl_easy_init();

    /* set URL to get here */
    curl_easy_setopt(this->curl_handle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L/*1L*/);

    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, Write_data);
}

/*virtual*/void CurlHttpInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) 
{
    stringstream ss;
    ss<< offset << "-" << offset + size - 1;
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, ss.str().c_str());

    WriteDataContext write_data_context;
    write_data_context.ptr_data = pv;
    write_data_context.size = size;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &write_data_context);

    curl_easy_perform(this->curl_handle);

    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = size;
    }
}

CurlHttpInputStream::~CurlHttpInputStream()
{

}

/*static*/size_t CurlHttpInputStream::Write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    WriteDataContext* write_data_context = (WriteDataContext*)stream;
    const size_t total_size = size * nmemb;
    memcpy(
        write_data_context->ptr_data, 
        ptr, 
        total_size);
    /*size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    return written;*/
    return total_size;
}
