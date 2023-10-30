// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "preadfileinputstream.h"

#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>   // required on BSD

#include "../utilities.h"

using namespace libCZI;

PreadFileInputStream::PreadFileInputStream(const std::string& filename)
{
    this->fileDescriptor = open(filename.c_str(), O_RDONLY);
    if (this->fileDescriptor < 0)
    {
        auto err = errno;
        std::stringstream ss;
        ss << "Error opening the file \"" << filename << "\" -> errno=" << err << " (" << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }
}

PreadFileInputStream::PreadFileInputStream(const wchar_t* filename)
    : PreadFileInputStream(Utilities::convertWchar_tToUtf8(filename))
{
}

PreadFileInputStream::~PreadFileInputStream()
{
    if (this->fileDescriptor != 0)
    {
        close(this->fileDescriptor);
    }
}

/*virtual*/void PreadFileInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    ssize_t bytesRead = pread(this->fileDescriptor, pv, size, offset);
    if (bytesRead < 0)
    {
        auto err = errno;
        std::stringstream ss;
        ss << "Error reading from file (errno=" << err << " -> " << strerror(err) << ")";
        throw std::runtime_error(ss.str());
    }

    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = bytesRead;
    }
}

#endif // LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
