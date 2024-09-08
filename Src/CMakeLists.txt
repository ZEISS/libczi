# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
 find_package (Eigen3 3.3 REQUIRED NO_MODULE)
else()
 include(ExternalEIGEN3)
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
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_AZURESDK)
    find_package(azure-identity-cpp CONFIG REQUIRED)
    find_package(azure-storage-blobs-cpp CONFIG REQUIRED)
    set(LIBCZI_AZURESDK_VERSION_STRING "core:${azure-core-cpp_VERSION} identity:${azure-identity-cpp_VERSION} storage-blobs:${azure-storage-blobs-cpp_VERSION}")
    message(STATUS "******************** ${LIBCZI_AZURESDK_VERSION_STRING} ********************")
  else()
    include(FetchContent)

    # -> https://github.com/Azure/azure-sdk-for-cpp/tree/main/samples/integration/cmake-fetch-content/

    #find_package(azure_macro_utils_c REQUIRED CONFIG)

        # Fetch the WIL repository
    FetchContent_Declare(
        wil
        GIT_REPOSITORY https://github.com/microsoft/wil.git
       # GIT_TAG        master  # Or specify a specific version/commit
    )
    # Make the content available
    set(WIL_BUILD_TESTS OFF)
    set(WIL_BUILD_PACKAGING OFF)
    set(FAST_BUILD ON)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/external/wil")
    FetchContent_MakeAvailable(wil)
    message(STATUS "********** ${wil_SOURCE_DIR} **********")

    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/cmake/dummy")


    FetchContent_Declare(azure_macro_utils_c
        # Set the SDK URL path and release tag
        GIT_REPOSITORY      https://github.com/Azure/macro-utils-c.git
        #GIT_TAG             azure-storage-files-datalake_12.0.0-beta.6)
        GIT_TAG             552dfadfca17c2fb3bd81c44bcbb44ce205817af)
    FetchContent_MakeAvailable(azure_macro_utils_c)
    message(STATUS "$$$$$$$$$$ ${azure_macro_utils_c_SOURCE_DIR} $$$$$$$$$$")
    set(azure_c_shared_utility_DIR  "D:/dev/Github/libczi-zeiss-ptahmose/build/_deps/azure-c-shared-utility-build")


    
    FetchContent_Declare(umock-c
        # Set the SDK URL path and release tag
        GIT_REPOSITORY      https://github.com/Azure/umock-c.git
        GIT_TAG             master)
    FetchContent_MakeAvailable(umock-c)
    message(STATUS "########## ${umock-c_SOURCE_DIR} ##########")

    FetchContent_Declare(azure-c-shared-utility
        # Set the SDK URL path and release tag
        GIT_REPOSITORY    https://github.com/Azure/azure-c-shared-utility.git
        GIT_TAG             master)
    FetchContent_MakeAvailable(azure-c-shared-utility)
    message(STATUS "++++++++++ ${azure-c-shared-utility_SOURCE_DIR} ++++++++++")
   # set(azure_c_shared_utility_DIR "D:/dev/Github/libczi-zeiss-ptahmose/build/_deps/azure-c-shared-utility-src")
   set(azure_c_shared_utility_DIR  "D:/dev/Github/libczi-zeiss-ptahmose/build/_deps/azure-c-shared-utility-src/configs")


    set(BUILD_TESTING OFF)
    FetchContent_Declare(opentelemetry-cpp
      GIT_REPOSITORY    https://github.com/open-telemetry/opentelemetry-cpp.git
      GIT_TAG           main)
    FetchContent_MakeAvailable(opentelemetry-cpp)
    message(STATUS "&&&&&&&&&& ${opentelemetry-cpp_SOURCE_DIR} &&&&&&&&&&")


    #FetchContent_GetProperties(azure_macro_utils_c)
    #if(NOT azure_macro_utils_c_POPULATED)
    #    #FetchContent_Populate(azure_macro_utils_c)
    #    #add_subdirectory(${MACRO_UTILS_INC_FOLDER} )
    #    #add_subdirectory(deps/macro-utils-c)
    #    include_directories(${MACRO_UTILS_INC_FOLDER})
    #endif()




   # find_package(umock_c REQUIRED CONFIG)
   # find_package(azure_c_shared_utility REQUIRED CONFIG)

    set(MSVC_USE_STATIC_CRT  ON CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_PERFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
    set(AZURE_SDK_DISABLE_AUTO_VCPKG ON CACHE BOOL "" FORCE)
    #set(FETCH_SOURCE_DEPS ON CACHE BOOL "" FORCE)
    set(AZ_ALL_LIBRARIES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(azuresdkforcpp
        # Set the SDK URL path and release tag
        GIT_REPOSITORY      https://github.com/Azure/azure-sdk-for-cpp.git
        #GIT_TAG             azure-storage-files-datalake_12.0.0-beta.6)
        GIT_TAG             azure-storage-blobs_12.10.0)
    FetchContent_MakeAvailable(azuresdkforcpp)
#    FetchContent_GetProperties(azuresdkforcpp)
#    if(NOT azuresdkforcpp_POPULATED)
#        FetchContent_Populate(azuresdkforcpp)
#        add_subdirectory(${azuresdkforcpp_SOURCE_DIR} ${azuresdkforcpp_BINARY_DIR} EXCLUDE_FROM_ALL)
#    endif()
  endif()
endif()



add_subdirectory(libCZI)

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_UNITTESTS)
 add_subdirectory(libCZI_UnitTests)
endif(LIBCZI_BUILD_UNITTESTS)
