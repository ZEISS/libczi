// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <utility>
#include <memory>
#include <map>
#include <tuple>
#include <string>
#include "libCZI.h"
#include "CziSubBlockDirectory.h"
#include "pugixml.hpp"

class CCZiMetadataBuilder : public libCZI::ICziMetadataBuilder, public std::enable_shared_from_this<CCZiMetadataBuilder>
{
private:
    libCZI::pugi::xml_document metadataDoc;
    libCZI::pugi::xml_node rootNode;
public:
    CCZiMetadataBuilder() = delete;
    explicit CCZiMetadataBuilder(const wchar_t* rootNodeName);
    CCZiMetadataBuilder(const wchar_t* rootNodeName, const std::string& xml);
    ~CCZiMetadataBuilder() override = default;


    std::shared_ptr<libCZI::IXmlNodeRw> GetRootNode() override;
    std::string GetXml(bool withIndent = false) override;
};

class CNodeWrapper : public libCZI::IXmlNodeRw
{
private:
    libCZI::pugi::xml_node node;
    std::shared_ptr<CCZiMetadataBuilder> builderRef;
    struct MetadataBuilderXmlNodeWrapperThrowExcp
    {
        static void ThrowInvalidPath();
    };
public:
    CNodeWrapper(std::shared_ptr<CCZiMetadataBuilder> builderRef, libCZI::pugi::xml_node_struct* node_struct) :
        node(node_struct), builderRef(std::move(builderRef))
    {
    }

    ~CNodeWrapper() override = default;

    // interface IXmlNodeRead
    bool TryGetAttribute(const wchar_t* attributeName, std::wstring* attribValue) const override;
    void EnumAttributes(const std::function<bool(const std::wstring& attribName, const std::wstring& attribValue)>& enumFunc) const override;
    bool TryGetValue(std::wstring* value) const override;
    std::shared_ptr<IXmlNodeRead> GetChildNodeReadonly(const char* path) override;
    void EnumChildren(const std::function<bool(std::shared_ptr<IXmlNodeRead>)>& enumChildren) override;
    std::wstring Name() const override;

    // interface IXmlNodeWrite
    std::shared_ptr<libCZI::IXmlNodeRw> AppendChildNode(const char* name) override;
    std::shared_ptr<IXmlNodeRw> GetOrCreateChildNode(const char* path) override;
    std::shared_ptr<IXmlNodeRw> GetChildNode(const char* path) override;
    void SetAttribute(const char* name, const char* value) override;
    void SetAttribute(const wchar_t* name, const wchar_t* value) override;
    void SetValue(const char* str) override;
    void SetValue(const wchar_t* str) override;
    void SetValueI32(int value) override;
    void SetValueUI32(unsigned int value) override;
    void SetValueDbl(double value) override;
    void SetValueFlt(float value) override;
    void SetValueBool(bool value) override;
    void SetValueI64(long long value) override;
    void SetValueUI64(unsigned long long value) override;
    void RemoveChildren() override;
    void RemoveAttributes() override;
    bool RemoveChild(const char* name) override;
    bool RemoveAttribute(const char* name) override;
private:
    libCZI::pugi::xml_node GetChildNodePathMustExist(const char* path);
    std::shared_ptr<IXmlNodeRw> GetOrCreateChildNodeInternal(const char* path);
    libCZI::pugi::xml_node GetOrCreateChildElementNode(const wchar_t* sz);
    static libCZI::pugi::xml_node GetOrCreateChildElementNode(libCZI::pugi::xml_node& node, const wchar_t* sz);
    static libCZI::pugi::xml_node GetOrCreateChildElementNodeWithAttributes(libCZI::pugi::xml_node& node, const std::wstring& str);
    static libCZI::pugi::xml_node GetOrCreateChildElementNodeWithAttributes(libCZI::pugi::xml_node& node, const std::wstring& str, const std::map<std::wstring, std::wstring>& attribs);

    void ThrowIfCannotSetValue();
    libCZI::pugi::xml_node GetOrCreatePcDataChild();

    template <typename t> void SetValueT(t v)
    {
        this->ThrowIfCannotSetValue();
        auto pcdata = this->GetOrCreatePcDataChild();
        pcdata.set_value(std::to_wstring(v).c_str());
    }
};

class CMetadataPrepareHelper
{
public:
    static void FillDimensionChannel(
        libCZI::ICziMetadataBuilder* builder,
        const libCZI::SubBlockStatistics& statistics,
        const PixelTypeForChannelIndexStatistic& pixelTypeForChannel,
        const std::function<std::tuple<std::string, std::tuple<bool, std::string>>(int channelIdx)>& getIdAndName
    );

    static void FillDimensionChannel(
        libCZI::ICziMetadataBuilder* builder,
        int channelIdxStart, int channelIdxSize,
        const PixelTypeForChannelIndexStatistic& pixelTypeForChannel,
        const std::function<std::tuple<std::string, std::tuple<bool, std::string>>(int channelIdx)>& getIdAndName);

    static bool TryConvertToXmlMetadataPixelTypeString(libCZI::PixelType pxlType, std::string& str);

private:
    static void FillImagePixelType(libCZI::ICziMetadataBuilder* builder, libCZI::PixelType pxlType);
};
