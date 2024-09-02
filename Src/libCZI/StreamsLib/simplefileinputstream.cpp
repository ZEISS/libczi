// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "simplefileinputstream.h"
#include "../utilities.h"
#include <iomanip>
#include <sstream>
#include <mutex>

using namespace std;

#if defined(_WIN32)

SimpleFileInputStream::SimpleFileInputStream(const wchar_t* filename)
{
    const errno_t err = _wfopen_s(&this->fp_, filename, L"rb");
    if (err != 0)
    {
        std::stringstream ss;
        char errMsg[100];
        strerror_s(errMsg, sizeof(errMsg), err);
        ss << "Error opening the file \"" << Utilities::convertWchar_tToUtf8(filename) << "\" -> errno=" << err << " (" << errMsg << ")";
        throw std::runtime_error(ss.str());
    }
}

SimpleFileInputStream::SimpleFileInputStream(const std::string& filename)
    : SimpleFileInputStream(Utilities::convertUtf8ToWchar_t(filename.c_str()).c_str())
{
}

#else

SimpleFileInputStream::SimpleFileInputStream(const wchar_t* filename)
    : SimpleFileInputStream(Utilities::convertWchar_tToUtf8(filename))
{
}

SimpleFileInputStream::SimpleFileInputStream(const std::string& filename)
{
    FILE* file = fopen(filename.c_str(), "rb");
    if (file == NULL)
    {
        auto err = errno;
        stringstream ss;
        ss << "Error opening the file \"" << filename<< "\" -> errno=" << err << " (" << strerror(err) << ")";
        throw runtime_error(ss.str());
    }

    this->fp_ = file;
}

#endif

SimpleFileInputStream::~SimpleFileInputStream()
{
    fclose(this->fp_);
}

/*virtual*/void SimpleFileInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    std::lock_guard<std::mutex> lck(this->request_mutex_);
#if defined(_WIN32)
    int r = _fseeki64(this->fp_, offset, SEEK_SET);
#else
    int r = fseeko(this->fp_, offset, SEEK_SET);
#endif
    if (r != 0)
    {
        const auto err = errno;
        stringstream ss;
        ss << "Seek to file-position " << offset << " failed, errno=<<" << err << ".";
        throw std::runtime_error(ss.str());
    }

    const std::uint64_t bytesRead = fread(pv, 1, static_cast<size_t>(size), this->fp_);
    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytesRead;
    }
}
