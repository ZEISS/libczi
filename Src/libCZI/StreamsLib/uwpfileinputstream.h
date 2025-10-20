// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_WINDOWS_UWPAPI_AVAILABLE
#include <string>
#include "../libCZI.h"
#include <Windows.h>

namespace libCZI
{
    namespace detail
    {

        /// Implementation of the IStream-interface for files based on the Win32-API.
        /// It leverages the Win32-API ReadFile passing in an offset, thus allowing for concurrent
        /// access without locking.
        class UwpFileInputStream : public libCZI::IStream
        {
        private:
            HANDLE handle;
        public:
            UwpFileInputStream() = delete;
            explicit UwpFileInputStream(const wchar_t* filename);
            explicit UwpFileInputStream(const std::string& filename);
            ~UwpFileInputStream() override;
        public: // interface libCZI::IStream
            void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
        };

    }   // namespace detail
}   // namespace libCZI

#endif
