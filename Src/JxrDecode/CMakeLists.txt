# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# build "JxrDecode" as an "object library" (cf. https://cmake.org/cmake/help/v3.0/command/add_library.html) -> we want it to be merged into the
#   static libCIZ-library
add_library(JxrDecodeStatic OBJECT 
							JxrDecode.cpp 
							stdafx.cpp 
							JxrDecode.h 
							stdafx.h 
							targetver.h 
							Jxr/adapthuff.c 
							Jxr/decode.c 
							Jxr/encode.c 
							Jxr/image.c 
							Jxr/JXRGlue.c 
							Jxr/JXRGlueJxr.c 
							Jxr/JXRGluePFC.c 
							Jxr/JXRMeta.c 
							Jxr/JXRTest.c 
							Jxr/JXRTestBmp.c 
							Jxr/JXRTestHdr.c 
							Jxr/JXRTestPnm.c 
							Jxr/JXRTestTif.c 
							Jxr/JXRTestWrapper.c 
							Jxr/JXRTestYUV.c 
							Jxr/JXRTranscode.c 
							Jxr/perfTimerANSI.c 
							Jxr/postprocess.c 
							Jxr/segdec.c 
							Jxr/segenc.c 
							Jxr/strcodec.c 
							Jxr/strdec.c 
							Jxr/strdec_x86.c 
							Jxr/strenc.c 
							Jxr/strenc_x86.c 
							Jxr/strFwdTransform.c 
							Jxr/strInvTransform.c 
							Jxr/strPredQuant.c 
							Jxr/strPredQuantDec.c 
							Jxr/strPredQuantEnc.c 
							Jxr/strTransform.c 
							Jxr/common.h 
							Jxr/decode.h 
							Jxr/encode.h 
							Jxr/JXRGlue.h 
							Jxr/JXRMeta.h 
							Jxr/JXRTest.h 
							Jxr/JXRTestWrapper.h 
							Jxr/perfTimer.h 
							Jxr/strcodec.h 
							Jxr/strTransform.h 
							Jxr/windowsmediaphoto.h 
							Jxr/_x86/_x86.h 
							Jxr/priv_guiddef.h 
							Jxr/wmsal.h 
							Jxr/wmspecstring.h 
							Jxr/wmspecstrings_adt.h 
							Jxr/wmspecstrings_strict.h 
							Jxr/wmspecstrings_undef.h 
							Jxr/jxr_defines.h)

set_target_properties(JxrDecodeStatic PROPERTIES CXX_STANDARD 11)

# shared libraries need PIC
set_property(TARGET JxrDecodeStatic PROPERTY POSITION_INDEPENDENT_CODE ON)

if (IS_BIG_ENDIAN)
 target_compile_definitions(JxrDecodeStatic PRIVATE _BIG__ENDIAN_)
endif(IS_BIG_ENDIAN)

if (CRASH_ON_UNALIGNED_ACCESS)
	target_compile_definitions(JxrDecodeStatic PRIVATE _NO_UNALIGNED_LOAD_)
endif()