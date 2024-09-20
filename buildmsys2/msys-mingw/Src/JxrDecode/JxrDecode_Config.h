// SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// if the host system is a big-endian system, this is "1", otherwise 0
#define JXRDECODE_ISBIGENDIANHOST 0

// whether the processor can load integers from an unaligned address, if this
//  is 1 it means that we cannot load an integer from an unaligned address (we'd
//  get a bus-error if we try), 0 means that the CPU can load from unaligned addresses
#define JXRDECODE_SIGBUS_ON_UNALIGNEDINTEGERS 0

// whether the intrinsic "__builtin_bswap32" is available (in the header-file byteswap.h)
#define JXRDECODE_HAS_BUILTIN_BSWAP32 0

// whether the function "_byteswap_ulong" is available (in the header-file stdlib.h)
#define JXRDECODE_HAS_BYTESWAP_IN_STDLIB 1

// whether the function "bswap32" is available (in the header-file sys/endian.h)
#define JXRDECODE_HAS_BSWAP_LONG_IN_SYS_ENDIAN 0

// whether the "_rotl" function is available in the respective header file - here in <intrin.h>, which should be the case with MSVC
#define JXRDECODE_HAS_ROTL_WITH_INTRIN_H 1

// whether the "_rotl" function is available in the respective header file - here in <x86intrin.h>, which should be the case with GCC
#define JXRDECODE_HAS_ROTL_WITH_X86INTRIN_H 0
