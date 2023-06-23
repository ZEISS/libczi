# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-src"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-build"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/tmp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/src/zstd-populate-stamp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/src"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/src/zstd-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/src/zstd-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/_deps/zstd-subbuild/zstd-populate-prefix/src/zstd-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
