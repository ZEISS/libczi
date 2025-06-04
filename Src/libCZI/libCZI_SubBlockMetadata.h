#pragma once

#include "libCZI_Metadata.h"

namespace libCZI
{
    class ISubBlockMetadata : public IXmlNodeRead
    {
    public:
        ISubBlockMetadata() = default;
        virtual ~ISubBlockMetadata() = default;

        // Delete copy constructor and copy assignment operator
        ISubBlockMetadata(const ISubBlockMetadata&) = delete;
        ISubBlockMetadata& operator=(const ISubBlockMetadata&) = delete;

        // Delete move constructor and move assignment operator
        ISubBlockMetadata(ISubBlockMetadata&&) = delete;
        ISubBlockMetadata& operator=(ISubBlockMetadata&&) = delete;
    };
} // namespace libCZI
