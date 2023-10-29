# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
 find_package (Eigen3 3.3 REQUIRED NO_MODULE)
else()
 include(ExternalEIGEN3)
endif()

add_subdirectory(JxrDecode)

if (LIBCZI_BUILD_STREAMSLIB)
  find_package(CURL CONFIG)
  if(${CURL_FOUND})
    message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")
  else(${CURL_FOUND})
    message(STATUS "Could not find libcURL.  This dependency will be downloaded.")
    ExternalProject_Add(
        libcurl
        GIT_REPOSITORY  "https://github.com/curl/curl.git"
        GIT_TAG "curl-8_4_0"
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/curl
        PATCH_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/cURL/buildconf
        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/cURL/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND ${MAKE}
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_UPDATE ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_TEST ON
        LOG_INSTALL ON
    )
    ExternalProject_Get_Property(libcurl source_dir)
    ExternalProject_Get_Property(libcurl binary_dir)

    # Define an imported target
    add_library(CURL::libcurl IMPORTED GLOBAL)

   # Set properties for the imported target
   set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION ${binary_dir}
    INTERFACE_INCLUDE_DIRECTORIES ${source_dir}
    )

   add_dependencies(CURL::libcurl libcurl)


    #set(CURL_SOURCE_DIR ${source_dir})
    #set(CURL_BINARY_DIR ${binary_dir})
    #set(CURL_LIBRARIES ${CURL_BINARY_DIR}/lib/.libs/libcurl.dylib)
    #include_directories(${CURL_SOURCE_DIR})
    #set(DEPENDENCIES ${DEPENDENCIES} libcurl)
  endif(${CURL_FOUND})

endif(LIBCZI_BUILD_STREAMSLIB)

add_subdirectory(libCZI)

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_UNITTESTS)
 add_subdirectory(libCZI_UnitTests)
endif(LIBCZI_BUILD_UNITTESTS)