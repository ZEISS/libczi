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
