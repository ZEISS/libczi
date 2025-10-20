// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubblockMetadata.h"
#include <sstream>

using namespace libCZI;
using namespace libCZI::detail;
using namespace libCZI::detail::pugi;
using namespace std;

SubblockMetadata::SubblockMetadata(const char* xml, size_t xml_size)
{
    this->parse_result_ = this->doc_.load_buffer(xml, xml_size, pugi::parse_default, pugi::encoding_utf8);
    if (this->parse_result_)
    {
        this->wrapper_ = std::make_unique<XmlNodeWrapperReadonly<SubblockMetadata, XmlNodeWrapperThrowExcp>>(this->doc_.internal_object());
    }
}

bool SubblockMetadata::TryGetAttribute(const wchar_t* attributeName, std::wstring* attribValue) const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper_->TryGetAttribute(attributeName, attribValue);
}

void SubblockMetadata::EnumAttributes(const std::function<bool(const std::wstring& attribName, const std::wstring& attribValue)>& enumFunc) const
{
    this->ThrowIfXmlInvalid();
    this->wrapper_->EnumAttributes(enumFunc);
}

bool SubblockMetadata::TryGetValue(std::wstring* value) const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper_->TryGetValue(value);
}

std::shared_ptr<libCZI::IXmlNodeRead> SubblockMetadata::GetChildNodeReadonly(const char* path)
{
    this->ThrowIfXmlInvalid();
    return this->wrapper_->GetChildNodeReadonly(path, this->shared_from_this());
}

std::wstring SubblockMetadata::Name() const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper_->Name();
}

void SubblockMetadata::EnumChildren(const std::function<bool(std::shared_ptr<libCZI::IXmlNodeRead>)>& enumChildren)
{
    this->ThrowIfXmlInvalid();
    return this->wrapper_->EnumChildren(enumChildren, this->shared_from_this());
}

void SubblockMetadata::ThrowIfXmlInvalid() const
{
    if (!this->IsXmlValid())
    {
        stringstream ss;
        ss << "Error parsing XML [offset " << this->parse_result_.offset << "]: " << this->parse_result_.description();
        throw LibCZIXmlParseException(ss.str().c_str());
    }
}

bool SubblockMetadata::IsXmlValid() const
{
    return this->parse_result_;
}

std::string SubblockMetadata::GetXml() const
{
    static libCZI::detail::pugi::char_t Indent[] = PUGIXML_TEXT(" ");

    this->ThrowIfXmlInvalid();

    std::ostringstream stream;
    xml_writer_stream writer(stream);
    this->doc_.save(writer, Indent, format_default, encoding_utf8);
    stream.flush();
    return stream.str();
}

bool SubblockMetadata::TryGetAttachmentDataFormat(std::wstring* data_format) 
{
    this->ThrowIfXmlInvalid();
    const auto node = this->GetChildNodeReadonly("METADATA/AttachmentSchema/DataFormat");
    if (node == nullptr)
    {
        return false;
    }

    return node->TryGetValue(data_format);
}

bool SubblockMetadata::TryGetTagAsDouble(const std::wstring& tag_name, double* value) 
{
    this->ThrowIfXmlInvalid();
    const auto node = this->GetChildNodeReadonly("METADATA/Tags");
    if (node == nullptr)
    {
        return false;
    }

    const auto requested_node = node->GetChildNodeReadonly(Utils::ConvertToUtf8(tag_name).c_str());
    if (requested_node == nullptr)
    {
        return false;
    }

    return requested_node->TryGetValueAsDouble(value);
}

bool SubblockMetadata::TryGetTagAsString(const std::wstring& tag_name, std::wstring* value) 
{
    this->ThrowIfXmlInvalid();
    const auto node = this->GetChildNodeReadonly("METADATA/Tags");
    if (node == nullptr)
    {
        return false;
    }

    const auto requested_node = node->GetChildNodeReadonly(Utils::ConvertToUtf8(tag_name).c_str());
    if (requested_node == nullptr)
    {
        return false;
    }

    return requested_node->TryGetValue(value);
}

bool SubblockMetadata::TryGetStagePositionFromTags(std::tuple<double, double>* stage_position) 
{
    this->ThrowIfXmlInvalid();
    const auto node = this->GetChildNodeReadonly("METADATA/Tags");
    if (node == nullptr)
    {
        return false;
    }

    const auto stage_x_position_node = node->GetChildNodeReadonly("StageXPosition");
    if (stage_x_position_node == nullptr)
    {
        return false;
    }

    const auto stage_y_position_node = node->GetChildNodeReadonly("StageYPosition");
    if (stage_y_position_node == nullptr)
    {
        return false;
    }

    double stage_x_position;
    if (!stage_x_position_node->TryGetValueAsDouble(&stage_x_position))
    {
        return false;
    }

    double stage_y_position;
    if (!stage_y_position_node->TryGetValueAsDouble(&stage_y_position))
    {
        return false;
    }

    if (stage_position != nullptr)
    {
        *stage_position = make_tuple(stage_x_position, stage_y_position);
    }

    return true;
}
