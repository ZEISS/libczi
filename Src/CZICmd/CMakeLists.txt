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
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        v2.2.0
)

FetchContent_MakeAvailable(cli11)

set (CZICMDSRCFILES
         BitmapGen.cpp          
         DisplaySettingsHelper.h  
         getOpt.cpp           
         targetver.h
         BitmapGenFreeType.cpp  
         cmdlineoptions.cpp  
         execute.cpp              
         getOpt.h             
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
         executeReadWriteCzi.cpp  
         inc_rapidjson.h      
         BitmapGenNull.h        
         CZIcmd.cpp          
         executeReadWriteCzi.h    
         platform_defines.h)
file(GLOB_RECURSE CZICMDRAPIDJSONSRCFILES "*.h" "*.cpp" "*.hpp")

add_executable(CZIcmd ${CZICMDSRCFILES} ${CZICMDRAPIDJSONSRCFILES})

set_target_properties(CZIcmd PROPERTIES CXX_STANDARD 11)
target_compile_definitions(CZIcmd PRIVATE _LIBCZISTATICLIB)

target_link_libraries(CZIcmd PRIVATE libCZIStatic ${ZLIB_LIBRARIES}  ${PNG_LIBRARIES}  CLI11::CLI11)
target_include_directories(CZIcmd PRIVATE ${ZLIB_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

IF (FREETYPE_FOUND)
    target_compile_definitions(CZIcmd PRIVATE _USE_FREETYPE)
    target_include_directories(CZIcmd PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(CZIcmd PRIVATE libCZIStatic ${FREETYPE_LIBRARIES})
ENDIF()

IF(UNIX)
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