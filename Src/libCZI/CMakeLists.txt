# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(FetchContent)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CMakePackageConfigHelpers)

set(LIBCZISRCFILES 
            BitmapOperations.cpp
            CreateBitmap.cpp
            CziAttachment.cpp
            CziAttachmentsDirectory.cpp
            CziDimensionInfo.cpp
            CziDisplaySettings.cpp
            CziMetadata.cpp
            CziMetadataBuilder.cpp
            CziMetadataDocumentInfo.cpp
            CziMetadataDocumentInfo2.cpp
            CziMetadataSegment.cpp
            CziParse.cpp
            CZIReader.cpp
            CziReaderCommon.cpp
            CziReaderWriter.cpp
            CziStructs.cpp
            CziSubBlock.cpp
            CziSubBlockDirectory.cpp
            CziUtils.cpp
            CziWriter.cpp
            decoder.cpp
            decoder_wic.cpp
            decoder_zstd.cpp
            DimCoordinate.cpp
            IndexSet.cpp
            libCZI_Lib.cpp
            libCZI_Site.cpp
            libCZI_Utilities.cpp
            MD5Sum.cpp
            MultiChannelCompositor.cpp
            pugixml.cpp
            SingleChannelAccessorBase.cpp
            SingleChannelPyramidLevelTileAccessor.cpp
            SingleChannelScalingTileAccessor.cpp
            SingleChannelTileAccessor.cpp
            SingleChannelTileCompositor.cpp
            splines.cpp
            stdAllocator.cpp
            StreamImpl.cpp
            utilities.cpp
            utilities_simd.cpp
            zstdCompress.cpp
            bitmapData.h
            BitmapOperations.h
            CziAttachment.h
            CziAttachmentsDirectory.h
            CziDimensionInfo.h
            CziDisplaySettings.h
            CziMetadata.h
            CziMetadataBuilder.h
            CziMetadataDocumentInfo.h
            CziMetadataDocumentInfo2.h
            CziMetadataSegment.h
            CziParse.h
            CziReaderCommon.h
            CZIReader.h
            CziReaderWriter.h
            CziStructs.h
            CziSubBlock.h
            CziSubBlockDirectory.h
            CziUtils.h
            CziWriter.h
            decoder.h
            decoder_wic.h
            decoder_zstd.h
            FileHeaderSegmentData.h
            ImportExport.h
            inc_libCZI_Config.h
            IndexSet.h
            libCZI.h
            libCZI_Compositor.h
            libCZI_compress.h
            libCZI_DimCoordinate.h
            libCZI_exceptions.h
            libCZI_Helpers.h
            libCZI_Metadata.h
            libCZI_Metadata2.h
            libCZI_Pixels.h
            libCZI_ReadWrite.h
            libCZI_Site.h
            libCZI_Utilities.h
            libCZI_Write.h
            libCZI_StreamsLib.h
            MD5Sum.h
            MultiChannelCompositor.h
            SingleChannelAccessorBase.h
            SingleChannelPyramidLevelTileAccessor.h
            SingleChannelScalingTileAccessor.h
            SingleChannelTileAccessor.h
            SingleChannelTileCompositor.h
            Site.h
            splines.h
            stdAllocator.h
            StreamImpl.h
            utilities.h
            XmlNodeWrapper.h
            BitmapOperations.hpp
            pugiconfig.hpp
            pugixml.hpp
            jxrlibcompress.cpp
            StreamsLib/streamsFactory.cpp
            StreamsLib/curlhttpinputstream.cpp
            StreamsLib/curlhttpinputstream.h
            StreamsLib/windowsfileinputstream.cpp
            StreamsLib/windowsfileinputstream.h
            StreamsLib/simplefileinputstream.cpp
            StreamsLib/simplefileinputstream.h
            StreamsLib/preadfileinputstream.cpp
            StreamsLib/preadfileinputstream.h
            StreamsLib/azureblobinputstream.h
            StreamsLib/azureblobinputstream.cpp
            subblock_cache.h
            subblock_cache.cpp
)

# prepare the configuration-file "libCZI_Config.h"
if (IS_BIG_ENDIAN)
 set(libCZI_ISBIGENDIANHOST 1)
else()
 set(libCZI_ISBIGENDIANHOST 0)
endif()

if (HAVE_ALIGNED_ALLOC)
 set(libCZI_HAVE_ALIGNED_ALLOC 1)
else()
 set(libCZI_HAVE_ALIGNED_ALLOC 0)
endif()
if (HAVE__ALIGNED_MALLOC)
 set(libCZI_HAVE__ALIGNED_MALLOC 1)
else()
 set(libCZI_HAVE__ALIGNED_MALLOC 0)
endif()

set(libCZI_HAVE_ENDIAN_H ${HAVE_ENDIAN_H})

if (CRASH_ON_UNALIGNED_ACCESS)
  set (libCZI_CrashOnUnalignedIntegers 1)
else()
  set (libCZI_CrashOnUnalignedIntegers 0)
endif()

if(NOT WIN32 AND HAVE_FCNTL_H_OPEN AND HAVE_UNISTD_H_PREAD AND HAVE_UNISTD_H_PWRITE)
  set(libCZI_UsePreadPwriteBasedStreamImplementation 1)
else()
  set(libCZI_UsePreadPwriteBasedStreamImplementation 0)
endif()

string(CONCAT libCZI_CompilerIdentification ${CMAKE_CXX_COMPILER_ID} " " ${CMAKE_CXX_COMPILER_VERSION} )

# get the URL and the hash of the source code we are building
# the strategy is:
# 1. allow for the information to be externally provided (via the variable LIBCZI_REPOSITORY_REMOTE, LIBCZI_REPOSITORY_BRANCH, LIBCZI_REPOSITORY_HASH)
# 2. if not provided, try to get the information by trying to run git commands
# 3. if that fails, set the information to "unknown"
# Use values from cache if set by portfile or other means
set(libCZI_REPOSITORYREMOTEURL "${LIBCZI_REPOSITORY_REMOTE}" CACHE STRING "")
set(libCZI_REPOSITORYBRANCH "${LIBCZI_REPOSITORY_BRANCH}" CACHE STRING "")
set(libCZI_REPOSITORYHASH "${LIBCZI_REPOSITORY_HASH}" CACHE STRING "")
# Fallback to git commands if not already set
if (NOT libCZI_REPOSITORYREMOTEURL)
  execute_process(
    COMMAND git remote get-url origin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE libCZI_REPOSITORYREMOTEURL
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
if (NOT libCZI_REPOSITORYBRANCH)
  execute_process(
    COMMAND git name-rev --name-only HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE libCZI_REPOSITORYBRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
if (NOT libCZI_REPOSITORYHASH)
  execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE libCZI_REPOSITORYHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
# Final fallback
if (NOT libCZI_REPOSITORYREMOTEURL)
  set(libCZI_REPOSITORYREMOTEURL "unknown")
endif()
if (NOT libCZI_REPOSITORYBRANCH)
  set(libCZI_REPOSITORYBRANCH "unknown")
endif()
if (NOT libCZI_REPOSITORYHASH)
  set(libCZI_REPOSITORYHASH "unknown")
endif()

# check whether we can use AVX2-intrinsics (on x86/x64) -> we check for the presence of the file "immintrin.h"
# (note that checking for presence of "immintrin.h" is not sufficient, as there seem to be different versions of this file, and
#  e.g. _mm256_storeu2_m128i is not available with older GCC-versions)
check_symbol_exists( _mm256_storeu2_m128i immintrin.h IMMINTRINFOUND)
if (IMMINTRINFOUND)
 set(libCZI_HAS_AVXINTRINSICS 1)
else()
 set(libCZI_HAS_AVXINTRINSICS 0)
endif()

# check whether we can use NEON-intrinsics (on ARM) -> 
set(libCZI_HAS_NEOININTRINSICS 0)
if (NOT libCZI_HAS_AVXINTRINSICS)
  CHECK_INCLUDE_FILES(arm_neon.h ARMNEONFOUND)
  if (ARMNEONFOUND)
    if (MSVC)
      # for Windows, we can safely assume that Neon is supported
      set(libCZI_HAS_NEOININTRINSICS 1)
    else()
      include (check_can_use_neon_intrinsics)
      CHECK_CAN_USE_NEON_INTRINSICS(NEON_INTRINSICS_CAN_BE_USED)
      if (NEON_INTRINSICS_CAN_BE_USED)
        set(libCZI_HAS_NEOININTRINSICS 1)
      endif()
    endif()
  endif()
endif()

if (libCZI_HAS_AVXINTRINSICS)
  IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # for GCC/Clang, we need to enable avx-support for the file with AVX-code
    set_source_files_properties(utilities_simd.cpp PROPERTIES COMPILE_OPTIONS "-mavx2")
  ENDIF()
endif()

if (LIBCZI_BUILD_CURL_BASED_STREAM)
  set(libCZI_libcurl_available 1)
else()
  set(libCZI_libcurl_available 0)
endif()

if (LIBCZI_BUILD_AZURESDK_BASED_STREAM)
  set(libCZI_AzureStorage_available 1)
  set(libCZI_AzureStorage_SDK_Version_Info "${LIBCZI_AZURESDK_VERSION_STRING}")
else()
  set(libCZI_AzureStorage_available 0)
  set(libCZI_AzureStorage_SDK_Version_Info "not available")
endif()


if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
  find_package(zstd CONFIG REQUIRED)
  if (TARGET zstd::libzstd_static)               # static triplet
    set(LIBCZI_ZSTD_LINK_TARGET "zstd::libzstd_static")
  elseif (TARGET zstd::libzstd_shared)           # shared triplet
    set(LIBCZI_ZSTD_LINK_TARGET "zstd::libzstd_shared")
  else()
    message(FATAL_ERROR "No suitable zstd target found")
  endif()
else()
  FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG "v1.5.7"
  )

  message(STATUS "Fetching zstd...")
  set(ZSTD_BUILD_PROGRAMS  OFF CACHE BOOL "" FORCE)
  set(ZSTD_BUILD_SHARED    OFF CACHE BOOL "" FORCE)
  set(ZSTD_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
  set(ZSTD_BUILD_CONTRIB   OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(zstd)

  add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
endif()

if (LIBCZI_HAVE_WIN32_API)
  set(libCZI_WINDOWSAPIAVAILABLE 1)
else()
  set(libCZI_WINDOWSAPIAVAILABLE 0)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/libCZI_Config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/libCZI_Config.h"
  ESCAPE_QUOTES @ONLY)


#  Define headers for this library. PUBLIC headers are used for compiling the library, and will be added to consumers' build paths.
set(libCZIPublicHeaders "ImportExport.h" "libCZI.h" "libCZI_Compositor.h" "libCZI_DimCoordinate.h" "libCZI_exceptions.h"
               "libCZI_Helpers.h" "libCZI_Metadata.h" "libCZI_Metadata2.h" "libCZI_Pixels.h" "libCZI_ReadWrite.h"
               "libCZI_Site.h" "libCZI_Utilities.h" "libCZI_Write.h" "libCZI_compress.h" "libCZI_StreamsLib.h")

#
#define the shared libCZI - library
#
if (LIBCZI_BUILD_DYNLIB)
  add_library(libCZI SHARED ${LIBCZISRCFILES} $<TARGET_OBJECTS:JxrDecodeStatic>)
 
  set_target_properties(libCZI PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES) # https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/
  SET_TARGET_PROPERTIES (libCZI PROPERTIES DEFINE_SYMBOL  "LIBCZI_EXPORTS" )
  set_target_properties(libCZI PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
  # add the binary tree to the search path for include files so that we will find libCZI_Config.h
  target_include_directories(libCZI PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}")
  target_include_directories(libCZI PRIVATE  ${EIGEN3_INCLUDE_DIR})
  set_target_properties(libCZI PROPERTIES DEBUG_POSTFIX "d")
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
   target_link_libraries(libCZI PRIVATE ${LIBCZI_ZSTD_LINK_TARGET})
  else()
   target_link_libraries(libCZI PRIVATE libzstd_static)
   target_include_directories(libCZI PRIVATE ${zstd_SOURCE_DIR}/lib)
  endif()
  if (LIBCZI_BUILD_CURL_BASED_STREAM)
    target_link_libraries(libCZI PRIVATE CURL::libcurl)
  endif()
  if (LIBCZI_BUILD_AZURESDK_BASED_STREAM)
    target_link_libraries(libCZI PRIVATE Azure::azure-identity Azure::azure-storage-blobs)
  endif()
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
   target_link_libraries(libCZI PRIVATE Eigen3::Eigen)
  else()
   add_dependencies(libCZI eigen_ext)
  endif()
  IF(UNIX)
    target_compile_options(libCZI PRIVATE -D_FILE_OFFSET_BITS=64)
  ENDIF(UNIX)
  if(LIBCZI_HAVE_WIN32_API)
    target_link_libraries(libCZI PRIVATE ole32 Windowscodecs)
  endif()
  target_include_directories(libCZI
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)
endif(LIBCZI_BUILD_DYNLIB)

#
#define the static libCZI - library
#
# Notes: -we use JxrDecode as an "object-library" in order have it "embedded" into libCZI.a
add_library(libCZIStatic STATIC ${LIBCZISRCFILES} $<TARGET_OBJECTS:JxrDecodeStatic>)
set_target_properties(libCZIStatic PROPERTIES CXX_STANDARD 14)
target_compile_definitions(libCZIStatic PRIVATE _LIBCZISTATICLIB)
set_target_properties(libCZIStatic PROPERTIES VERSION ${PROJECT_VERSION})
# add the binary tree to the search path for include files so that we will find libCZI_Config.h
target_include_directories(libCZIStatic PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(libCZIStatic PRIVATE  ${EIGEN3_INCLUDE_DIR})
set_target_properties(libCZIStatic PROPERTIES DEBUG_POSTFIX "d")
if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
   target_link_libraries(libCZIStatic PUBLIC ${LIBCZI_ZSTD_LINK_TARGET})
else()
   target_link_libraries(libCZIStatic PRIVATE libzstd_static)
   target_include_directories(libCZIStatic PRIVATE ${zstd_SOURCE_DIR}/lib)
endif()

if (LIBCZI_BUILD_CURL_BASED_STREAM)
  target_link_libraries(libCZIStatic PRIVATE CURL::libcurl)
endif()
if (LIBCZI_BUILD_AZURESDK_BASED_STREAM)
  target_link_libraries(libCZIStatic PRIVATE Azure::azure-identity Azure::azure-storage-blobs)
endif()

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
  target_link_libraries(libCZIStatic PUBLIC Eigen3::Eigen)
else()
  add_dependencies(libCZIStatic eigen_ext)
endif()
IF(UNIX)
  target_compile_options(libCZIStatic PRIVATE -D_FILE_OFFSET_BITS=64)
  set_property(TARGET libCZIStatic PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF(UNIX)
if(LIBCZI_HAVE_WIN32_API)
    target_link_libraries(libCZIStatic PUBLIC ole32 Windowscodecs)
endif()
target_compile_definitions(libCZIStatic PUBLIC _LIBCZISTATICLIB)    # make sure that users of the static library get the correct definition

# Static library include directories
target_include_directories(libCZIStatic
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

if(LIBCZI_ENABLE_INSTALL)

    string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWERCASE)
    set(INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWERCASE}")

    # Shared lib install
    if(LIBCZI_BUILD_DYNLIB)
        install(TARGETS libCZI
                EXPORT libCZITargets
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel)

    else()
        # Static lib install
        install(TARGETS libCZIStatic
                EXPORT libCZITargets
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
    endif()

    # Manual install of public headers to include/libCZI (simulate structure without moving files)
    install(FILES ${libCZIPublicHeaders}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libCZI)

    # Export target definitions
    if(TARGET libCZI)
        set(_libczi_target libCZI)
    elseif(TARGET libCZIStatic)
        set(_libczi_target libCZIStatic)
    endif()
    if(DEFINED _libczi_target)
        install(EXPORT libCZITargets
                NAMESPACE libczi::
                DESTINATION ${INSTALL_CMAKEDIR}
                FILE libCZITargets.cmake
                EXPORT_LINK_INTERFACE_LIBRARIES)
    endif()

    # Generate and install libcziConfig.cmake and version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/${PROJECT_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${INSTALL_CMAKEDIR}
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${INSTALL_CMAKEDIR}
    )

endif()