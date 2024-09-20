# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext")
  file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext")
endif()
file(MAKE_DIRECTORY
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-build"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/tmp"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src"
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp${cfgdir}") # cfgdir has leading slash
endif()
