// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later﻿
#include "pch.h"
#include "inc_libCZI.h"

using namespace libCZI;
using namespace std;

TEST(MetadataBuilder, MetadataBuilder1)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();
	root->GetOrCreateChildNode("Metadata/Information/Image/SizeX")->SetValueI32(1024);
	root->GetOrCreateChildNode("Metadata/Information/Image/SizeY")->SetValueI32(768);

	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0]/PixelType")->SetValue("Bgr24");
	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0]/BitCountRange")->SetValueI32(16);

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <Image>\n"
		"        <SizeX>1024</SizeX>\n"
		"        <SizeY>768</SizeY>\n"
		"      </Image>\n"
		"    </Information>\n"
		"    <DisplaySetting>\n"
		"      <Channels>\n"
		"        <Channel Id=\"Channel:0\">\n"
		"          <PixelType>Bgr24</PixelType>\n"
		"          <BitCountRange>16</BitCountRange>\n"
		"        </Channel>\n"
		"      </Channels>\n"
		"    </DisplaySetting>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataBuilder2)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();

	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0,Name=1st]/PixelType")->SetValue("Bgr24");
	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0,Name=1st]/BitCountRange")->SetValueI32(16);
	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:1,Name=2nd]/PixelType")->SetValue("Bgr48");
	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:1,Name=2nd]/BitCountRange")->SetValueI32(32);

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <DisplaySetting>\n"
		"      <Channels>\n"
		"        <Channel Id=\"Channel:0\" Name=\"1st\">\n"
		"          <PixelType>Bgr24</PixelType>\n"
		"          <BitCountRange>16</BitCountRange>\n"
		"        </Channel>\n"
		"        <Channel Id=\"Channel:1\" Name=\"2nd\">\n"
		"          <PixelType>Bgr48</PixelType>\n"
		"          <BitCountRange>32</BitCountRange>\n"
		"        </Channel>\n"
		"      </Channels>\n"
		"    </DisplaySetting>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataBuilder3)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();

	bool exceptionOk = false;
	try
	{
		root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[=Channel:0")->SetValue("Bgr24");
	}
	catch (const LibCZIMetadataBuilderException& excp)
	{
		exceptionOk = excp.GetErrorType() == LibCZIMetadataBuilderException::ErrorType::InvalidPath;
	}

	EXPECT_TRUE(exceptionOk) << "should have thrown an exception";

	exceptionOk = false;
	try
	{
		root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0,Name")->SetValueI32(16);
	}
	catch (const LibCZIMetadataBuilderException& excp)
	{
		exceptionOk = excp.GetErrorType() == LibCZIMetadataBuilderException::ErrorType::InvalidPath;
	}

	EXPECT_TRUE(exceptionOk) << "should have thrown an exception";
}

TEST(MetadataBuilder, MetadataBuilder4)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();
	auto node = root->AppendChildNode("TESTNODE");
	node->SetValue(u8"火车站");
	node->SetAttribute(u8"数量", u8"通り");

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		u8"<ImageDocument>\n"
		u8"  <TESTNODE 数量=\"通り\">火车站</TESTNODE>\n"
		u8"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, SubBlockMetadataBuilder1)
{
	auto mdBldr = Utils::CreateSubBlockMetadata();
	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<METADATA />\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, SubBlockMetadataBuilder2)
{
	auto mdBldr = Utils::CreateSubBlockMetadata(
		[](int no, std::tuple<std::string, std::string>& nodeNameAndValue)->bool
	{
		switch (no)
		{
		case 0:
			nodeNameAndValue = std::tuple<std::string, std::string>("Tag1", "ABC");
			return true;
		case 1:
			nodeNameAndValue = std::tuple<std::string, std::string>("Tag2", "XYZ");
			return true;
		default:
			return false;
		}
	});

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<METADATA>\n"
		"  <Tags>\n"
		"    <Tag1>ABC</Tag1>\n"
		"    <Tag2>XYZ</Tag2>\n"
		"  </Tags>\n"
		"</METADATA>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, SubBlockMetadataBuilder3)
{
	auto mdBldr = Utils::CreateSubBlockMetadata(
		[](int no, std::tuple<std::string, std::string>& nodeNameAndValue)->bool
	{
		switch (no)
		{
		case 0:
			nodeNameAndValue = std::tuple<std::string, std::string>("Tag1", "ABC");
			return true;
		case 1:
			nodeNameAndValue = std::tuple<std::string, std::string>("Tag2", "XYZ");
			return true;
		default:
			return false;
		}
	});

	auto n = mdBldr->GetRootNode()->GetOrCreateChildNode("DataSchema/ValidBitsPerPixel");
	n->SetValueI32(16);

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<METADATA>\n"
		"  <Tags>\n"
		"    <Tag1>ABC</Tag1>\n"
		"    <Tag2>XYZ</Tag2>\n"
		"  </Tags>\n"
		"  <DataSchema>\n"
		"    <ValidBitsPerPixel>16</ValidBitsPerPixel>\n"
		"  </DataSchema>\n"
		"</METADATA>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils1)
{
	XmlDateTime dt;
	dt.Clear();
	dt.sec = 34;
	dt.min = 32;
	dt.hour = 2;
	dt.mday = 20;
	dt.mon = 6;
	dt.year = 1969;
	dt.offsetHours = 1;
	dt.offsetMinutes = 53;
	auto s = dt.ToXmlString();
	EXPECT_TRUE(s.compare("1969-06-20T02:32:34+01:53") == 0) << "Incorrect result";

	dt.Clear();
	dt.sec = 4;
	dt.min = 2;
	dt.hour = 1;
	dt.mday = 2;
	dt.mon = 6;
	dt.year = 9;
	dt.isUTC = true;
	s = dt.ToXmlString();
	EXPECT_TRUE(s.compare("0009-06-02T01:02:04Z") == 0) << "Incorrect result";

	dt.Clear();
	dt.sec = 4;
	dt.min = 2;
	dt.hour = 21;
	dt.mday = 2;
	dt.mon = 6;
	dt.year = 92;
	dt.offsetHours = -11;
	dt.offsetMinutes = 53;
	s = dt.ToXmlString();
	EXPECT_TRUE(s.compare("0092-06-02T21:02:04-11:53") == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils2)
{
	auto mdBldr = CreateMetadataBuilder();
	XmlDateTime dt;
	dt.Clear();
	dt.sec = 34;
	dt.min = 32;
	dt.hour = 2;
	dt.mday = 20;
	dt.mon = 6;
	dt.year = 1969;
	MetadataUtils::WriteDimInfoT_Interval(mdBldr.get(), &dt, 0, 1);
	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <Dimensions>\n"
		"        <T>\n"
		"          <StartTime>1969-06-20T02:32:34</StartTime>\n"
		"          <Positions>\n"
		"            <Interval>\n"
		"              <Start>0</Start>\n"
		"              <Increment>1</Increment>\n"
		"            </Interval>\n"
		"          </Positions>\n"
		"        </T>\n"
		"      </Dimensions>\n"
		"    </Information>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils3)
{
	auto mdBldr = CreateMetadataBuilder();
	XmlDateTime dt;
	dt.Clear();
	dt.sec = 34;
	dt.min = 32;
	dt.hour = 2;
	dt.mday = 20;
	dt.mon = 6;
	dt.year = 1969;

	static const double offsets[] = { 1,2.5,3.5,4,5,6,7,8,10,122,220 };

	MetadataUtils::WriteDimInfoT_List(mdBldr.get(), &dt,
		[&](int i)->double
	{
		if (i < sizeof(offsets) / sizeof(offsets[0]))
		{
			return offsets[i];
		}

		return std::numeric_limits<double>::quiet_NaN();
	});

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <Dimensions>\n"
		"        <T>\n"
		"          <StartTime>1969-06-20T02:32:34</StartTime>\n"
		"          <Positions>\n"
		"            <List>\n"
		"              <Offsets>1 2.5 3.5 4 5 6 7 8 10 122 220</Offsets>\n"
		"            </List>\n"
		"          </Positions>\n"
		"        </T>\n"
		"      </Dimensions>\n"
		"    </Information>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils4)
{
	auto mdBldr = CreateMetadataBuilder();
	GeneralDocumentInfo docInfo;
	docInfo.name = L"NAME";
	docInfo.name_valid = true;
	docInfo.title = L"TITLE";
	docInfo.title_valid = true;
	docInfo.userName = L"USERNAME";
	docInfo.userName_valid = true;
	docInfo.description = L"DESCRIPTION";
	docInfo.description_valid = true;
	docInfo.comment = L"COMMENT";
	docInfo.comment_valid = true;
	docInfo.keywords = L"KEYWORDS";
	docInfo.keywords_valid = true;
	docInfo.rating = 4;
	docInfo.rating_valid = true;

	XmlDateTime dt;
	dt.Clear();
	dt.sec = 34;
	dt.min = 32;
	dt.hour = 2;
	dt.mday = 20;
	dt.mon = 6;
	dt.year = 1969;
	docInfo.creationDateTime = dt.ToXmlWstring();
	docInfo.creationDateTime_valid = true;

	MetadataUtils::WriteGeneralDocumentInfo(mdBldr.get(), docInfo);
	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <Document>\n"
		"        <Name>NAME</Name>\n"
		"        <Title>TITLE</Title>\n"
		"        <UserName>USERNAME</UserName>\n"
		"        <Description>DESCRIPTION</Description>\n"
		"        <Comment>COMMENT</Comment>\n"
		"        <Keywords>KEYWORDS</Keywords>\n"
		"        <CreationDate>1969-06-20T02:32:34</CreationDate>\n"
		"        <Rating>4</Rating>\n"
		"      </Document>\n"
		"    </Information>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils5)
{
	auto mdBldr = CreateMetadataBuilder();
	ScalingInfo scalingInfo;
	scalingInfo.scaleX = scalingInfo.scaleY = 1.06822E-07;
	scalingInfo.scaleZ = 5E-07;
	MetadataUtils::WriteScalingInfo(mdBldr.get(), scalingInfo);
	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Scaling>\n"
		"      <Items>\n"
		"        <Distance Id=\"X\">\n"
		"          <Value>1.06822e-07</Value>\n"
		"        </Distance>\n"
		"        <Distance Id=\"Y\">\n"
		"          <Value>1.06822e-07</Value>\n"
		"        </Distance>\n"
		"        <Distance Id=\"Z\">\n"
		"          <Value>5e-07</Value>\n"
		"        </Distance>\n"
		"      </Items>\n"
		"    </Scaling>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils6)
{
	auto mdBldr = CreateMetadataBuilder();
	ScalingInfoEx scalingInfo;
	scalingInfo.scaleX = scalingInfo.scaleY = 1.06822E-07;
	scalingInfo.defaultUnitFormatX =
		scalingInfo.defaultUnitFormatY =
		scalingInfo.defaultUnitFormatZ = L"µm";
	scalingInfo.scaleZ = 5E-07;
	MetadataUtils::WriteScalingInfoEx(mdBldr.get(), scalingInfo);
	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		u8"<ImageDocument>\n"
		u8"  <Metadata>\n"
		u8"    <Scaling>\n"
		u8"      <Items>\n"
		u8"        <Distance Id=\"X\">\n"
		u8"          <Value>1.06822e-07</Value>\n"
		u8"          <DefaultUnitFormat>µm</DefaultUnitFormat>\n"
		u8"        </Distance>\n"
		u8"        <Distance Id=\"Y\">\n"
		u8"          <Value>1.06822e-07</Value>\n"
		u8"          <DefaultUnitFormat>µm</DefaultUnitFormat>\n"
		u8"        </Distance>\n"
		u8"        <Distance Id=\"Z\">\n"
		u8"          <Value>5e-07</Value>\n"
		u8"          <DefaultUnitFormat>µm</DefaultUnitFormat>\n"
		u8"        </Distance>\n"
		u8"      </Items>\n"
		u8"    </Scaling>\n"
		u8"  </Metadata>\n"
		u8"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils7)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();

	root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0]/PixelType")->SetValue("Bgr24");

	auto node = root->GetOrCreateChildNode("Metadata/DisplaySetting/Channels/Channel[Id=Channel:0]");

	bool wasOk = false;
	int callCnt = 0;
	node->EnumAttributes([&](const std::wstring& attribName, const std::wstring& attribValue)->bool
	{
		if (wcscmp(attribName.c_str(), L"Id") == 0 && wcscmp(attribValue.c_str(), L"Channel:0") == 0)
		{
			wasOk = true;
		}

		callCnt++;
		return true;
	});

	EXPECT_TRUE(wasOk && callCnt == 1) << "Incorrect result";

	wstring idAttribValue;
	bool b = node->TryGetAttribute(L"Id", &idAttribValue);
	EXPECT_TRUE(b == true && wcscmp(idAttribValue.c_str(), L"Channel:0") == 0) << "Incorrect result";
}

TEST(MetadataBuilder, MetadataBuilder8)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();
	auto n = root->AppendChildNode("TEST");
	n->SetValue("ABC");

	wstring value;
	bool b = n->TryGetValue(&value);
	EXPECT_TRUE(b == true && wcscmp(value.c_str(), L"ABC") == 0)<<"Incorrect result";

	auto n2 = n->AppendChildNode("TEST2");
	n2->AppendChildNode("TEST3")->SetValue("123");

	b = n2->TryGetValue(&value);
	EXPECT_FALSE(b)<<"Incorrect result";
}

TEST(MetadataBuilder, MetadataBuilder9)
{
	auto mdBldr = CreateMetadataBuilder();
	auto root = mdBldr->GetRootNode();
	auto n = root->GetChildNode("Metadata/Information/Image/SizeX");
	EXPECT_TRUE(!n)<<"Incorrect result";
}

TEST(MetadataBuilder, MetadataUtils10)
{
	auto mdBldr = CreateMetadataBuilder();
	map<string, CustomValueVariant> InvalidCA = { {"1234", CustomValueVariant(1234)}, {"5678", CustomValueVariant(5678)} };

	for (auto InvalidIt = InvalidCA.begin(); InvalidIt != InvalidCA.end(); InvalidIt++) 
	{
		auto key = InvalidIt->first;
		auto value = InvalidIt->second;
		EXPECT_THROW({MetadataUtils::SetOrAddCustomKeyValuePair(mdBldr.get(), key, value);}, invalid_argument);
	}
}

TEST(MetadataBuilder, MetadataUtils11)
{
	auto mdBldr = CreateMetadataBuilder();
	map<string, CustomValueVariant> ValidCA = { {"Attr1", CustomValueVariant(1234)}, {"Attr2", CustomValueVariant(string("SomeStrings"))}, {"Attr3", CustomValueVariant(true)} , {"Attr4", CustomValueVariant(12.5f)}, {"Attr5", CustomValueVariant(22.5)} };

	for (auto ValidIt = ValidCA.begin(); ValidIt != ValidCA.end(); ValidIt++) 
	{
		auto key = ValidIt->first;
		auto value = ValidIt->second;

		MetadataUtils::SetOrAddCustomKeyValuePair(mdBldr.get(), key, value);
	}

	auto xml = mdBldr->GetXml(true);

	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <CustomAttributes>\n"
		"        <KeyValue>\n"
		"          <Attr1 Type=\"Int32\">1234</Attr1>\n"
		"          <Attr2 Type=\"String\">SomeStrings</Attr2>\n"
		"          <Attr3 Type=\"Boolean\">true</Attr3>\n"
		"          <Attr4 Type=\"Float\">12.5</Attr4>\n"
		"          <Attr5 Type=\"Double\">22.5</Attr5>\n"
		"        </KeyValue>\n"
		"      </CustomAttributes>\n"
		"    </Information>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}

TEST(MetadataBuilder, AddPropertyMultipleTimes_Expect_LastestUpdateWins)
{
	auto mdBldr = CreateMetadataBuilder();

	MetadataUtils::SetOrAddCustomKeyValuePair(mdBldr.get(), "test", CustomValueVariant("TestText"));
	MetadataUtils::SetOrAddCustomKeyValuePair(mdBldr.get(), "test", CustomValueVariant(12.5));

	auto xml = mdBldr->GetXml(true);

	// we expect that that "last set operation" wins
	static const char* expectedResult =
		"<ImageDocument>\n"
		"  <Metadata>\n"
		"    <Information>\n"
		"      <CustomAttributes>\n"
		"        <KeyValue>\n"
		"          <test Type=\"Double\">12.5</test>\n"
		"        </KeyValue>\n"
		"      </CustomAttributes>\n"
		"    </Information>\n"
		"  </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(expectedResult, xml.c_str()) == 0) << "Incorrect result";
}
