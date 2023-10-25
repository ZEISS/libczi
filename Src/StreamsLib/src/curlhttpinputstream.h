#pragma once

#include "../include/streamsFactory.h"

class CurlHttpInputStream : public libCZI::IStream
{
private:
  //  cpr::Session session;
public:
    CurlHttpInputStream(const std::string& url, const std::map<int, libCZIStreamsLib::Property>& propertyBag);

    virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    virtual ~CurlHttpInputStream() override = default;
private:
};
