# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(ExternalProject)

ExternalProject_Add(
  eigen_ext
  PREFIX "${CMAKE_BINARY_DIR}/vendor/eigen3"
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        3147391d946bb4b6c68edd901f2add6ac1f31f8c  # this is "release version 3.4.0", we pin it to that version because as of today there are problems with the Linux-build
  CMAKE_ARGS
    -DEIGEN_BUILD_PKGCONFIG=OFF
    -DBUILD_TESTING=OFF
  CMAKE_CACHE_ARGS
    "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""  # w/o this the build step fails with clang on windows
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON   # redirect output to log-file (so that we have less clutter)
  LOG_CONFIGURE ON  # redirect output to log-file (so that we have less clutter)
)

ExternalProject_Get_Property(eigen_ext source_dir)
set(EIGEN3_INCLUDE_DIR ${source_dir})
