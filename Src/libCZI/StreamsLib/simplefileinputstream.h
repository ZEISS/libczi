#pragma once
#include <libCZI_Config.h>

#include <string>
#include <cstdio>
#include "../libCZI.h"

class SimpleFileInputStream : public libCZI::IStream
{
private:
    FILE* fp;
public:
    SimpleFileInputStream() = delete;
    explicit SimpleFileInputStream(const wchar_t* filename);
    explicit SimpleFileInputStream(const std::string& filename);
    ~SimpleFileInputStream() override;
public: // interface libCZI::IStream
    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

