// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include <codecvt>
#include <locale>

using namespace libCZI;
using namespace std;

namespace
{
    tuple<shared_ptr<void>, size_t> CreateCziDocumentOneSubblockWithSubBlockMetadata(const string& sub_block_metadata_xml)
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);
        ScopedBitmapLockerSP lockBm{ bitmap };
        AddSubBlockInfoStridedBitmap addSbBlkInfo;
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = 0;
        addSbBlkInfo.x = 0;
        addSbBlkInfo.y = 0;
        addSbBlkInfo.logicalWidth = bitmap->GetWidth();
        addSbBlkInfo.logicalHeight = bitmap->GetHeight();
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lockBm.stride;
        addSbBlkInfo.ptrSbBlkMetadata = sub_block_metadata_xml.c_str();
        addSbBlkInfo.sbBlkMetadataSize = sub_block_metadata_xml.size();
        writer->SyncAddSubBlock(addSbBlkInfo);
        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }
}

TEST(SubBlockMetadata, BasicReadSubBlockMetadataTest)
{
    // arrange
    const string sub_block_metadata_xml = R"(
        <SubBlockMetadata>
            <Attribute Name="TestAttribute">TestValue</Attribute>
            <Value>TestValue2</Value>
        </SubBlockMetadata>
    )";

    auto czi_and_size = CreateCziDocumentOneSubblockWithSubBlockMetadata(sub_block_metadata_xml);

    // act
    auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto sub_block = reader->ReadSubBlock(0);

    auto sub_block_metadata = libCZI::CreateSubBlockMetadataFromSubBlock(sub_block.get());

    // assert
    EXPECT_TRUE(sub_block_metadata->IsXmlValid());
    auto node = sub_block_metadata->GetChildNodeReadonly("SubBlockMetadata/Attribute[Name=TestAttribute]");
    EXPECT_TRUE(node != nullptr);

    wstring attribute_value;
    EXPECT_TRUE(node->TryGetAttribute(L"Name", &attribute_value));
    EXPECT_EQ(attribute_value, L"TestAttribute");
    wstring node_value;
    EXPECT_TRUE(node->TryGetValue(&node_value));
    EXPECT_EQ(node_value, L"TestValue");

    node = sub_block_metadata->GetChildNodeReadonly("SubBlockMetadata/Value");
    EXPECT_TRUE(node != nullptr);
    EXPECT_TRUE(node->TryGetValue(&node_value));
    EXPECT_EQ(node_value, L"TestValue2");

    string xml_output = sub_block_metadata->GetXml();
    EXPECT_FALSE(xml_output.empty());
}

TEST(SubBlockMetadata, ReadSubBlockMetadataWithInvalidXml)
{
    // arrange
    const string invalid_sub_block_metadata_xml = R"(
        <SubBlockMetadata>
            <Attribute Name="TestAttribute">TestValue</Attribute>
            <Value>TestValue2
        </SubBlockMetadata>
    )";

    auto czi_and_size = CreateCziDocumentOneSubblockWithSubBlockMetadata(invalid_sub_block_metadata_xml);

    // act
    auto inputStream = CreateStreamFromMemory(get<0>(czi_and_size), get<1>(czi_and_size));
    auto reader = CreateCZIReader();
    reader->Open(inputStream);
    auto sub_block = reader->ReadSubBlock(0);
    ASSERT_TRUE(sub_block != nullptr);
    auto sub_block_metadata = libCZI::CreateSubBlockMetadataFromSubBlock(sub_block.get());

    // assert
    EXPECT_FALSE(sub_block_metadata->IsXmlValid());
    EXPECT_THROW(sub_block_metadata->GetXml(), LibCZIXmlParseException);
    EXPECT_THROW(sub_block_metadata->GetChildNodeReadonly("SubBlockMetadata"), LibCZIXmlParseException);
    EXPECT_THROW(sub_block_metadata->TryGetAttribute(L"TestAttribute", nullptr), LibCZIXmlParseException);
    EXPECT_THROW(sub_block_metadata->EnumAttributes([](const std::wstring&, const std::wstring&) { return true; }), LibCZIXmlParseException);
}
