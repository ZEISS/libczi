# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-src"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-build"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/tmp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/src/googletest-stamp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/src"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/src/googletest-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/src/googletest-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-download/googletest-prefix/src/googletest-stamp${cfgdir}") # cfgdir has leading slash
endif()
