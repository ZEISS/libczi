// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>
#include "inc_libCZI.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // perform the "one-time initialization" of the libCZI streams factory
    libCZI::StreamsFactory::Initialize();

    const int ret = RUN_ALL_TESTS();
    return ret;
}
