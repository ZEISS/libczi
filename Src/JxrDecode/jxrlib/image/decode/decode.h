//*@@@+++@@@@******************************************************************
//
// Copyright (C)) Microsoft Corp.
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

#ifndef WMI_DECODE_H
#define WMI_DECODE_H

#include "../../common/include/jxrlib_symbol_mangle.h"

typedef struct CWMDecoderParameters {
    /** ROI decode **/
    Bool bDecodeFullFrame;
    Bool bDecodeFullWidth;

    /** thumbnail decode **/
    Bool bSkipFlexbits;
    size_t cThumbnailScale;    // 1: cThumbnailScale thumbnail, only supports cThumbnailScale = 2^m for now
    Bool bDecodeHP;
    Bool bDecodeLP;

    // Region of interest decoding
    size_t cROILeftX;
    size_t cROIRightX;
    size_t cROITopY;
    size_t cROIBottomY;

    // table lookups for rotation and flip
    size_t* pOffsetX;
    size_t* pOffsetY;
} CWMDecoderParameters;

Void JXRLIB_API(predCBPDec)(CWMImageStrCodec*, CCodingContext*);
Void JXRLIB_API(predDCACDec)(CWMImageStrCodec*);
Void JXRLIB_API(predACDec)(CWMImageStrCodec*);

Int JXRLIB_API(dequantizeMacroblock)(CWMImageStrCodec*);
Int JXRLIB_API(invTransformMacroblock)(CWMImageStrCodec* pSC);
Int JXRLIB_API(invTransformMacroblock_alteredOperators_hard)(CWMImageStrCodec* pSC);

Int JXRLIB_API(DecodeMacroblockDC)(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY);
Int JXRLIB_API(DecodeMacroblockLowpass)(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY);
Int JXRLIB_API(DecodeMacroblockHighpass)(CWMImageStrCodec* pSC, CCodingContext* pContext, Int iMBX, Int iMBY);

Int JXRLIB_API(AdaptLowpassDec)(struct CCodingContext*);
Int JXRLIB_API(AdaptHighpassDec)(struct CCodingContext*);

Void JXRLIB_API(ResetCodingContextDec)(CCodingContext* pContext);
Void JXRLIB_API(FreeCodingContextDec)(struct CWMImageStrCodec* pSC);

/*************************************************************************/
// Inverse transform functions
// 2-point post filter for boundaries (only used in 420 UV DC subband)
Void JXRLIB_API(strPost2)(PixelI*, PixelI*);

// 2x2 post filter (only used in 420 UV DC subband)
Void JXRLIB_API(strPost2x2)(PixelI*, PixelI*, PixelI*, PixelI*);

/** 4-point post filter for boundaries **/
Void JXRLIB_API(strPost4)(PixelI*, PixelI*, PixelI*, PixelI*);

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

/** 4x4 inverse DCT for first stage **/
Void JXRLIB_API(strIDCT4x4FirstStage)(PixelI*);
Void JXRLIB_API(strIDCT4x4Stage1)(PixelI*);
Void JXRLIB_API(strIDCT4x4FirstStage420UV)(PixelI*);

/** 4x4 post filter for first stage **/
Void JXRLIB_API(strPost4x4FirstStage)(PixelI*);
Void JXRLIB_API(strPost4x4Stage1Split)(PixelI*, PixelI*, Int, Int, Bool);
Void JXRLIB_API(strPost4x4Stage1)(PixelI*, Int, Int, Bool);
Void JXRLIB_API(strPost4x4Stage1Split_alternate)(PixelI*, PixelI*, Int);
Void JXRLIB_API(strPost4x4Stage1_alternate)(PixelI*, Int);
//Void strPost4x4Stage1Split_420(PixelI*, PixelI*);
//Void strPost4x4Stage1_420(PixelI*);

Void JXRLIB_API(strPost4x4FirstStage420UV)(PixelI*);

/** data allocation in working buffer (second stage)**/

/** Y, 444 U and V **/
/**   0   4   8  12 **/
/** 128 132 136 140 **/
/** 256 260 264 268 **/
/** 384 388 392 396 **/

/** 420 U and V **/
/**   0   8 **/
/** 256 264 **/

/** 4x4 invesr DCT for second stage **/
//Void strIDCT4x4SecondStage(PixelI *);
Void JXRLIB_API(strIDCT4x4Stage2)(PixelI*);
Void JXRLIB_API(strNormalizeDec)(PixelI*, Bool);
Void JXRLIB_API(strDCT2x2dnDec)(PixelI*, PixelI*, PixelI*, PixelI*);

/** 4x4 post filter for second stage **/
Void JXRLIB_API(strPost4x4SecondStage)(PixelI*);
Void JXRLIB_API(strPost4x4Stage2Split)(PixelI*, PixelI*);
Void JXRLIB_API(strPost4x4Stage2Split_alternate)(PixelI*, PixelI*);

/** Huffman decode related defines **/
#define HUFFMAN_DECODE_ROOT_BITS_LOG    3
#define HUFFMAN_DECODE_ROOT_BITS    (5)   

Int JXRLIB_API(getHuff)(const short* pDecodeTable, BitIOInfo* pIO);

#endif // WMI_DECODE_H

