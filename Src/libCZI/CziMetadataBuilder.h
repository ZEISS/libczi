// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <utility>
#include "libCZI.h"
#include "CziSubBlockDirectory.h"
#include "pugixml.hpp"

class CCZiMetadataBuilder : public libCZI::ICziMetadataBuilder, public std::enable_shared_from_this<CCZiMetadataBuilder>
{
private:
	pugi::xml_document metadataDoc;
	pugi::xml_node rootNode;
public:
	CCZiMetadataBuilder() = delete;
	CCZiMetadataBuilder(const wchar_t* rootNodeName);
	virtual ~CCZiMetadataBuilder() override = default;


	virtual std::shared_ptr<libCZI::IXmlNodeRw> GetRootNode() override;
	virtual std::string GetXml(bool withIndent = false) override;
};

class CNodeWrapper : public libCZI::IXmlNodeRw
{
private:
	pugi::xml_node node;
	std::shared_ptr<CCZiMetadataBuilder> builderRef;
	struct MetadataBuilderXmlNodeWrapperThrowExcp
	{
		static void ThrowInvalidPath();
	};
public:
	CNodeWrapper(std::shared_ptr<CCZiMetadataBuilder> builderRef, pugi::xml_node_struct* node_struct) :
		node(node_struct), builderRef(std::move(builderRef))
	{
	}

	virtual ~CNodeWrapper() override = default;

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
	void SetValue(const char* str) override;
	void SetValue(const wchar_t* str) override;
	void SetValueI32(int value) override;
	void SetValueUI32(unsigned int value) override;
	void SetValueDbl(double value) override;
	void SetValueFlt(float value) override;
	void SetValueBool(bool value) override;
	void SetValueI64(long long value) override;
	void SetValueUI64(unsigned long long value) override;

private:
	std::shared_ptr<IXmlNodeRw> GetOrCreateChildNode(const char* path, bool allowCreation);
	pugi::xml_node GetOrCreateChildElementNode(const wchar_t* sz, bool allowCreation);
	static pugi::xml_node GetOrCreateChildElementNode(pugi::xml_node& node, const wchar_t* sz, bool allowCreation);

	pugi::xml_node GetOrCreateChildElementNodeWithAttributes(const std::wstring& str, bool allowCreation);
	static pugi::xml_node GetOrCreateChildElementNodeWithAttributes(pugi::xml_node& node, const std::wstring& str, bool allowCreation);

	static pugi::xml_node GetOrCreateChildElementNodeWithAttributes(pugi::xml_node& node, const std::wstring& str, const std::map<std::wstring, std::wstring>& attribs, bool allowCreation);

	static std::map<std::wstring, std::wstring> ParseAttributes(const std::wstring& str);

	void ThrowIfCannotSetValue();
	pugi::xml_node GetOrCreatePcDataChild();

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
		std::function<std::tuple<std::string, std::tuple<bool, std::string>>(int channelIdx)> getIdAndName
	);

	static void FillDimensionChannel(
		libCZI::ICziMetadataBuilder* builder,
		int channelIdxStart, int channelIdxSize,
		const PixelTypeForChannelIndexStatistic& pixelTypeForChannel,
		std::function<std::tuple<std::string, std::tuple<bool, std::string>>(int channelIdx)> getIdAndName);


private:
	static bool TryConvertToXmlMetadataPixelTypeString(libCZI::PixelType pxlType, std::string& str);
	static void FillImagePixelType(libCZI::ICziMetadataBuilder* builder, libCZI::PixelType pxlType);
};