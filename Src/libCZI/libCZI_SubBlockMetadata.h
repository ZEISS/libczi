#pragma once

#include "libCZI_Metadata.h"

namespace libCZI
{
    class ISubBlockMetadata : public IXmlNodeRead
    {
    public:
        ISubBlockMetadata() = default;
        virtual ~ISubBlockMetadata() = default;

        virtual bool IsXmlValid() const = 0;
        virtual std::string GetXml() = 0;

        // Delete copy constructor and copy assignment operator
        ISubBlockMetadata(const ISubBlockMetadata&) = delete;
        ISubBlockMetadata& operator=(const ISubBlockMetadata&) = delete;

        // Delete move constructor and move assignment operator
        ISubBlockMetadata(ISubBlockMetadata&&) = delete;
        ISubBlockMetadata& operator=(ISubBlockMetadata&&) = delete;
    };
} // namespace libCZI
