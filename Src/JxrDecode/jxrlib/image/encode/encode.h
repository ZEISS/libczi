//*@@@+++@@@@******************************************************************
//
// Copyright (C) Microsoft Corp.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*@@@---@@@@******************************************************************

#ifndef WMI_ENCODE_H
#define WMI_ENCODE_H

#include "../../common/include/jxrlib_symbol_mangle.h"
#include "../sys/strcodec.h"

/*************************************************************************
    struct / class definitions
*************************************************************************/

Int JXRLIB_API(EncodeMacroblockDC)(CWMImageStrCodec*, CCodingContext*, Int, Int);
Int JXRLIB_API(EncodeMacroblockLowpass)(CWMImageStrCodec*, CCodingContext*, Int, Int);
Int JXRLIB_API(EncodeMacroblockHighpass)(CWMImageStrCodec*, CCodingContext*, Int, Int);

Int JXRLIB_API(quantizeMacroblock)(CWMImageStrCodec*);
Void JXRLIB_API(transformMacroblock)(CWMImageStrCodec*);
Void JXRLIB_API(predMacroblockEnc)(CWMImageStrCodec*);

Void JXRLIB_API(AdaptLowpassEnc)(CCodingContext* pContext);
Void JXRLIB_API(AdaptHighpassEnc)(CCodingContext* pContext);
Void JXRLIB_API(ResetCodingContextEnc)(CCodingContext* pContext);
Int  JXRLIB_API(AllocateCodingContextEnc)(struct CWMImageStrCodec* pSC, Int iNumContexts, Int iTrimFlexBits);
Void JXRLIB_API(FreeCodingContextEnc)(struct CWMImageStrCodec* pSC);
Void JXRLIB_API(predCBPEnc)(CWMImageStrCodec* pSC, CCodingContext* pContext);

/*************************************************************************
    Forward transform definitions
*************************************************************************/
/** 2-point pre filter for boundaries (only used in 420 UV DC subband) **/
Void JXRLIB_API(strPre2)(PixelI*, PixelI*);

/** 2x2 pre filter (only used in 420 UV DC subband) **/
Void JXRLIB_API(strPre2x2)(PixelI*, PixelI*, PixelI*, PixelI*);

/** 4-point pre filter for boundaries **/
Void JXRLIB_API(strPre4)(PixelI*, PixelI*, PixelI*, PixelI*);

/** data allocation in working buffer (first stage) **/

/** Y, 444 U and V **/
/**  0  1  2  3 **/
/** 32 33 34 35 **/
/** 64 65 66 67 **/
/** 96 97 98 99 **/

/** 420 U and V **/
/**   0   2   4   6 **/
/**  64  66  68  70 **/
/** 128 130 132 134 **/
/** 192 194 196 198 **/

/** 4x4 foward DCT for first stage **/
Void JXRLIB_API(strDCT4x4FirstStage)(PixelI*);
Void JXRLIB_API(strDCT4x4FirstStage420UV)(PixelI*);

Void JXRLIB_API(strDCT4x4Stage1)(PixelI*);

/** 4x4 pre filter for first stage **/
Void JXRLIB_API(strPre4x4FirstStage)(PixelI*);
Void JXRLIB_API(strPre4x4FirstStage420UV)(PixelI*);

Void JXRLIB_API(strPre4x4Stage1Split)(PixelI* p0, PixelI* p1, Int iOffset);
Void JXRLIB_API(strPre4x4Stage1)(PixelI* p, Int iOffset);

/** data allocation in working buffer (second stage)**/

/** Y, 444 U and V **/
/**   0   4   8  12 **/
/** 128 132 136 140 **/
/** 256 260 264 268 **/
/** 384 388 392 396 **/

/** 420 U and V **/
/**   0  8 **/
/** 256 264 **/

/** 4x4 foward DCT for second stage **/
Void JXRLIB_API(strDCT4x4SecondStage)(PixelI*);
Void JXRLIB_API(strNormalizeEnc)(PixelI*, Bool);
Void JXRLIB_API(strDCT2x2dnEnc)(PixelI*, PixelI*, PixelI*, PixelI*);

/** 4x4 pre filter for second stage **/
Void JXRLIB_API(strPre4x4Stage2Split)(PixelI* p0, PixelI* p1);

#endif // ENCODE_H

