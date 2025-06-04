#pragma once

#include "libCZI_SubBlockMetadata.h"

#include "pugixml.hpp"
#include <memory>
#include <string>
#include <functional>



class SubblockMetadata : public libCZI::ISubBlockMetadata
{
private:
    pugi::xml_document doc;
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
};
