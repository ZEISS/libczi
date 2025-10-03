# SPDX-FileCopyrightText: 2017-2023 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# build "JxrDecode" as an "object library" (cf. https://cmake.org/cmake/help/v3.0/command/add_library.html) -> we want it to be merged into the
#   static libCIZ-library

include(CheckSymbolExists)
include(CheckCSourceCompiles)



add_library(JxrDecodeStatic OBJECT 
							"JxrDecode.h"
							"JxrDecode.cpp"

							"jxrlib/common/include/guiddef.h" 
							"jxrlib/common/include/log.h"
							"jxrlib/common/include/log.h"
							"jxrlib/common/src/log.c"

							"jxrlib/image/decode/decode.c" 
							"jxrlib/image/decode/decode.h" 
							"jxrlib/image/decode/segdec.c" 
							"jxrlib/image/decode/strdec.c" 
							"jxrlib/image/decode/strInvTransform.c"
							"jxrlib/image/decode/strPredQuantDec.c"
							"jxrlib/image/decode/postprocess.c"
							"jxrlib/image/decode/JXRTranscode.c"

							"jxrlib/image/encode/strenc.c" 
							"jxrlib/image/encode/encode.h" 
							"jxrlib/image/encode/encode.c"
							"jxrlib/image/encode/segenc.c"
							"jxrlib/image/encode/strFwdTransform.c" 
							"jxrlib/image/encode/strPredQuantEnc.c"

							"jxrlib/image/sys/strcodec.h" 
							"jxrlib/image/sys/strcodec.c"
							"jxrlib/image/sys/windowsmediaphoto.h" 
							"jxrlib/image/sys/common.h" 
							"jxrlib/image/sys/strTransform.h"
							"jxrlib/image/sys/strTransform.c"
							"jxrlib/image/sys/image.c"
							"jxrlib/image/sys/adapthuff.c"
							"jxrlib/image/sys/strPredQuant.c"
							"jxrlib/image/sys/ansi.h"
							
							"jxrlib/jxrgluelib/JXRGlue.h"
							"jxrlib/jxrgluelib/JXRGlue.c" 
							"jxrlib/jxrgluelib/JXRGlueJxr.c"
							"jxrlib/jxrgluelib/JXRMeta.h"
							"jxrlib/jxrgluelib/JXRMeta.c"         
							"jxrlib/jxrgluelib/JXRGluePFC.c")


# prepare the configuration-file "JxrDecode_Config.h"
if (IS_BIG_ENDIAN)
 set(jxrdecode_ISBIGENDIANHOST 1)
else()
 set(jxrdecode_ISBIGENDIANHOST 0)
endif()
if (CRASH_ON_UNALIGNED_ACCESS)
  set (jxrdecode_CrashOnUnalignedIntegers 1)
else()
  set (jxrdecode_CrashOnUnalignedIntegers 0)
endif()

if (LIBCZI_BUILDCONFIG_HAS_BUILTIN_BSWAP32)
	set (jxrdecode_Has_builtin_bswap32 1)
else()	
	set (jxrdecode_Has_builtin_bswap32 0)	
endif()
if (LIBCZI_BUILDCONFIG_HAS_BYTESWAP_IN_STDLIB_BSWAP32)
	set (jxrdecode_Has_byteswap_long_in_stdlib 1)
else()	
	set (jxrdecode_Has_byteswap_long_in_stdlib 0)
endif()
if (LIBCZI_BUILDCONFIG_HAS_BSWAP_IN_SYS_ENDIAN)
	set (jxrdecode_Has_bswap_long_in_sys_endian 1)
else()	
	set (jxrdecode_Has_bswap_long_in_sys_endian 0)
endif()

# Here we check for the _rotl() function - c.f. https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c
# With MSVC, it is found in <intrin.h>, with gcc it is expected to be found in <x86intrin.h>.
check_symbol_exists(_rotl "intrin.h" jxrdecode_Has_rotl_with_intrin)
BoolToYesNo(jxrdecode_Has_rotl_with_intrin jxrdecode_result_text)
message("jxrdecode_Has_rotl_with_intrin = ${jxrdecode_result_text}.")
if (NOT jxrdecode_Has_rotl_with_intrin)
	set (jxrdecode_Has_rotl_with_intrin 0)
endif()

if (NOT jxrdecode_Has_rotl_with_intrin )
	check_symbol_exists(_rotl "x86intrin.h" jxrdecode_Has_rotl_with_x86intrin)
	BoolToYesNo(jxrdecode_Has_rotl_with_x86intrin jxrdecode_result_text)
	message("jxrdecode_Has_rotl_with_x86intrin = ${jxrdecode_result_text}.")
endif()
if (NOT jxrdecode_Has_rotl_with_x86intrin)
	set (jxrdecode_Has_rotl_with_x86intrin 0)
endif()

if (NOT jxrdecode_Has_rotl_with_intrin AND NOT jxrdecode_Has_rotl_with_x86intrin)
	# Latest Clang and AppleClang seem to support the builtin function __builtin_rotateleft32()
	check_cxx_source_compiles(
		"#include <stdint.h>
		static uint32_t test(uint32_t x) { return __builtin_rotateleft32(x, 7); }
		int main(){ volatile uint32_t v = test(0x12345678u); (void)v; return 0; }"
		jxrdecode_has_builtin_rotateleft32)
	BoolToYesNo(jxrdecode_has_builtin_rotateleft32 jxrdecode_result_text)
	message("jxrdecode_has_builtin_rotateleft32 = ${jxrdecode_result_text}.")
endif()
if (NOT jxrdecode_has_builtin_rotateleft32)
	set (jxrdecode_has_builtin_rotateleft32 0)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/JxrDecode_Config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/JxrDecode_Config.h"
  ESCAPE_QUOTES @ONLY)
target_include_directories(JxrDecodeStatic PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}") # for JxrDecode_Config.h to be found

set_target_properties(JxrDecodeStatic PROPERTIES CXX_STANDARD 11)

# shared libraries need PIC
set_property(TARGET JxrDecodeStatic PROPERTY POSITION_INDEPENDENT_CODE ON)

