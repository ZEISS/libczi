#include "SubblockMetadata.h"

SubblockMetadata::SubblockMetadata(const char* xml, size_t xml_size)
{
    auto parse_result = this->doc.load_buffer(xml, xml_size, pugi::parse_default, pugi::encoding_utf8);
}
