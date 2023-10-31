# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
 find_package (Eigen3 3.3 REQUIRED NO_MODULE)
else()
 include(ExternalEIGEN3)
endif()

add_subdirectory(JxrDecode)

if (LIBCZI_BUILD_CURL_BASED_STREAM)
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
    find_package(CURL CONFIG QUIET)
    if (NOT CURL_FOUND)
      message(FATAL_ERROR [=[
        CURL library was not found, which is required for building. Consider installing
        like 'sudo apt-get install libcurl4-openssl-dev' or enable the option 'LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL'.
      ]=])
    endif()

    message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")

  else(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
    # apt install libcurl4-openssl-dev
    message(STATUS "Could not find libcURL.  This dependency will be downloaded.")
    include(FetchContent)
    FetchContent_Declare(
        libcurl
        GIT_REPOSITORY  "https://github.com/curl/curl.git"
        GIT_TAG "curl-8_4_0"
        # Set the prefix to control where it's installed
        PREFIX ${CMAKE_BINARY_DIR}/curl
         # Pass CMake arguments to build curl as a static library
         # for possible options -> c.f https://github.com/curl/curl/blob/master/CMakeLists.txt
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF 
                    -DBUILD_STATIC_LIBS=ON 
                    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/curl 
                    -DENABLE_UNICODE=ON
                    -DCURL_USE_SECTRANSP=ON  # for MacOS - let's choose Apple's native secure transport backend
    )

    FetchContent_MakeAvailable(libcurl)
  endif(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL)
endif(LIBCZI_BUILD_CURL_BASED_STREAM)

add_subdirectory(libCZI)

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_UNITTESTS)
 add_subdirectory(libCZI_UnitTests)
endif(LIBCZI_BUILD_UNITTESTS)