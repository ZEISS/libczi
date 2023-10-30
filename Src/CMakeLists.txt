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
  find_package(CURL CONFIG )
  # apt install libcurl4-openssl-dev
# #[[
  if(${CURL_FOUND})
    message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")
  else(${CURL_FOUND})
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
    )

    FetchContent_MakeAvailable(libcurl)
  endif(${CURL_FOUND})

endif(LIBCZI_BUILD_CURL_BASED_STREAM)

add_subdirectory(libCZI)

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_UNITTESTS)
 add_subdirectory(libCZI_UnitTests)
endif(LIBCZI_BUILD_UNITTESTS)