# SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

##
# detect_win32_api_mode(<out_var_win32> <out_var_uwp>)
#
# Detects whether the current platform supports the classic Win32 API
# or is targeting Universal Windows Platform (UWP). This function is designed
# to be safe in cross-compilation environments and does not run any test executables.
#
# Motivation:
# - Simply checking `WIN32` is not sufficient, as it is also defined for UWP.
# - Cross-compilation scenarios (e.g., arm64-windows) cannot run executables,
#   so `try_run()` or runtime detection is not an option.
# - UWP builds do define `WIN32`, but are sandboxed and do not support many
#   traditional Win32 APIs (e.g., `CreateFileW`, registry, console APIs).
#
# How it works:
# - First checks if the target platform is some form of Windows
#   (including MinGW, MSYS, Cygwin, or cross-compiling for Windows).
# - Then uses `check_cxx_source_compiles()` to compile a snippet that tests
#   for `WINAPI_FAMILY == WINAPI_FAMILY_APP` which is true for UWP targets.
# - Outputs two mutually exclusive cache variables:
#     <out_var_win32>: set to ON if classic Win32 APIs are available
#     <out_var_uwp>:   set to ON if building for UWP
#   Both are OFF on non-Windows platforms.
#
# Example usage:
#   detect_win32_api_mode(LIBCZI_HAVE_WIN32_API LIBCZI_HAVE_WIN32UWP_API)
#
function(detect_win32_api_mode out_win32 out_uwp)
    include(CheckCXXSourceCompiles)

    # Assume no Win32/UWP support by default
    set(${out_win32} OFF PARENT_SCOPE)
    set(${out_uwp} OFF PARENT_SCOPE)

    # Check if this is some variant of Windows
    if (WIN32 OR CYGWIN OR MSYS OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Distinguish UWP from classic Win32
        check_cxx_source_compiles("
            #include <winapifamily.h>
            int main() {
            #if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
                return 0;
            #else
                #error Not UWP
            #endif
            }
        " IS_UWP)

        if (IS_UWP)
            message(STATUS "Detected UWP target.")
            set(${out_uwp} ON PARENT_SCOPE)
        else()
            message(STATUS "Detected classic Win32 API target.")
            set(${out_win32} ON PARENT_SCOPE)
        endif()
    else()
        message(STATUS "Non-Windows platform - Win32 API not available.")
    endif()
endfunction()
