# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext-build"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/tmp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext-stamp"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src"
  "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/vendor/eigen3/src/eigen_ext-stamp${cfgdir}") # cfgdir has leading slash
endif()
