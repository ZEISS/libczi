

include ("${CMAKE_CURRENT_LIST_DIR}/RapidJSON-targets.cmake")

################################################################################
# RapidJSON source dir
set( RapidJSON_SOURCE_DIR "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/rapidjson-src")

################################################################################
# RapidJSON build dir
set( RapidJSON_DIR "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/rapidjson-build")

################################################################################
# Compute paths
get_filename_component(RapidJSON_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

get_target_property(RapidJSON_INCLUDE_DIR RapidJSON INTERFACE_INCLUDE_DIRECTORIES)

set( RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR} )
