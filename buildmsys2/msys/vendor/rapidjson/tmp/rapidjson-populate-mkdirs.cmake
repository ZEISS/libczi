# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/rapidjson-src")
  file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/rapidjson-src")
endif()
file(MAKE_DIRECTORY
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/_deps/rapidjson-build"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/tmp"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/src/rapidjson-populate-stamp"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/src"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/src/rapidjson-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/src/rapidjson-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/rapidjson/src/rapidjson-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
