#pragma once

#include "libCZI_SubBlockMetadata.h"

#include "pugixml.hpp"
#include "XmlNodeWrapper.h"
#include "libCZI_Exceptions.h"

#include <memory>
#include <string>
#include <functional>

class SubblockMetadata : public libCZI::ISubBlockMetadata, public std::enable_shared_from_this<SubblockMetadata>
{
private:
    struct XmlNodeWrapperThrowExcp
    {
        [[noreturn]] static void ThrowInvalidPath()
        {
            throw libCZI::LibCZIMetadataException("invalid path", libCZI::LibCZIMetadataException::ErrorType::InvalidPath);
        }
    };

    pugi::xml_parse_result   parseResult;
    pugi::xml_document doc;
    std::unique_ptr<XmlNodeWrapperReadonly<SubblockMetadata, XmlNodeWrapperThrowExcp> > wrapper;
public:
    SubblockMetadata(const char* xml, size_t xmlSize);
    SubblockMetadata() = delete;
    SubblockMetadata(const SubblockMetadata&) = delete;
    SubblockMetadata(SubblockMetadata&&) = delete;
    SubblockMetadata& operator=(const SubblockMetadata&) = delete;
    SubblockMetadata& operator=(SubblockMetadata&&) = delete;
    virtual ~SubblockMetadata() = default;

public: // interface IXmlNodeRead
    bool TryGetAttribute(const wchar_t* attributeName, std::wstring* attribValue) const override;
    void EnumAttributes(const std::function<bool(const std::wstring& attribName, const std::wstring& attribValue)>& enumFunc) const override;
    bool TryGetValue(std::wstring* value) const override;
    std::shared_ptr<libCZI::IXmlNodeRead> GetChildNodeReadonly(const char* path) override;
    std::wstring Name() const override;
    void EnumChildren(const std::function<bool(std::shared_ptr<libCZI::IXmlNodeRead>)>& enumChildren) override;

    bool IsXmlValid() const /*override*/;

private:
    void ThrowIfXmlInvalid() const;
};
