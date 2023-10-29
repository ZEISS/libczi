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
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF 
                    -DBUILD_STATIC_LIBS=ON 
                    -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/curl 
                    -DENABLE_UNICODE=ON  
    )

    FetchContent_MakeAvailable(libcurl)
#    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/curl
#        PATCH_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/cURL/buildconf
#        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/cURL/configure --prefix=<INSTALL_DIR>
#        BUILD_COMMAND ${MAKE}
#        UPDATE_COMMAND ""
#        INSTALL_COMMAND ""
#        LOG_DOWNLOAD ON
#        LOG_UPDATE ON
#        LOG_CONFIGURE ON
#        LOG_BUILD ON
#        LOG_TEST ON
#        LOG_INSTALL ON
#[[
    # Specify the include directories
    #ExternalProject_Get_Property(libcurl INSTALL_DIR)
    message(STATUS "********** CURL-INSTALL_DIR **********: ${INSTALL_DIR}")
    #ExternalProject_Get_Property(libcurl source_dir)
    #ExternalProject_Get_Property(libcurl binary_dir)

    # Define an imported target
    #add_library(CURL::libcurl STATIC IMPORTED GLOBAL)


    #message(STATUS "********** CURL-source **********: ${source_dir}")

    #add_dependencies(CURL::libcurl libcurl)

    #add_custom_target(set_curl_properties
    #COMMAND ${CMAKE_COMMAND} -P set_curl_properties.cmake)
    #add_dependencies(set_curl_properties libcurl)

   # Set properties for the imported target
#   set_target_properties(CURL::libcurl PROPERTIES
#    IMPORTED_LOCATION ${binary_dir}
#    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/curl/include"
#    )

    set(CURL_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/curl/include)
    set(CURL_LIBRARIES ${CMAKE_BINARY_DIR}/curl/lib/libcurl.a)

    #find_package(CURL REQUIRED)
    message(STATUS "********** CURL_INCLUDE_DIRS **********: ${CURL_INCLUDE_DIRS}")
    #set(CURL_SOURCE_DIR ${source_dir})
    #set(CURL_BINARY_DIR ${binary_dir})
    #set(CURL_LIBRARIES ${CURL_BINARY_DIR}/lib/.libs/libcurl.dylib)
    #include_directories(${CURL_SOURCE_DIR})
    #set(DEPENDENCIES ${DEPENDENCIES} libcurl)
    set(CURL_DOWNLOADED_AND_BUILT TRUE)
      #]]
  endif(${CURL_FOUND})

endif(LIBCZI_BUILD_CURL_BASED_STREAM)

add_subdirectory(libCZI)

if (LIBCZI_BUILD_CZICMD)
 add_subdirectory(CZICmd)
endif(LIBCZI_BUILD_CZICMD)

if (LIBCZI_BUILD_UNITTESTS)
 add_subdirectory(libCZI_UnitTests)
endif(LIBCZI_BUILD_UNITTESTS)