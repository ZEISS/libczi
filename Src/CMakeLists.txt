# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(utilities)
include(CheckCSourceCompiles)

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
 find_package (Eigen3 REQUIRED NO_MODULE)
else()
 include(ExternalEIGEN3)
endif()

# check for availability of "bswap"
#
# c.f. https://stackoverflow.com/questions/41770887/cross-platform-definition-of-byteswap-uint64-and-byteswap-ulong
# not exactly sure why check_symbol_exists() does not work here, but it does not for me - so we do it the hard way and try to compile a test program
check_c_source_compiles(
		"#include <byteswap.h>
		 int main() {__builtin_bswap32(0x12345678);}"
        LIBCZI_BUILDCONFIG_HAS_BUILTIN_BSWAP32)
BoolToYesNo(LIBCZI_BUILDCONFIG_HAS_BUILTIN_BSWAP32 check_result_text)
message("Has_builtin_bswap32 = ${check_result_text}.")
if (NOT LIBCZI_BUILDCONFIG_HAS_BUILTIN_BSWAP32)
	check_symbol_exists(_byteswap_ulong "stdlib.h" LIBCZI_BUILDCONFIG_HAS_BYTESWAP_IN_STDLIB_BSWAP32)
	BoolToYesNo(LIBCZI_BUILDCONFIG_HAS_BYTESWAP_IN_STDLIB_BSWAP32 check_result_text)
	message("Has_byteswap_long_in_stdlib = ${check_result_text}.")
    if (NOT LIBCZI_BUILDCONFIG_HAS_BYTESWAP_IN_STDLIB_BSWAP32)
	  check_symbol_exists(bswap32 "sys/endian.h" LIBCZI_BUILDCONFIG_HAS_BSWAP_IN_SYS_ENDIAN)
      BoolToYesNo(LIBCZI_BUILDCONFIG_HAS_BSWAP_IN_SYS_ENDIAN check_result_text)
	  message("Has_bswap_long_in_sys_endian = ${check_result_text}.")
	endif()
endif()

add_subdirectory(JxrDecode)

# if the build is configured to include the curl-based stream, we need to find or download the curl library
if (LIBCZI_BUILD_CURL_BASED_STREAM)
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
    find_package(CURL CONFIG QUIET)
    if (NOT CURL_FOUND)
      message(STATUS "Did not find a package configuration file provided by CURL, will try to locate CURL package by standard search procedure.")
      find_package(CURL QUIET)
      if (NOT CURL_FOUND)
        message(FATAL_ERROR [=[
          CURL library was not found, which is required for building. Consider installing it with a package manager, something
          like 'sudo apt-get install libcurl4-openssl-dev', or disable the option 'LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL'.
        ]=])
      endif()
    endif()

    message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")

  else(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
    message(STATUS "Attempting to download and build libcURL.")
    include(FetchContent)
    # It seems for MacOS, the secure transport API is deprecated (->  https://curl.se/mail/lib-2023-09/0027.html),
    #  using OpenSSL seems to be the way to go here - so, we better do not default to secure transport here.
    # if (APPLE)  # for MacOS - let's choose Apple's native secure transport backend
    #    set(CURL_USE_SECTRANSP ON)
    # endif(APPLE)

    # On Windows, we can enable the SChannel backend, as it is the native one, and it requires no additional dependencies.
    if(WIN32)
      set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    endif(WIN32)

    FetchContent_Declare(
        libcurl
        GIT_REPOSITORY  "https://github.com/curl/curl.git"
        GIT_TAG "curl-8_9_1"
        # Set the prefix to control where it's installed
        PREFIX "${CMAKE_BINARY_DIR}/vendor/curl"
        )

    # configure libcurl-build as a static library, for possible options -> c.f. https://github.com/curl/curl/blob/master/CMakeLists.txt
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_STATIC_CURL ON CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
    set(ENABLE_UNICODE ON CACHE BOOL "" FORCE)
   
    FetchContent_MakeAvailable(libcurl)
  endif(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
endif(LIBCZI_BUILD_CURL_BASED_STREAM)

if(LIBCZI_BUILD_AZURESDK_BASED_STREAM)
  # -> https://github.com/Azure/azure-sdk-for-cpp#azure-sdk-for-c
  # -> https://learn.microsoft.com/en-us/azure/storage/blobs/quickstart-blobs-c-plus-plus?tabs=managed-identity%2Croles-azure-portal
  find_package(azure-identity-cpp CONFIG REQUIRED)
  find_package(azure-storage-blobs-cpp CONFIG REQUIRED)
  set(LIBCZI_AZURESDK_VERSION_STRING "core:${azure-core-cpp_VERSION} identity:${azure-identity-cpp_VERSION} storage-blobs:${azure-storage-blobs-cpp_VERSION}")
  message(STATUS "AZURE-SDK available, version-info: ${LIBCZI_AZURESDK_VERSION_STRING}")
endif()

add_subdirectory(libCZI)

# CZICmd and libCZIAPI are dependent on RapidJSON, so we need to find or download it.
if (LIBCZI_BUILD_CZICMD OR LIBCZI_BUILD_LIBCZIAPI)
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON)
      find_package(RapidJSON QUIET)
      if (NOT RapidJSON_FOUND)
          message(FATAL_ERROR [=[
          RapidJSON library was not found, which is required for building. Consider installing
          like 'sudo apt-get install rapidjson-dev'. Alternatively, consider setting the option
          LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON to OFF in order to download and build RapidJSON
          automatically as part of the build process.
          ]=])
      endif()
  else()
      # Since "RapidJSON" is a header-only library, we just have to download it and point to the include directory.
      # Note: when using v1.1.0 of RapidJSON (the latest release) there we problems (with GCC14.2 with msys2), so
      #        we use a later version from the master branch.
      include(FetchContent)
      FetchContent_Declare(
              RapidJSON
              GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
              GIT_TAG        24b5e7a8b27f42fa16b96fc70aade9106cf7102f # master as of 2025-04-01
              PREFIX "${CMAKE_BINARY_DIR}/vendor/rapidjson"
      )

      set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
      set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
      FetchContent_MakeAvailable(RapidJSON)

      set(RAPIDJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
  endif()
endif()

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_LIBCZIAPI)
  add_subdirectory(libCZIAPI)
endif(LIBCZI_BUILD_LIBCZIAPI)

if (LIBCZI_BUILD_UNITTESTS)
  include(FetchContent)

  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  if(${CMAKE_VERSION}  VERSION_GREATER_EQUAL "3.24.0")
	  FetchContent_Declare(
		  googletest
		  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
		  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  else ()
	  FetchContent_Declare(
		  googletest
		  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  endif()

  FetchContent_MakeAvailable(googletest)

 add_subdirectory(libCZI_UnitTests)

  if (LIBCZI_BUILD_LIBCZIAPI)
    add_subdirectory(libCZIAPI_UnitTests)
  endif()
endif(LIBCZI_BUILD_UNITTESTS)

