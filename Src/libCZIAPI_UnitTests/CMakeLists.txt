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
    "test_parameterhelpers.cpp"
    "test_accessors.cpp")

TARGET_LINK_LIBRARIES(libCZIAPI_UnitTests PRIVATE libCZIAPIStatic GTest::gtest GTest::gmock)

target_compile_definitions(libCZIAPI_UnitTests PRIVATE _LIBCZISTATICLIB _LIBCZIAPISTATICLIB)
target_include_directories(libCZIAPI_UnitTests PRIVATE
    # Public API headers of libCZIAPI (e.g. libCZIApi.h)
    ../libCZIAPI/inc
    # Private/internal headers of libCZIAPI - needed because some tests directly
    # include non-public headers such as <../src/parameterhelpers.h>.
    ../libCZI
    # Pull in libCZIStatic's full set of INTERFACE include directories. This is
    # necessary because libCZI_Config.h is a generated header that CMake places in
    # the *build* directory (${CMAKE_CURRENT_BINARY_DIR} of Src/libCZI), not in the
    # source tree.  libCZIStatic advertises that path as a PUBLIC include directory,
    # but libCZIAPIStatic links libCZIStatic only PRIVATE, so the path is not
    # propagated transitively to consumers of libCZIAPIStatic.  By reading it
    # directly from the target property here we get both the source dir and the
    # build dir without having to hard-code any paths.
    $<TARGET_PROPERTY:libCZIStatic,INTERFACE_INCLUDE_DIRECTORIES>
    "${RAPIDJSON_INCLUDE_DIRS}")

include(GoogleTest)

if(CMAKE_CROSSCOMPILING)
    add_test(NAME libCZIAPI_UnitTests COMMAND libCZIAPI_UnitTests)
else()
    gtest_discover_tests(libCZIAPI_UnitTests)
endif()
