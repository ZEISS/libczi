// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include "libCZI.h"
#include "CziParse.h"

namespace libCZI
{
    namespace detail
    {

        class CCziAttachment : public  libCZI::IAttachment
        {
        private:
            std::shared_ptr<const void> spData;
            std::uint64_t   dataSize;
            libCZI::AttachmentInfo  info;
        public:
            CCziAttachment(libCZI::AttachmentInfo info, const CCZIParse::AttachmentData& data, std::function<void(void*)> deleter);

            // interface IAttachment
            const libCZI::AttachmentInfo& GetAttachmentInfo() const override;

            void DangerousGetRawData(const void*& ptr, size_t& size) const override;

            std::shared_ptr<const void> GetRawData(size_t* ptrSize) override;
        };

    }  // namespace detail
}  // namespace libCZI
