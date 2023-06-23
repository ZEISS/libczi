# Install script for directory: D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/libCZI")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "D:/SW/mingw64/bin/objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI/liblibCZId.dll.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI/liblibCZId.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/liblibCZId.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/liblibCZId.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "D:/SW/mingw64/bin/strip.exe" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/liblibCZId.dll")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libCZI" TYPE FILE FILES
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/ImportExport.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Compositor.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_DimCoordinate.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_exceptions.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Helpers.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata2.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Pixels.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_ReadWrite.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Site.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Utilities.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Write.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_compress.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/priv_guiddef.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI/liblibCZIStaticd.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libCZI" TYPE FILE FILES
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/ImportExport.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Compositor.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_DimCoordinate.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_exceptions.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Helpers.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Metadata2.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Pixels.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_ReadWrite.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Site.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Utilities.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_Write.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/libCZI_compress.h"
    "D:/dev/Github/libczi-zeiss-ptahmose/Src/libCZI/priv_guiddef.h"
    )
endif()

