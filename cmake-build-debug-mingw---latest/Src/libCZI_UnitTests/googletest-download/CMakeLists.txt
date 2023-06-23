# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 2.8.2)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           release-1.12.1
  SOURCE_DIR        "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-src"
  BINARY_DIR        "D:/dev/Github/libczi-zeiss-ptahmose/cmake-build-debug-mingw---latest/Src/libCZI_UnitTests/googletest-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
