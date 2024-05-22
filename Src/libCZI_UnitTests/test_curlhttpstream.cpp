// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"

using namespace libCZI;

TEST(CurlHttpInputStream, SimpleReadFromHttps)
{
    static constexpr char kUrl[] = "https://media.githubusercontent.com/media/ptahmose/libCZI_testdata/main/MD5/ff20e3a15d797509f7bf494ea21109d3";  // sparse_planes.czi.

    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "curl_http_inputstream";

    // set a five-seconds time-out (for the whole operation)
    create_info.property_bag = { {StreamsFactory::StreamProperties::kCurlHttp_Timeout,StreamsFactory::Property(5)} };

    const auto stream = StreamsFactory::CreateStream(create_info, kUrl);

    if (!stream)
    {
        GTEST_SKIP() << "The stream-class 'curl_http_inputstream' is not available/configured, skipping this test therefore.";
    }

    uint8_t buffer[1024];
    uint64_t bytes_read = 0;
    try
    {
        stream->Read(0, buffer, sizeof(buffer), &bytes_read);
    }
    catch (const std::exception& e)
    {
        GTEST_SKIP() << "Exception: " << e.what() << "--> skipping this test as inconclusive, assuming network issues";
    }

    EXPECT_EQ(bytes_read, 1024);

    uint8_t hash[16];
    Utils::CalcMd5SumHash(buffer, sizeof(buffer), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0xb9, 0xad, 0x63, 0xdd, 0xa7, 0xcb, 0x4e, 0x6a, 0x15, 0xe2, 0x59, 0x6e, 0xbf, 0xc7, 0x7a, 0xce };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}

TEST(CurlHttpInputStream, TryToReadZeroBytesFromHttpsAndExpectSuccess)
{
    static constexpr char kUrl[] = "https://media.githubusercontent.com/media/ptahmose/libCZI_testdata/main/MD5/ff20e3a15d797509f7bf494ea21109d3";  // sparse_planes.czi.

    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "curl_http_inputstream";

    // set a five-seconds time-out (for the whole operation)
    create_info.property_bag = { {StreamsFactory::StreamProperties::kCurlHttp_Timeout,StreamsFactory::Property(5)} };

    const auto stream = StreamsFactory::CreateStream(create_info, kUrl);

    if (!stream)
    {
        GTEST_SKIP() << "The stream-class 'curl_http_inputstream' is not available/configured, skipping this test therefore.";
    }

    uint8_t buffer[1];
    uint64_t bytes_read = (std::numeric_limits<uint64_t>::max)();
    try
    {
        stream->Read(0, buffer, 0, &bytes_read);
    }
    catch (const std::exception& e)
    {
        GTEST_SKIP() << "Exception: " << e.what() << "--> skipping this test as inconclusive, assuming network issues";
    }

    EXPECT_EQ(bytes_read, 0);
}

TEST(CurlHttpInputStream, OpenAndReadCziFromUrl)
{
    static constexpr char kUrl[] = "https://media.githubusercontent.com/media/ptahmose/libCZI_testdata/main/MD5/ff20e3a15d797509f7bf494ea21109d3";  // sparse_planes.czi

    StreamsFactory::CreateStreamInfo create_info;
    create_info.class_name = "curl_http_inputstream";

    // set a five-seconds time-out (for the whole operation)
    create_info.property_bag = { {StreamsFactory::StreamProperties::kCurlHttp_Timeout,StreamsFactory::Property(5)} };

    const auto stream = StreamsFactory::CreateStream(create_info, kUrl);

    if (!stream)
    {
        GTEST_SKIP() << "The stream-class 'curl_http_inputstream' is not available/configured, skipping this test therefore.";
    }

    const auto czi_reader = CreateCZIReader();
    try
    {
        czi_reader->Open(stream);
    }
    catch (const LibCZIIOException& e)
    {
        GTEST_SKIP() << "Exception: " << e.what() << "--> skipping this test as inconclusive, assuming network issues";
    }

    const auto statistics = czi_reader->GetStatistics();
    EXPECT_EQ(statistics.subBlockCount, 2);

    const auto file_header_info = czi_reader->GetFileHeaderInfo();
    static constexpr libCZI::GUID expected_file_guild { 0x61b6b581, 0x5d0c, 0x475e, {0x91, 0x9b, 0x32, 0x4c, 0x57, 0xd1, 0x7c, 0x09} };
    EXPECT_TRUE(file_header_info.fileGuid == expected_file_guild);

    auto sub_block = czi_reader->ReadSubBlock(0);
    ASSERT_TRUE(sub_block);

    size_t size_of_data;
    auto data = sub_block->GetRawData(ISubBlock::MemBlkType::Data, &size_of_data);
    EXPECT_EQ(size_of_data, 250000);

    uint8_t hash[16];
    Utils::CalcMd5SumHash(data.get(), size_of_data, hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x9f, 0xb0, 0x52, 0x86, 0x58, 0xde, 0xe0, 0x95, 0xfd, 0x2c, 0x90, 0x93, 0x7c, 0x8a, 0x94, 0xde };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}
