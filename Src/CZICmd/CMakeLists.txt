# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

IF(UNIX)
 find_package(ZLIB QUIET)
 if (NOT ZLIB_FOUND)
    message(FATAL_ERROR [=[
    ZLIB library was not found, which is required for building. Consider installing
    like 'sudo apt-get install zlib1g-dev'.
    ]=])
 endif()

 find_package(PNG QUIET)
 if (NOT PNG_FOUND)
    message(FATAL_ERROR [=[
    PNG library was not found, which is required for building. Consider installing
    like 'sudo apt-get install libpng-dev'.
    ]=])
 endif()

 find_package(Freetype QUIET)
 IF (NOT FREETYPE_FOUND)
    message("** FreeType was not found, CZICmd with the option \"CreateCZI\" will create only empty images.")
	  message("   (consider installing FreeType with e.g. 'sudo apt-get install libfreetype6-dev')")
 ENDIF()
ENDIF(UNIX)

include(FetchContent)

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
    # since "RapidJSON" is a header-only library, we just have to download it and point to the include directory
     FetchContent_Declare(
            RapidJSON
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG        "v1.1.0"
            GIT_SHALLOW    TRUE
            PREFIX "${CMAKE_BINARY_DIR}/vendor/rapidjson"
    )

    if (NOT rapidjson_POPULATED)
      FetchContent_Populate(RapidJSON)
      set(RAPIDJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
    endif()
endif()

# make "CLI11" available
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        v2.3.2
)

if (NOT cli11_POPULATED)
  FetchContent_MakeAvailable(cli11)
endif()

set (CZICMDSRCFILES
         BitmapGen.cpp          
         DisplaySettingsHelper.h  
         targetver.h
         BitmapGenFreeType.cpp  
         cmdlineoptions.cpp  
         execute.cpp              
         SaveBitmap.cpp  
         utils.cpp
         BitmapGenFreeType.h    
         cmdlineoptions.h    
         executeCreateCzi.cpp     
         IBitmapGen.h         
         SaveBitmap.h    
         utils.h
         BitmapGenGdiplus.cpp   
         consoleio.cpp       
         executeCreateCzi.h       
         inc_CZIcmd_Config.h  
         stdafx.cpp
         BitmapGenGdiplus.h     
         consoleio.h         
         execute.h                
         inc_libCZI.h         
         stdafx.h
         BitmapGenNull.cpp      
         inc_rapidjson.h      
         BitmapGenNull.h        
         CZIcmd.cpp          
         platform_defines.h
         executePlaneScan.h
         executePlaneScan.cpp
         executeBase.h
         executeBase.cpp)

add_executable(CZIcmd ${CZICMDSRCFILES})

set_target_properties(CZIcmd PROPERTIES CXX_STANDARD 11)
target_compile_definitions(CZIcmd PRIVATE _LIBCZISTATICLIB)

target_link_libraries(CZIcmd PRIVATE ${ZLIB_LIBRARIES}  ${PNG_LIBRARIES} CLI11::CLI11 libCZIStatic)
target_include_directories(CZIcmd PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${RAPIDJSON_INCLUDE_DIRS})

IF(WIN32)
  target_link_libraries(CZIcmd PRIVATE gdiplus Windowscodecs)
ENDIF()

IF (FREETYPE_FOUND)
    target_compile_definitions(CZIcmd PRIVATE _USE_FREETYPE)
    target_include_directories(CZIcmd PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(CZIcmd PRIVATE ${FREETYPE_LIBRARIES})
ENDIF()

IF(UNIX)
  target_include_directories(CZIcmd PRIVATE ${ZLIB_INCLUDE_DIR} ${PNG_INCLUDE_DIR})
  # seems to be problem with glibc I'd reckon -> https://stackoverflow.com/questions/51584960/stdcall-once-throws-stdsystem-error-unknown-error-1
  target_link_libraries(CZIcmd  PUBLIC pthread)
ENDIF(UNIX)

set(CZICMD_USE_FREETYPE 0)
set(CZICMD_USE_WIC 0)
set(CZICMD_USE_GDIPLUS 0)
set(CZICMD_USE_LIBPNG 0)

IF (FREETYPE_FOUND)
	set(CZICMD_USE_FREETYPE 1)
	set(CZICMD_FREETYPE_VERSION_STRING "${FREETYPE_VERSION_STRING}")
ENDIF()

IF(PNG_FOUND)
	set(CZICMD_USE_LIBPNG 1)
	set(CZICMD_LIBPNG_VERSION_STRING "${PNG_VERSION_STRING}")
ENDIF()

IF(WIN32)
	set(CZICMD_USE_WIC 1)
	set(CZICMD_USE_GDIPLUS 1)
ENDIF()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/CZIcmd_Config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CZIcmd_Config.h"
  )


# checking platform -> https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Checking-Platform
#                   -> https://stackoverflow.com/questions/9160335/os-specific-instructions-in-cmake-how-to

install(TARGETS CZIcmd RUNTIME  DESTINATION bin)
