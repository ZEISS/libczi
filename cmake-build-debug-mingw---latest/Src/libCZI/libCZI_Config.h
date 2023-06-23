// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// those numbers define the version of libCZI
#define LIBCZI_VERSION_MAJOR "0"
#define LIBCZI_VERSION_MINOR "48"
#define LIBCZI_VERSION_PATCH "2"
#define LIBCZI_VERSION_TWEAK ""

// if the host system is a big-endian system, this is "1", otherwise 0
#define LIBCZI_ISBIGENDIANHOST 0

// if the include-file "endian.h" is available, then this is "1", otherwise 0
#define LIBCZI_HAVE_ENDIAN_H 0

// whether the processor can load integers from an unaligned address, if this
//  is 1 it means that we cannot load an integer from an unaligned address (we'd
//  get a bus-error if we try), 0 means that the CPU can load from unaligned addresses
#define LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS 0

#define LIBCZI_CXX_COMPILER_IDENTIFICATION "GNU 12.2.0"

// whether the function "aligned_alloc" is available
#define LIBCZI_HAVE_ALIGNED_ALLOC 0

// whether the function "_aligned_malloc" is available
#define LIBCZI_HAVE__ALIGNED_MALLOC 1

// whether we can use pread/pwrite-APIs (for implementing file-stream objects), only relevant if not Win32-environment
#define LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL 0

#define LIBCZI_REPOSITORYREMOTEURL "git@github.com:ptahmose/libczi-zeiss.git"

#define LIBCZI_REPOSITORYBRANCH    "write_colormode_to_displaysettingsxml"

#define LIBCZI_REPOSITORYHASH      "6547ca6f0cca14a76cd17d46e36626caec3af7e5"

// whether the header "immintrin.h" is available and AVX-SIMD intrinsics can be used
#define LIBCZI_HAS_AVXINTRINSICS   0

// whether ARM-Neon-intrinsics can be used
#define LIBCZI_HAS_NEOININTRINSICS 0
