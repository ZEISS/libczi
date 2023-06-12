// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include "libCZI.h"
#include "pugixml.hpp"
#include "XmlNodeWrapper.h"

class CCziMetadata : public libCZI::ICziMetadata, public std::enable_shared_from_this<CCziMetadata>
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
    std::unique_ptr<XmlNodeWrapperReadonly<CCziMetadata, XmlNodeWrapperThrowExcp> > wrapper;
public:
    explicit CCziMetadata(libCZI::IMetadataSegment* pMdSeg);

public:
    const pugi::xml_document& GetXmlDoc() const;

public: // interface ICziMetadata
    bool IsXmlValid() const override;
    std::string GetXml() override;
    std::shared_ptr<libCZI::ICziMultiDimensionDocumentInfo> GetDocumentInfo() override;

public: // interface IXmlNodeRead
    bool TryGetAttribute(const wchar_t* attributeName, std::wstring* attribValue) const override;
    void EnumAttributes(const std::function<bool(const std::wstring& attribName, const std::wstring& attribValue)>& enumFunc) const override;
    bool TryGetValue(std::wstring* value) const override;
    std::shared_ptr<IXmlNodeRead> GetChildNodeReadonly(const char* path) override;
    std::wstring Name() const override;
    void EnumChildren(const std::function<bool(std::shared_ptr<IXmlNodeRead>)>& enumChildren) override;
private:
    void ThrowIfXmlInvalid() const;
};
