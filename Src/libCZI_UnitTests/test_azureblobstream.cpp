// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/utilities.h"

using namespace libCZI;
using namespace std;

struct AzureUriAndExpectedResultFixture : public testing::TestWithParam<tuple<wstring, map<wstring, wstring>>> { };

TEST_P(AzureUriAndExpectedResultFixture, TokenizeAzureUriScheme_ValidCases)
{
    const auto parameters = GetParam();
    const auto tokens = Utilities::TokenizeAzureUriString(get<0>(parameters));
    EXPECT_EQ(tokens, get<1>(parameters));
}

INSTANTIATE_TEST_SUITE_P(
    AzureBlobStream,
    AzureUriAndExpectedResultFixture,
    testing::Values(
    // third tile on top, second tile in the middle, first tile at the bottom
    make_tuple(L"a=b;c=d;d=e", map<wstring, wstring> { { L"a", L"b" }, { L"c", L"d" }, { L"d", L"e" } }),
    make_tuple(L"a=b x;c= d ;d=e  ;o=   w", map<wstring, wstring> { { L"a", L"b x" }, { L"c", L" d " }, { L"d", L"e  " }, { L"o", L"   w" } }),
    make_tuple(L" a =b;c =d; d =e", map<wstring, wstring> { { L" a ", L"b" }, { L"c ", L"d" }, { L" d ", L"e" } }),
    make_tuple(LR"(a=\;\;;c=\\d;d=e)", map<wstring, wstring> { { L"a", L";;" }, { L"c", LR"(\\d)" }, { L"d", L"e" } }),
    make_tuple(LR"(c=\\d)", map<wstring, wstring> { { L"c", LR"(\\d)" } }),
    make_tuple(LR"(\;a=abc)", map<wstring, wstring> { { LR"(;a)", L"abc" } }),
    make_tuple(LR"(\;a\==abc)", map<wstring, wstring> { { LR"(;a=)", L"abc" } }),
    make_tuple(LR"(c=\\d\=\;)", map<wstring, wstring> { { L"c", LR"(\\d=;)" } })
));

struct IllFormedAzureUriAndExpectedErrorFixture : public testing::TestWithParam<wstring> { };

TEST_P(IllFormedAzureUriAndExpectedErrorFixture, TokenizeAzureUriScheme_InvalidCases)
{
    const auto parameter = GetParam();
    EXPECT_THROW(Utilities::TokenizeAzureUriString(parameter), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    AzureBlobStream,
    IllFormedAzureUriAndExpectedErrorFixture,
    testing::Values(
    L"xxx",
    L"=xxx",
    LR"(a\=xxx)",
    L";",
    L"=",
    L"=;",
    LR"(\=\;)",
    L"a=b;c=d;k="
));

static bool IsAzureBlobInputStreamAvailable()
{
    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "azure_blob_inputstream";

    for (int i = 0; i < StreamsFactory::GetStreamClassesCount(); ++i)
    {
        StreamsFactory::StreamClassInfo info;
        if (StreamsFactory::GetStreamInfoForClass(i, info) && info.class_name == create_info.class_name)
        {
            return true;
        }
    }

    return false;
}

static string EscapeForUri(const char* str)
{
    string result;
    for (const char* p = str; *p; ++p)
    {
        if (*p == ';')
        {
            result += "\\;";
        }
        else if (*p == '=')
        {
            result += "\\=";
        }
        else
        {
            result += *p;
        }
    }

    return result;
}

static const char* GetAzureBlobStoreConnectionString()
{
    // We use the environment variable 'AZURE_BLOB_STORE_CONNECTION_STRING' to communicate a connection string.

    const char* azure_blob_store_connection_string = std::getenv("AZURE_BLOB_STORE_CONNECTION_STRING");
    return azure_blob_store_connection_string;
    
    //return R"(DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;BlobEndpoint=http://localhost:10000/devstoreaccount1;)";
}

TEST(AzureBlobStream, GetStatisticsFromBlobUsingConnectionString)
{
    if (!IsAzureBlobInputStreamAvailable())
    {
        GTEST_SKIP() << "The stream-class 'azure_blob_inputstream' is not available/configured, therefore skipping this test.";
    }

    const char* azure_blob_store_connection_string = GetAzureBlobStoreConnectionString();
    if (!azure_blob_store_connection_string)
    {
        GTEST_SKIP() << "The environment variable 'AZURE_BLOB_STORE_CONNECTION_STRING' is not set, skipping this test therefore.";
    }

    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "azure_blob_inputstream";
    create_info.property_bag = { {StreamsFactory::StreamProperties::kAzureBlob_AuthenticationMode, StreamsFactory::Property("ConnectionString")} };

    stringstream string_stream_uri;
    string_stream_uri << "containername=testcontainer;blobname=testblob;connectionstring=" << EscapeForUri(azure_blob_store_connection_string);

    const auto stream = StreamsFactory::CreateStream(create_info, string_stream_uri.str());
    ASSERT_TRUE(stream);

    const auto reader = CreateCZIReader();
    reader->Open(stream);

    const auto statistics = reader->GetStatistics();

    // we expect to find CZI with 4 subblocks, 1024x1024, and C=0..1 and T=0..1
    EXPECT_EQ(statistics.subBlockCount, 4);
    EXPECT_EQ(statistics.boundingBox.w, 1024);
    EXPECT_EQ(statistics.boundingBox.h, 1024);
    int start_c, size_c;
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::C, &start_c, &size_c));
    EXPECT_EQ(start_c, 0);
    EXPECT_EQ(size_c, 2);
    int start_t, size_t;
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::T, &start_t, &size_t));
    EXPECT_EQ(start_t, 0);
    EXPECT_EQ(size_t, 2);
}

TEST(AzureBlobStream, ReadSubBlockFromBlobUsingConnectionString)
{
    if (!IsAzureBlobInputStreamAvailable())
    {
        GTEST_SKIP() << "The stream-class 'azure_blob_inputstream' is not available/configured, skipping this test therefore.";
    }

    const char* azure_blob_store_connection_string = GetAzureBlobStoreConnectionString();
    if (!azure_blob_store_connection_string)
    {
        GTEST_SKIP() << "The environment variable 'AZURE_BLOB_STORE_CONNECTION_STRING' is not set, skipping this test therefore.";
    }

    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "azure_blob_inputstream";
    create_info.property_bag = { {StreamsFactory::StreamProperties::kAzureBlob_AuthenticationMode, StreamsFactory::Property("ConnectionString")} };

    stringstream string_stream_uri;
    string_stream_uri << "containername=testcontainer;blobname=testblob;connectionstring=" << EscapeForUri(azure_blob_store_connection_string);

    const auto stream = StreamsFactory::CreateStream(create_info, string_stream_uri.str());
    ASSERT_TRUE(stream);

    const auto reader = CreateCZIReader();
    reader->Open(stream);

    reader->EnumerateSubBlocks(
        [&](int index, const SubBlockInfo& subBlockInfo)
        {
            const auto subBlock = reader->ReadSubBlock(index);
            EXPECT_TRUE(subBlock);

            // get the bitmap from the subblock (which decompresses the data, thus testing for correctness of the data)
            const auto bitmap = subBlock->CreateBitmap();
            return true;
        });
}
