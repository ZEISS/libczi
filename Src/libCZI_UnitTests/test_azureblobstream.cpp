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
     return R"(DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;BlobEndpoint=http://localhost:10000/devstoreaccount1;)";
    //const char* azure_blob_store_connection_string = std::getenv("AZURE_BLOB_STORE_CONNECTION_STRING");
    //return azure_blob_store_connection_string;
}

TEST(AzureBlobStream, ReadFromBlobConnectionString)
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

    const auto stream = StreamsFactory::CreateStream(
        create_info,
        string_stream_uri.str());

    ASSERT_TRUE(stream);

    const auto reader = CreateCZIReader();

    try
    {
        reader->Open(stream);
    }
    catch (libCZI::LibCZIIOException& libCZI_io_exception)
    {
        std::stringstream ss;
        string what(libCZI_io_exception.what() != nullptr ? libCZI_io_exception.what() : "");
        ss << "LibCZIIOException caught -> \"" << what << "\"";
        try
        {
            libCZI_io_exception.rethrow_nested();
        }
        catch (std::exception& inner_exception)
        {
            what = inner_exception.what() != nullptr ? inner_exception.what() : "";
            ss << endl << " nested exception -> \"" << what << "\"";
        }

        cout << ss.str() << endl;
    }
    catch (std::exception& excp)
    {
        std::stringstream ss;
        ss << "Exception caught -> \"" << excp.what() << "\"";
        cout << ss.str() << endl;
    }

    const auto statistics = reader->GetStatistics();
    EXPECT_EQ(statistics.subBlockCount, 4);
}
