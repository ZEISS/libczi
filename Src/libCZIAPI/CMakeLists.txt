# SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

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
    # Since "RapidJSON" is a header-only library, we just have to download it and point to the include directory.
    # Note: when using v1.1.0 of RapidJSON (the latest release) there we problems (with GCC14.2 with msys2), so
    #        we use a later version from the master branch.
    include(FetchContent)
    FetchContent_Declare(
            RapidJSON
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG        24b5e7a8b27f42fa16b96fc70aade9106cf7102f # master as of 2025-03-15
            PREFIX "${CMAKE_BINARY_DIR}/vendor/rapidjson"
    )

    set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(RapidJSON)

    set(RAPIDJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
endif()

set(LIBCZIAPISRCFILES 
    "inc/libCZIApi.h"
    "inc/importexport.h"
    "inc/errorcodes.h"
    "inc/versioninfo_structs.h"
    "inc/inputstream_class_info_struct.h"
    "inc/subblock_info_interop.h"
    "inc/attachment_info_interop.h"
    "src/libCZIApi.cpp"
    "src/sharedptrwrapper.h" 
    "inc/external_input_stream_struct.h"
    "inc/external_output_stream_struct.h"
    "inc/external_stream_error_information_struct.h"
    "inc/reader_open_info_struct.h"
    "inc/ObjectHandles.h" 
    "inc/subblock_statistics_struct.h" 
    "inc/misc_types.h" 
    "inc/bitmap_structs.h" 
    "inc/MetadataAsXml_struct.h"
    "inc/attachment_info_interop.h"
    "inc/add_attachment_info_interop.h"
    "inc/add_subblock_info_interop.h"
    "inc/write_metadata_info_interop.h"
    "inc/accessor_options_interop.h"
    "inc/composition_channel_info_interop.h"
    "inc/scaling_info_interop.h"
    "src/parameterhelpers.h"
    "src/parameterhelpers.cpp"
)

add_library(libCZIAPI SHARED ${LIBCZIAPISRCFILES} )

add_library(libCZIAPIStatic STATIC ${LIBCZIAPISRCFILES})

set_target_properties(libCZIAPI PROPERTIES CXX_STANDARD 17)
set_target_properties(libCZIAPIStatic PROPERTIES CXX_STANDARD 17)

target_compile_definitions(libCZIAPI PRIVATE _LIBCZISTATICLIB LIBCZIAPI_EXPORTS_SHARED)
target_compile_definitions(libCZIAPIStatic PRIVATE _LIBCZISTATICLIB _LIBCZIAPISTATICLIB)

target_link_libraries(libCZIAPI PRIVATE libCZIStatic)
target_link_libraries(libCZIAPIStatic PRIVATE libCZIStatic)

target_include_directories(libCZIAPI PRIVATE ../libCZI ${RAPIDJSON_INCLUDE_DIRS})
target_include_directories(libCZIAPIStatic PRIVATE ../libCZI ${RAPIDJSON_INCLUDE_DIRS})

# Copy the resulting DLL to custom folders after the build 
# TODO(JBL): This is a convenient way to copy the DLL to the output folder of the .NET project. It has to be seen
#            how we deal with this towards production/publication.

# Function to handle post-build copying
function(copy_to_runtime TARGET_NAME  TARGET_DIR)
    #set(RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Bindings/dotnet/LibCZI_Net/runtimes/${DIR_SUFFIX}/native")
    set(RUNTIME_DIR "${TARGET_DIR}")
    message(STATUS "Copying DLL to output folder: ${RUNTIME_DIR}")
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libCZIAPI> "${RUNTIME_DIR}/$<TARGET_FILE_NAME:${TARGET_NAME}>"
        COMMENT "Copying DLL to ${RUNTIME_DIR}"
    )
endfunction()

if (LIBCZI_DESTINATION_FOLDER_LIBCZIAPI)
  message(STATUS "'libCZIAPI' will be copied to ${LIBCZI_DESTINATION_FOLDER_LIBCZIAPI}")
  copy_to_runtime(libCZIAPI "${LIBCZI_DESTINATION_FOLDER_LIBCZIAPI}")
endif()

# Debugging messages
#message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
#message(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
##message(STATUS "SYSTEM_PROCESSOR_UPPER: ${CMAKE_SYSTEM_PROCESSOR}")

#[[
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Normalize system processor name
    string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_UPPER)

    if ((DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR
        (NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "") AND SYSTEM_PROCESSOR_UPPER MATCHES "AMD64|X86_64")
        message(STATUS "Building for Windows x64")
        copy_to_runtime(libCZIAPI "Windows x64" "win-x64")

    elseif ((DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM STREQUAL "x86") OR
            (NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "") AND SYSTEM_PROCESSOR_UPPER MATCHES "X86")
        message(STATUS "Building for Windows x86")
        copy_to_runtime(libCZIAPI "Windows x86" "win-x86")

    elseif ((DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64") OR
            (NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "") AND SYSTEM_PROCESSOR_UPPER MATCHES "ARM64")
        message(STATUS "Building for Windows ARM64")
        copy_to_runtime(libCZIAPI "Windows ARM64" "win-arm64")
    else()
        message(WARNING "Unknown architecture for Windows")
    endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_UPPER)
    if ((DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR
            (NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "") AND SYSTEM_PROCESSOR_UPPER MATCHES "AMD64|X86_64")
        message(STATUS "Building for Linux x64")
        copy_to_runtime(libCZIAPI "Linux x64" "linux-x64")
    elseif ((DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64|aarch64") OR
            (NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "") AND SYSTEM_PROCESSOR_UPPER MATCHES "ARM64|AARCH64")
        message(STATUS "Building for Linux ARM64")
        copy_to_runtime(libCZIAPI "Linux ARM64" "linux-arm64")
    else()
        message(WARNING "Unknown architecture for Linux")
    endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_LOWER)

    if (SYSTEM_PROCESSOR_LOWER STREQUAL "arm64")
        message(STATUS "Building for macOS (Apple Silicon)")
        copy_to_runtime(libCZIAPI "macOS ARM64" "osx-arm64")
    else()
        message(STATUS "Building for macOS (Intel)")
        copy_to_runtime(libCZIAPI "macOS Intel" "osx-x64")
    endif()
else()
    message(WARNING "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()

#]]

#get_target_property(LIBRARIES_NEEDED libCZIAPIStatic INTERFACE_LINK_LIBRARIES)

#if (LIBRARIES_NEEDED)
#    message(STATUS "Libraries needed by libCZIAPIStatic: ${LIBRARIES_NEEDED}")
#else()
#    message(STATUS "No libraries are linked to libCZIAPIStatic.")
#endif()

#add_custom_target(print_dependencies
#    COMMAND ${CMAKE_COMMAND} -E echo "Dependencies for libCZIAPIStatic: ${LIBRARIES_NEEDED}"
#)

#add_custom_target(print_resolved_dependencies
#    COMMAND ${CMAKE_COMMAND} -E echo "Resolved dependencies for libCZIAPIStatic: $<TARGET_PROPERTY:libCZIAPIStatic,LINK_LIBRARIES>"
#)

#get_target_property(LINK_LIBRARIES_NEEDED libCZIAPIStatic LINK_LIBRARIES)
#
#add_custom_target(print_resolved_dependencies ALL
#    COMMAND ${CMAKE_COMMAND} -E echo "Resolved dependencies for libCZIAPIStatic: ${LINK_LIBRARIES_NEEDED}"
#)

#get_target_property(TRANSITIVE_DEPENDENCIES libCZIAPIStatic INTERFACE_LINK_LIBRARIES)

#add_custom_target(print_resolved_dependencies ALL
#    COMMAND ${CMAKE_COMMAND} -E echo "Transitive dependencies for libCZIAPIStatic: ${TRANSITIVE_DEPENDENCIES}"
#)