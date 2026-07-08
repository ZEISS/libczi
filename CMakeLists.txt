cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW) # enable new "MSVC runtime library selection" (https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html)

project(libCZI 
      VERSION 0.69.0
      HOMEPAGE_URL "https://github.com/ZEISS/libczi"
      DESCRIPTION "libCZI is an Open Source Cross-Platform C++ library to read and write CZI")

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

# choose the appropriate MSVC-runtime
option(LIBCZI_DO_NOT_SET_MSVC_RUNTIME_LIBRARY "In case of building with VisualStudio, do NOT configure to link the static C-runtime" OFF)

# By default, the binaries built here will link to the static MSVC-runtime.
# However, if the option LIBCZI_DO_NOT_SET_MSVC_RUNTIME_LIBRARY is given, we will not instruct CMake to choose
#  the static runtime and it will default to the dynamic runtime (c.f. https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html)
#  (or what is configured by the upstream CMake-project)
if (NOT LIBCZI_DO_NOT_SET_MSVC_RUNTIME_LIBRARY)
 set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(TestLargeFile)
test_large_files(_LARGEFILES)

function(BoolToFoundNotFound var result_text)
  if (${var})
    set( ${result_text} "found" PARENT_SCOPE)
  else()
    set( ${result_text} "not found" PARENT_SCOPE)
  endif()
endfunction()

include(check_unaligned_access)
CHECK_UNALIGNED_ACCESS(CRASH_ON_UNALIGNED_ACCESS)
if (CRASH_ON_UNALIGNED_ACCESS)
  message("Read/write to unaligned addresses is NOT possible.")
else()
  message("Read/write to unaligned addresses is OK.")
endif()

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
 message("identified a big-endian platform")
else()
 message("identified a little-endian platform")
endif()

# for some ancient platforms it is necessary to link against additional libraries when using C++'s 'std::atomic'.
# This test is compiling a test program, which is problematic in cross-compilation scenarios. In order to prevent
# problems in cross-compilation scenarios, the user can set the variable 'ADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC' manually.
include(CheckForAdditionalLibsRequiredForAtomic)
CheckForAdditionalLibsRequiredForAtomic(ADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC)
if (ADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC)
  message(STATUS "It was determined that linking to those additional libraries is necessary for using 'atomic': ${ADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC}.")
endif()

check_include_file_CXX(endian.h HAVE_ENDIAN_H)
BoolToFoundNotFound(HAVE_ENDIAN_H HAVE_ENDIAN_H_TEXT)
if (NOT DEFINED HAVE_ENDIAN_H OR NOT HAVE_ENDIAN_H)
	set(HAVE_ENDIAN_H 0)
endif()
message("check for endian.h -> ${HAVE_ENDIAN_H_TEXT}")

include (CheckCXXSymbolExists)
check_cxx_symbol_exists(aligned_alloc stdlib.h HAVE_ALIGNED_ALLOC)
if (NOT HAVE_ALIGNED_ALLOC)
  check_cxx_symbol_exists(_aligned_malloc stdlib.h HAVE__ALIGNED_MALLOC)
endif()

check_cxx_symbol_exists(aligned_alloc stdlib.h HAVE_ALIGNED_ALLOC)
if (NOT HAVE_ALIGNED_ALLOC)
  check_cxx_symbol_exists(_aligned_malloc stdlib.h HAVE__ALIGNED_MALLOC)
endif()

check_cxx_symbol_exists(open fcntl.h HAVE_FCNTL_H_OPEN)
BoolToFoundNotFound(HAVE_FCNTL_H_OPEN HAVE_FCNTL_H_OPEN_TEXT)
check_cxx_symbol_exists(pread unistd.h HAVE_UNISTD_H_PREAD)
BoolToFoundNotFound(HAVE_UNISTD_H_PREAD HAVE_UNISTD_H_PREAD_TEXT)
check_cxx_symbol_exists(pwrite unistd.h HAVE_UNISTD_H_PWRITE)
BoolToFoundNotFound(HAVE_UNISTD_H_PWRITE HAVE_UNISTD_H_PWRITE_TEXT)
message("check for open -> ${HAVE_FCNTL_H_OPEN_TEXT} ; check for pread -> ${HAVE_UNISTD_H_PREAD_TEXT} ; check for pwrite -> ${HAVE_UNISTD_H_PWRITE_TEXT}")

# Determine whether we are building for the classic Win32-API or for UWP (Universal Windows Platform).
include(detect_win32_api_mode)
detect_win32_api_mode(
    LIBCZI_HAVE_WIN32_API
    LIBCZI_HAVE_WIN32UWP_API
)

# This option controls whether to build the curl-based http-/https-stream object. If this option is
# "ON", the build will fail if the curl-library is not available (either as an external package or
# as a private copy downloaded during the CMake-run).
option(LIBCZI_BUILD_CURL_BASED_STREAM "include curl-based http-/https-stream object" OFF)

# This option controls whether we should prefer an external package of libcurl (this means that it must be
# found with the find_package()-command). If this option is "OFF", we will download a private copy of libcurl
# during the CMake-run (and build and use this one).
option(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL "Prefer a libcurl package present on the system" OFF)

# This option controls whether to build the Azure-SDK-based-reader object. The Azure-SDK must be externally
# available (and accessible to CMake's find_package()-command). If using vcpkg, the packages "azure-storage-blobs-cpp"
# and "azure-identity-cpp" need to be available.
option(LIBCZI_BUILD_AZURESDK_BASED_STREAM "include AzureSDK-based stream object for accessing Azure-Blob-Store" OFF)

# This option allows to exclude the unit-tests from the build. The unit-tests are using the
# Google-Test-framework which is downloaded from GitHub during the CMake-run.
option(LIBCZI_BUILD_UNITTESTS "Build the gTest-based unit-tests" ON)

# Whether to build the test- and sample-application CZICmd.
option(LIBCZI_BUILD_CZICMD "Build application 'CZICmd'." OFF)

# Whether to build dynamic libCZI-library.
option(LIBCZI_BUILD_DYNLIB "Build dynamic library for 'libCZI'." ON)

# Whether the libCZI-library should be installed to the system.
option(LIBCZI_ENABLE_INSTALL "Enable installation of libCZI" OFF)

# Whether to build the libCZIApi-project (C-API-layer for libCZI).
option(LIBCZI_BUILD_LIBCZIAPI "Build the libCZIApi-project (C-bindings for libCZI)." OFF)

# With this option one can choose between "downloading a private copy of Eigen3 during build" or use
#  an existing Eigen3 (on the system). If CMake is unable to find an Eigen3-package (and this option is
#  "ON"), the build will fail.
option(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3 "Prefer an Eigen3-package present on the system" OFF)

# With this option, we try to use an existing zstd-library. Note: this seems to work with vcpkg, but
#  on Debian it seems that the package "libzstd-dev" does not have the required CMake-config-file.
#  See here: https://discourse.cmake.org/t/findzstd/3506
option(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD "Prefer a ZSTD-package present on the system" OFF)

# When this option is ON, we try to use an existing RapidJSON-library. Otherwise, we download a private
#  copy of RapidJSON during the CMake-run. This option is only relevant if building CZICmd.
option(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON "Prefer an RapidJSON-package present on the system" ON)

# Experimental functionality policy.
#
# This cache variable intentionally is not declared with option(), because it has
# three states:
#
#   AUTO - Do not force experimental features globally. Each experimental feature
#          keeps its own default. This is the default and preserves the normal,
#          conservative build behavior.
#
#   ON   - Enable all experimental features by default.
#
#   OFF  - Disable all experimental features by default.
#
# Individual experimental feature options remain available and may still be set
# explicitly by the user. If both this policy variable and an individual feature
# option are specified, the individual feature option wins. This allows package
# managers and advanced users to enable the global policy while still opting out
# of one specific experimental feature if needed.
#
# Example:
#   -DLIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY=ON
#
# Example with explicit override:
#   -DLIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY=ON
#   -DLIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION=OFF
set(LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY "AUTO" CACHE STRING
    "Experimental functionality policy: AUTO, ON, or OFF.")
set_property(CACHE LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY}" _libczi_experimental_functionality_policy)

if (NOT _libczi_experimental_functionality_policy STREQUAL "AUTO" AND
    NOT _libczi_experimental_functionality_policy STREQUAL "ON" AND
    NOT _libczi_experimental_functionality_policy STREQUAL "OFF")
    message(FATAL_ERROR
        "LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY must be one of: AUTO, ON, OFF.")
endif()

# Default for experimental chunked compression.
#
# The default is derived from LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY:
#
#   AUTO -> OFF
#   ON   -> ON
#   OFF  -> OFF
#
# The value is used only as the default for option(). If the user already
# supplied LIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION in the CMake cache or
# via -D, that explicit value is preserved by option().
set(_libczi_default_experimental_chunked_compression OFF)

if (_libczi_experimental_functionality_policy STREQUAL "ON")
    set(_libczi_default_experimental_chunked_compression ON)
elseif (_libczi_experimental_functionality_policy STREQUAL "OFF")
    set(_libczi_default_experimental_chunked_compression OFF)
endif()

option(LIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION "Enable experimental chunked compression support in the build." ${_libczi_default_experimental_chunked_compression})

# Controls how the mandatory LZ4 dependency is resolved when experimental
# chunked compression is enabled. When ON, CMake requires an externally
# provided lz4 package via find_package(lz4 CONFIG REQUIRED). When OFF,
# CMake fetches and builds a private copy of LZ4 using FetchContent.
# This option has no effect when LIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION is OFF.
option(LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LZ4 "Prefer an LZ4-package present on the system" OFF)



if (LIBCZI_BUILD_UNITTESTS)
 include (CTest)
 enable_testing()
endif()

add_subdirectory(Src)

