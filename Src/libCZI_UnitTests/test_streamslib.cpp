// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"

using namespace libCZI;

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
