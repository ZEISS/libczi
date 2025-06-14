#include "SubblockMetadata.h"
#include <sstream>

using namespace std;
using namespace libCZI;

SubblockMetadata::SubblockMetadata(const char* xml, size_t xml_size)
{
    this->parseResult = this->doc.load_buffer(xml, xml_size, pugi::parse_default, pugi::encoding_utf8);
    if (this->parseResult)
    {
        this->wrapper = std::unique_ptr<XmlNodeWrapperReadonly<SubblockMetadata, XmlNodeWrapperThrowExcp>>(
            new XmlNodeWrapperReadonly<SubblockMetadata, XmlNodeWrapperThrowExcp>(this->doc.internal_object()));
    }
}

bool SubblockMetadata::TryGetAttribute(const wchar_t* attributeName, std::wstring* attribValue) const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper->TryGetAttribute(attributeName, attribValue);
}

void SubblockMetadata::EnumAttributes(const std::function<bool(const std::wstring& attribName, const std::wstring& attribValue)>& enumFunc) const
{
    this->ThrowIfXmlInvalid();
    this->wrapper->EnumAttributes(enumFunc);
}

bool SubblockMetadata::TryGetValue(std::wstring* value) const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper->TryGetValue(value);
}

std::shared_ptr<libCZI::IXmlNodeRead> SubblockMetadata::GetChildNodeReadonly(const char* path)
{
    this->ThrowIfXmlInvalid();
    return this->wrapper->GetChildNodeReadonly(path, this->shared_from_this());
}

std::wstring SubblockMetadata::Name() const
{
    this->ThrowIfXmlInvalid();
    return this->wrapper->Name();
}

void SubblockMetadata::EnumChildren(const std::function<bool(std::shared_ptr<libCZI::IXmlNodeRead>)>& enumChildren)
{
    this->ThrowIfXmlInvalid();
    return this->wrapper->EnumChildren(enumChildren, this->shared_from_this());
}

void SubblockMetadata::ThrowIfXmlInvalid() const
{
    if (!this->IsXmlValid())
    {
        stringstream ss;
        ss << "Error parsing XML [offset " << this->parseResult.offset << "]: " << this->parseResult.description();
        throw LibCZIXmlParseException(ss.str().c_str());
    }
}

/*virtual*/bool SubblockMetadata::IsXmlValid() const
{
    return this->parseResult;
}
