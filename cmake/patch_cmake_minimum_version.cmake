# SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# Patches a CMakeLists.txt to raise its cmake_minimum_required() to VERSION 3.10,
# suppressing the CMake 4.x deprecation warning for third-party FetchContent dependencies.
#
# Usage:
#   cmake -DFILE=<path/to/CMakeLists.txt> -P patch_cmake_minimum_version.cmake

if(NOT DEFINED FILE)
  message(FATAL_ERROR "FILE variable not set. Pass -DFILE=<path/to/CMakeLists.txt>")
endif()

if(NOT EXISTS "${FILE}")
  message(FATAL_ERROR "File not found: ${FILE}")
endif()

file(READ "${FILE}" _content)

# Match both simple (VERSION X.Y.Z) and range (VERSION X.Y...A.B) forms.
string(REGEX REPLACE
  "cmake_minimum_required[ \t]*\\([ \t]*VERSION[ \t]+[0-9]+\\.[0-9]+(\\.[0-9]+)?(\\.\\.\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?)?[ \t]*\\)"
  "cmake_minimum_required(VERSION 3.10)"
  _patched "${_content}")

if(NOT _content STREQUAL _patched)
  file(WRITE "${FILE}" "${_patched}")
  message(STATUS "patch_cmake_minimum_version: updated ${FILE}")
endif()
