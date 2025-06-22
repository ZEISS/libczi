#pragma once

#include <tuple>
#include "libCZI_Metadata.h"

namespace libCZI
{
    class ISubBlockMetadataMetadataView
    {
    public:
        ISubBlockMetadataMetadataView() = default;
        virtual ~ISubBlockMetadataMetadataView() = default;

        virtual bool TryGetAttachmentDataFormat(std::wstring* data_format)= 0;

        virtual bool TryGetTagAsDouble(std::wstring tag_name, double* value) = 0;
        virtual bool TryGetTagAsString(std::wstring tag_name, std::wstring* value) = 0;

        virtual bool TryGetStagePositionFromTags(std::tuple<double, double>* stage_position) = 0;

        // Delete copy constructor and copy assignment operator
        ISubBlockMetadataMetadataView(const ISubBlockMetadataMetadataView&) = delete;
        ISubBlockMetadataMetadataView& operator=(const ISubBlockMetadataMetadataView&) = delete;
        // Delete move constructor and move assignment operator
        ISubBlockMetadataMetadataView(ISubBlockMetadataMetadataView&&) = delete;
        ISubBlockMetadataMetadataView& operator=(ISubBlockMetadataMetadataView&&) = delete;

    };

    class ISubBlockMetadata : public IXmlNodeRead, ISubBlockMetadataMetadataView
    {
    public:
        ISubBlockMetadata() = default;
        virtual ~ISubBlockMetadata() override = default;

        virtual bool IsXmlValid() const = 0;
        virtual std::string GetXml() const = 0;



        // Delete copy constructor and copy assignment operator
        ISubBlockMetadata(const ISubBlockMetadata&) = delete;
        ISubBlockMetadata& operator=(const ISubBlockMetadata&) = delete;

        // Delete move constructor and move assignment operator
        ISubBlockMetadata(ISubBlockMetadata&&) = delete;
        ISubBlockMetadata& operator=(ISubBlockMetadata&&) = delete;
    };
} // namespace libCZI
