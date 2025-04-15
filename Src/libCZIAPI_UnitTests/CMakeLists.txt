# SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT


ADD_EXECUTABLE(libCZIAPI_UnitTests 
    "test_misc.cpp"
    "test_CZIReader.cpp"
    "main.cpp" 
    "MemoryInputStream.h" 
    "MemoryInputStream.cpp"
    "testdata.h"
    "testdata.cpp" 
    "utilities.h" 
    "utilities.cpp" 
    "MemoryOutputStream.h" 
    "MemoryOutputStream.cpp" 
    "SimpleBitmap.h" 
    "test_parameterhelpers.cpp")

TARGET_LINK_LIBRARIES(libCZIAPI_UnitTests PRIVATE libCZIAPIStatic GTest::gtest GTest::gmock)

add_test(NAME libCZIAPI_UnitTests COMMAND libCZIAPI_UnitTests)
target_include_directories(libCZIAPI_UnitTests PRIVATE ../libCZIAPI/inc ../libCZI "${RAPIDJSON_INCLUDE_DIRS}")
target_compile_definitions(libCZIAPI_UnitTests PRIVATE _LIBCZISTATICLIB)
