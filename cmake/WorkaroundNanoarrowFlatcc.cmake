# SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

function(libczi_workaround_nanoarrow_flatcc)
  # nanoarrow 0.9.0 exports a bare flatccrt dependency without exporting its
  # imported target. vcpkg patches this on Windows, but not on Linux:
  # https://github.com/apache/arrow-nanoarrow/issues/922
  # Check the exported dependency rather than an SDK version so that fixed
  # packages and packages without IPC do not need this workaround.
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR TARGET flatccrt)
    return()
  endif()

  set(_needs_flatccrt FALSE)
  foreach(_target nanoarrow::nanoarrow_ipc nanoarrow::nanoarrow_ipc_static)
    if(TARGET ${_target})
      get_target_property(_links ${_target} INTERFACE_LINK_LIBRARIES)
      if("flatccrt" IN_LIST _links OR "$<LINK_ONLY:flatccrt>" IN_LIST _links)
        set(_needs_flatccrt TRUE)
      endif()
    endif()
  endforeach()
  if(NOT _needs_flatccrt OR NOT nanoarrow_DIR)
    return()
  endif()

  # Use the same installation as nanoarrow, not an unrelated system library.
  # vcpkg installs the config under <prefix>/share/nanoarrow. Restrict this
  # workaround to the static runtime layout verified for that package.
  get_filename_component(_prefix "${nanoarrow_DIR}/../.." ABSOLUTE)
  find_library(LIBCZI_FLATCCRT_LIBRARY_RELEASE NAMES libflatccrt.a
    PATHS "${_prefix}/lib" NO_DEFAULT_PATH)
  if(NOT LIBCZI_FLATCCRT_LIBRARY_RELEASE)
    return()
  endif()
  find_library(LIBCZI_FLATCCRT_LIBRARY_DEBUG NAMES libflatccrt_d.a libflatccrt.a
    PATHS "${_prefix}/debug/lib" NO_DEFAULT_PATH)
  mark_as_advanced(LIBCZI_FLATCCRT_LIBRARY_RELEASE LIBCZI_FLATCCRT_LIBRARY_DEBUG)

  add_library(flatccrt STATIC IMPORTED)
  set_target_properties(flatccrt PROPERTIES
    IMPORTED_LOCATION "${LIBCZI_FLATCCRT_LIBRARY_RELEASE}"
    IMPORTED_CONFIGURATIONS Release
    IMPORTED_LOCATION_RELEASE "${LIBCZI_FLATCCRT_LIBRARY_RELEASE}")
  if(LIBCZI_FLATCCRT_LIBRARY_DEBUG)
    set_property(TARGET flatccrt APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
    set_target_properties(flatccrt PROPERTIES
      IMPORTED_LOCATION_DEBUG "${LIBCZI_FLATCCRT_LIBRARY_DEBUG}")
  endif()
endfunction()
