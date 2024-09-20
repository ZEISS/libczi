# Install script for directory: /d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/zstd-build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/Src/libCZI/liblibCZI.dll.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/Src/libCZI/msys-libCZI-1.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/msys-libCZI-1.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/msys-libCZI-1.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip.exe" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/msys-libCZI-1.dll")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libCZI" TYPE FILE FILES
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/ImportExport.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Compositor.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_DimCoordinate.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_exceptions.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Helpers.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata2.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Pixels.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_ReadWrite.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Site.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Utilities.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Write.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_compress.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_StreamsLib.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/Src/libCZI/liblibCZIStatic.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libCZI" TYPE FILE FILES
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/ImportExport.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Compositor.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_DimCoordinate.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_exceptions.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Helpers.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata2.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Pixels.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_ReadWrite.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Site.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Utilities.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Write.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_compress.h"
    "/d/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_StreamsLib.h"
    )
endif()

