// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/StreamImpl.h"
#include "../libCZI/utilities.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <future>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

using namespace libCZI;

namespace
{
    class LocalStreamReadTest : public testing::TestWithParam<std::string>
    {
    protected:
        std::shared_ptr<libCZI::IStream> stream;
        const std::array<uint8_t, 4> data{ { 1, 2, 3, 4 } };
        std::string directory;
        std::string filename;

        void Open(bool empty = false)
        {
            stream.reset();
            if (GetParam() == "memory")
            {
                auto storage = std::make_shared<std::array<uint8_t, 4>>(data);
                stream = CreateStreamFromMemory(std::shared_ptr<const void>(storage, storage->data()), empty ? 0 : data.size());
                return;
            }

            if (directory.empty())
            {
                const auto guid = libCZI::detail::Utilities::GenerateNewGuid();
                std::ostringstream name;
                name << "stream-test-" << std::hex << guid.Data1 << '-' << guid.Data2 << '-' << guid.Data3;
                for (auto byte : guid.Data4)
                {
                    name << '-' << static_cast<unsigned>(byte);
                }

#if defined(_WIN32)
                ASSERT_EQ(_mkdir(name.str().c_str()), 0);
#else
                ASSERT_EQ(mkdir(name.str().c_str(), 0700), 0);
#endif
                directory = name.str();
                filename = directory + "/data.bin";
            }

            {
                std::ofstream file(filename, std::ios::binary | std::ios::trunc);
                ASSERT_TRUE(file.is_open());
                if (!empty)
                {
                    file.write(reinterpret_cast<const char*>(data.data()), data.size());
                }

                file.close();
                ASSERT_TRUE(file.good());
            }

            const std::wstring wide_filename(filename.begin(), filename.end());
            if (GetParam() == "default_file")
            {
                stream = StreamsFactory::CreateDefaultStreamForFile(filename.c_str());
            }
#if LIBCZI_WINDOWSAPI_AVAILABLE
            else if (GetParam() == "windows_io")
            {
                stream = std::make_shared<libCZI::detail::CSimpleInputOutputStreamImplWindows>(wide_filename.c_str());
            }
#endif
#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
            else if (GetParam() == "pread_io")
            {
                stream = std::make_shared<libCZI::detail::CInputOutputStreamImplPreadPwrite>(wide_filename.c_str());
            }
#endif
            else
            {
                StreamsFactory::CreateStreamInfo info;
                info.class_name = GetParam();
                stream = StreamsFactory::CreateStream(info, filename);
            }

            ASSERT_NE(stream, nullptr);
        }

        void SetUp() override
        {
            Open();
        }

        void TearDown() override
        {
            stream.reset();
            if (!filename.empty())
            {
                EXPECT_EQ(std::remove(filename.c_str()), 0);
            }

            if (!directory.empty())
            {
#if defined(_WIN32)
                EXPECT_EQ(_rmdir(directory.c_str()), 0);
#else
                EXPECT_EQ(rmdir(directory.c_str()), 0);
#endif
            }
        }

        void CheckRead(uint64_t offset, uint64_t size, uint64_t expected_count)
        {
            for (bool report_count : { true, false })
            {
                std::array<uint8_t, 8> buffer;
                buffer.fill(0xa5);
                uint64_t count = 99;
                ASSERT_NO_THROW(stream->Read(offset, buffer.data(), size, report_count ? &count : nullptr));
                if (report_count)
                {
                    EXPECT_EQ(count, expected_count);
                }

                for (size_t i = 0; i < buffer.size(); ++i)
                {
                    EXPECT_EQ(buffer[i], i < expected_count ? data[static_cast<size_t>(offset) + i] : 0xa5);
                }
            }
        }
    };

    class EndOfStreamTest : public LocalStreamReadTest {};

    std::vector<std::string> ReadBackends()
    {
        std::vector<std::string> names{ "memory", "default_file", "c_runtime_file_inputstream" };
#if LIBCZI_WINDOWSAPI_AVAILABLE
        names.push_back("windows_file_inputstream");
        names.push_back("windows_io");
#endif
#if LIBCZI_WINDOWS_UWPAPI_AVAILABLE
        names.push_back("uwp_file_inputstream");
#endif
#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
        names.push_back("pread_file_inputstream");
        names.push_back("pread_io");
#endif
        return names;
    }

}

TEST_P(EndOfStreamTest, AtAndBeyondEnd)
{
    CheckRead(data.size(), 1, 0);
    CheckRead(data.size() + 17, 1, 0);
    CheckRead((uint64_t{ 1 } << 32) + data.size(), 1, 0);
    CheckRead(0, data.size(), data.size());
    CheckRead(0, data.size(), data.size());
}

TEST_P(EndOfStreamTest, CrossingEnd)
{
    CheckRead(data.size() - 1, 3, 1);
    CheckRead(0, 8, data.size());
}

TEST_P(EndOfStreamTest, ZeroLength)
{
    CheckRead(0, 0, 0);
    CheckRead(data.size(), 0, 0);
    CheckRead(data.size() + 1, 0, 0);
}

TEST_P(EndOfStreamTest, EmptyStream)
{
    ASSERT_NO_FATAL_FAILURE(Open(true));
    CheckRead(0, 1, 0);
    CheckRead(1, 1, 0);
    CheckRead(0, 0, 0);
    CheckRead(1, 0, 0);
}

INSTANTIATE_TEST_SUITE_P(LocalStreams, EndOfStreamTest, testing::ValuesIn(ReadBackends()),
    [](const testing::TestParamInfo<std::string>& info) { return info.param; });

TEST(StreamsLib, Enumeration)
{
    const int number_of_classes = StreamsFactory::GetStreamClassesCount();
    EXPECT_GT(number_of_classes, 0);

    StreamsFactory::StreamClassInfo info;
    for (int i = 0; i < number_of_classes; ++i)
    {
        const bool b = StreamsFactory::GetStreamInfoForClass(i, info);
        EXPECT_TRUE(b);
        EXPECT_FALSE(info.class_name.empty());
        EXPECT_FALSE(info.short_description.empty());
    }

    // the next value for the index should be invalid now
    const bool b = StreamsFactory::GetStreamInfoForClass(number_of_classes, info);
    EXPECT_FALSE(b);
}

TEST(StreamsLib, TryToInstantiate)
{
    const int number_of_classes = StreamsFactory::GetStreamClassesCount();

    StreamsFactory::StreamClassInfo info;
    for (int i = 0; i < number_of_classes; ++i)
    {
        const bool b = StreamsFactory::GetStreamInfoForClass(i, info);
        ASSERT_TRUE(b);

        StreamsFactory::CreateStreamInfo create_info;
        create_info.class_name = info.class_name;

        // It is reasonable to assume (and therefore checked here) that when passing in an empty filename,
        //  the creation of the stream will fail.
        EXPECT_ANY_THROW(StreamsFactory::CreateStream(create_info, L"")) << "with stream-class \"" << create_info.class_name << "\"";
        EXPECT_ANY_THROW(StreamsFactory::CreateStream(create_info, "")) << "with stream-class \"" << create_info.class_name << "\"";
    }
}

TEST(StreamsLib, TestGetBuildInfoAndCheckThatStringIsNonEmptyIfAvailable)
{
    // here we check that if a stream-class has a non-zero get_build_info function, that this function
    //  would return a non-empty string
    const int number_of_classes = StreamsFactory::GetStreamClassesCount();

    StreamsFactory::StreamClassInfo info;
    for (int i = 0; i < number_of_classes; ++i)
    {
        const bool b = StreamsFactory::GetStreamInfoForClass(i, info);
        ASSERT_TRUE(b);

        if (info.get_build_info)
        {
            const std::string build_info = info.get_build_info();
            EXPECT_FALSE(build_info.empty()) << "for stream-class \"" << info.class_name << "\"";
        }
    }
}

TEST(StreamsLib, TestGetProperty)
{
    // for the classes that have a get_property function, we call into this function
    const int number_of_classes = StreamsFactory::GetStreamClassesCount();

    StreamsFactory::StreamClassInfo info;
    for (int i = 0; i < number_of_classes; ++i)
    {
        const bool b = StreamsFactory::GetStreamInfoForClass(i, info);
        ASSERT_TRUE(b);

        if (info.get_property)
        {
            auto property = info.get_property(StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaInfo);

            // the result should be either invalid or a string
            EXPECT_TRUE(property.GetType() == StreamsFactory::Property::Type::Invalid || property.GetType() == StreamsFactory::Property::Type::String);

            property = info.get_property(StreamsFactory::kStreamClassInfoProperty_CurlHttp_CaPath);

            // the result should be either invalid or a string
            EXPECT_TRUE(property.GetType() == StreamsFactory::Property::Type::Invalid || property.GetType() == StreamsFactory::Property::Type::String);
        }
    }
}

TEST(StreamsLib, TestGetStreamPropertyBagPropertyInfo)
{
    int property_infos_count = -1;
    const auto property_infos = StreamsFactory::GetStreamPropertyBagPropertyInfo(&property_infos_count);

    ASSERT_TRUE(property_infos != nullptr);
    ASSERT_GE(property_infos_count, 0);

    // now, check that the fields 'property_name' and 'property_id' are unique
    for (int i = 0; i < property_infos_count; ++i)
    {
        const auto& info = property_infos[i];
        for (int j = i + 1; j < property_infos_count; ++j)
        {
            const auto& info2 = property_infos[j];
            EXPECT_FALSE(info.property_name == info2.property_name || info.property_id == info2.property_id);
        }
    }

    // check that the list of properties is terminated with an empty entry
    ASSERT_TRUE(property_infos[property_infos_count].property_name == nullptr);
}
