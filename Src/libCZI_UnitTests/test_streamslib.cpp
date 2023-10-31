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
    for (int i=0;i<number_of_classes;++i)
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
    
