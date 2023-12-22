# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(FetchContent)
include(CheckIncludeFiles)
include(CheckSymbolExists)

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

# get the URL of the upstream repository
execute_process(
  COMMAND git remote get-url origin
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE libCZI_REPOSITORYREMOTEURL
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT DEFINED libCZI_REPOSITORYREMOTEURL OR "${libCZI_REPOSITORYREMOTEURL}" STREQUAL "")
  set(libCZI_REPOSITORYREMOTEURL "unknown")
endif()

execute_process(
  COMMAND git name-rev --name-only HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE libCZI_REPOSITORYBRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT DEFINED libCZI_REPOSITORYBRANCH OR "${libCZI_REPOSITORYBRANCH}" STREQUAL "")
  set(libCZI_REPOSITORYBRANCH "unknown")
endif()

execute_process(
  COMMAND  git log -1 --format=%H
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE libCZI_REPOSITORYHASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT DEFINED libCZI_REPOSITORYHASH OR "${libCZI_REPOSITORYHASH}" STREQUAL "")
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

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
  find_package(zstd CONFIG REQUIRED)
else()
  FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG "v1.5.5"
  )

  if(NOT zstd_POPULATED)
    message(STATUS "Fetching zstd...")
    set(ZSTD_BUILD_PROGRAMS  OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED    OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_CONTRIB   OFF CACHE BOOL "" FORCE)

    FetchContent_Populate(zstd)

    add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
  endif()
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
 
  set_target_properties(libCZI PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES) # https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/
  SET_TARGET_PROPERTIES (libCZI PROPERTIES DEFINE_SYMBOL  "LIBCZI_EXPORTS" )
  set_target_properties(libCZI PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
  # add the binary tree to the search path for include files so that we will find libCZI_Config.h
  target_include_directories(libCZI PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}")
  target_include_directories(libCZI PRIVATE  ${EIGEN3_INCLUDE_DIR})
  set_target_properties(libCZI PROPERTIES DEBUG_POSTFIX "d")
  set_target_properties(libCZI PROPERTIES PUBLIC_HEADER "${libCZIPublicHeaders}")
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
   target_link_libraries(libCZI PRIVATE zstd::libzstd_static)
  else()
   target_link_libraries(libCZI PRIVATE libzstd_static)
   target_include_directories(libCZI PRIVATE ${zstd_SOURCE_DIR}/lib)
  endif()
  if (LIBCZI_BUILD_CURL_BASED_STREAM)
    target_link_libraries(libCZI PRIVATE CURL::libcurl)
  endif()
  if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
   target_link_libraries(libCZI PRIVATE Eigen3::Eigen)
  else()
   add_dependencies(libCZI eigen_ext)
  endif()
  IF(UNIX)
    target_compile_options(libCZI PRIVATE -D_FILE_OFFSET_BITS=64)
  ENDIF(UNIX)
endif(LIBCZI_BUILD_DYNLIB)

#
#define the static libCZI - library
#
# Notes: -we use JxrDecode as an "object-library" in order have it "embedded" into libCZI.a
add_library(libCZIStatic STATIC ${LIBCZISRCFILES} $<TARGET_OBJECTS:JxrDecodeStatic>)
set_target_properties(libCZIStatic PROPERTIES CXX_STANDARD 11)
target_compile_definitions(libCZIStatic PRIVATE _LIBCZISTATICLIB)
set_target_properties(libCZIStatic PROPERTIES VERSION ${PROJECT_VERSION})
# add the binary tree to the search path for include files so that we will find libCZI_Config.h
target_include_directories(libCZIStatic PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(libCZIStatic PRIVATE  ${EIGEN3_INCLUDE_DIR})
set_target_properties(libCZIStatic PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(libCZIStatic PROPERTIES PUBLIC_HEADER "${libCZIPublicHeaders}")
if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD)
   target_link_libraries(libCZIStatic PRIVATE zstd::libzstd_static)
else()
   target_link_libraries(libCZIStatic PRIVATE libzstd_static)
   target_include_directories(libCZIStatic PRIVATE ${zstd_SOURCE_DIR}/lib)
endif()

if (LIBCZI_BUILD_CURL_BASED_STREAM)
  target_link_libraries(libCZIStatic PRIVATE CURL::libcurl)
endif()

if (LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3)
  target_link_libraries(libCZIStatic PRIVATE Eigen3::Eigen)
else()
  add_dependencies(libCZIStatic eigen_ext)
endif()
IF(UNIX)
  target_compile_options(libCZIStatic PRIVATE -D_FILE_OFFSET_BITS=64)
  set_property(TARGET libCZIStatic PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF(UNIX)


#
# Notes: - the variables CMAKE_INSTALL_<...> have been defined in the module "GNUInstallDirs"
#install(FILES ${libCZIPublicHeaders} DESTINATION  ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
if (LIBCZI_BUILD_DYNLIB)
  install(TARGETS libCZI
  #		 EXPORT "libCZI"
     ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
     LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
     RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
     PUBLIC_HEADER DESTINATION  ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
endif(LIBCZI_BUILD_DYNLIB)

install(TARGETS libCZIStatic
#		 EXPORT "libCZIStatic"
     ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
     LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
     RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
     PUBLIC_HEADER DESTINATION  ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
