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
#include "../common/include/jxrlib_symbol_mangle.h"
#include <stdlib.h>
#include <ctype.h>

#define INITGUID
#include "JXRGlue.h"

//================================================================
const PKIID JXRLIB_API(IID_PKImageScanEncode) = 1;
const PKIID JXRLIB_API(IID_PKImageFrameEncode) = 2;

const PKIID JXRLIB_API(IID_PKImageUnsupported) = 100;
const PKIID JXRLIB_API(IID_PKImageWmpEncode) = 101;

const PKIID JXRLIB_API(IID_PKImageWmpDecode) = 201;

//================================================================
// Misc supporting functions
//================================================================
ERR JXRLIB_API(PKAlloc)(void** ppv, size_t cb)
{
    *ppv = calloc(1, cb);
    return *ppv ? WMP_errSuccess : WMP_errOutOfMemory;
}


ERR JXRLIB_API(PKFree)(void** ppv)
{
    if (ppv)
    {
        free(*ppv);
        *ppv = NULL;
    }

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKAllocAligned)(void** ppv, size_t cb, size_t iAlign)
{
    U8* pOrigPtr;
    U8* pReturnedPtr;
    size_t       iAlignmentCorrection;
    const size_t c_cbBlockSize = cb + sizeof(void*) + iAlign - 1;

    *ppv = NULL;
    pOrigPtr = calloc(1, c_cbBlockSize);
    if (NULL == pOrigPtr)
        return WMP_errOutOfMemory;

    iAlignmentCorrection = iAlign - ((size_t)pOrigPtr % iAlign);
    if (iAlignmentCorrection < sizeof(void*))
        // Alignment correction won't leave us enough space to store pOrigPtr - advance to next block
        iAlignmentCorrection += iAlign;

    assert(iAlignmentCorrection >= sizeof(void*)); // Alignment correction must have space for pOrigPtr
    assert(iAlignmentCorrection + cb <= c_cbBlockSize); // Don't exceed right edge of memory block

    pReturnedPtr = pOrigPtr + iAlignmentCorrection;
    *(void**)(pReturnedPtr - sizeof(void*)) = pOrigPtr;

    assert(0 == ((size_t)pReturnedPtr % iAlign)); // Are we in fact aligned?
    *ppv = pReturnedPtr;
    return WMP_errSuccess;
}

ERR JXRLIB_API(PKFreeAligned)(void** ppv)
{
    if (ppv && *ppv)
    {
        U8** ppOrigPtr = (U8**)((U8*)(*ppv) - sizeof(void*));
        assert(*ppOrigPtr <= (U8*)ppOrigPtr); // Something's wrong if pOrigPtr points forward
        free(*ppOrigPtr);
        *ppv = NULL;
    }
    return WMP_errSuccess;
}



int JXRLIB_API(PKStrnicmp)(const char* s1, const char* s2, size_t c)
{
    for (; tolower(*s1) == tolower(*s2) && *s1 && *s2 && c; ++s1, ++s2, --c);
    return c ? *s1 - *s2 : 0;
}

static const PKPixelInfo pixelInfo[] =
{
    {&JXRLIB_API(GUID_PKPixelFormatDontCare), 1, Y_ONLY, BD_8, 8, PK_pixfmtNul, 0, 0, 0, 0},

    // Gray
    //{&GUID_PKPixelFormat2bppGray, 1, Y_ONLY, BD_8, 2, PK_pixfmtNul},
    //{&GUID_PKPixelFormat4bppGray, 1, Y_ONLY, BD_8, 4, PK_pixfmtNul},

    {&JXRLIB_API(GUID_PKPixelFormatBlackWhite), 1, Y_ONLY, BD_1, 1, PK_pixfmtNul,               1, 1, 1, 1},//BlackIsZero is default for GUID_PKPixelFormatBlackWhite
    {&JXRLIB_API(GUID_PKPixelFormatBlackWhite), 1, Y_ONLY, BD_1, 1, PK_pixfmtNul,               0, 1, 1, 1},//WhiteIsZero
    {&JXRLIB_API(GUID_PKPixelFormat8bppGray), 1, Y_ONLY, BD_8, 8, PK_pixfmtNul,                 1, 1, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat16bppGray), 1, Y_ONLY, BD_16, 16, PK_pixfmtNul,              1, 1, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat16bppGrayFixedPoint), 1, Y_ONLY, BD_16S, 16, PK_pixfmtNul,   1, 1, 16, 2},
    {&JXRLIB_API(GUID_PKPixelFormat16bppGrayHalf), 1, Y_ONLY, BD_16F, 16, PK_pixfmtNul,         1, 1, 16, 3},
    //{&GUID_PKPixelFormat32bppGray, 1, Y_ONLY, BD_32, 32, PK_pixfmtNul,              1, 1, 32, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppGrayFixedPoint), 1, Y_ONLY, BD_32S, 32, PK_pixfmtNul,   1, 1, 32, 2},
    {&JXRLIB_API(GUID_PKPixelFormat32bppGrayFloat), 1, Y_ONLY, BD_32F, 32, PK_pixfmtNul,        1, 1, 32, 3},

    // RGB
    {&JXRLIB_API(GUID_PKPixelFormat24bppRGB), 3, CF_RGB, BD_8, 24, PK_pixfmtNul,                2, 3, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat24bppBGR), 3, CF_RGB, BD_8, 24, PK_pixfmtBGR,                2, 3, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppRGB), 3, CF_RGB, BD_8, 32, PK_pixfmtNul,                2, 3, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppBGR), 3, CF_RGB, BD_8, 32, PK_pixfmtBGR,                2, 3, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat48bppRGB), 3, CF_RGB, BD_16, 48, PK_pixfmtNul,               2, 3, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat48bppRGBFixedPoint), 3, CF_RGB, BD_16S, 48, PK_pixfmtNul,    2, 3, 16, 2},
    {&JXRLIB_API(GUID_PKPixelFormat48bppRGBHalf), 3, CF_RGB, BD_16F, 48, PK_pixfmtNul,          2, 3, 16, 3},
    {&JXRLIB_API(GUID_PKPixelFormat64bppRGBFixedPoint), 3, CF_RGB, BD_16S, 64, PK_pixfmtNul,    2, 3, 16, 2},
    {&JXRLIB_API(GUID_PKPixelFormat64bppRGBHalf), 3, CF_RGB, BD_16F, 64, PK_pixfmtNul,          2, 3, 16, 3},
    //{&GUID_PKPixelFormat96bppRGB, 3, CF_RGB, BD_32, 96, PK_pixfmtNul,               2, 3, 32, 1},
    {&JXRLIB_API(GUID_PKPixelFormat96bppRGBFixedPoint), 3, CF_RGB, BD_32S, 96, PK_pixfmtNul,    2, 3, 32, 2},
    {&JXRLIB_API(GUID_PKPixelFormat96bppRGBFloat), 3, CF_RGB, BD_32F, 96, PK_pixfmtNul,         2, 3, 32, 3},
    {&JXRLIB_API(GUID_PKPixelFormat128bppRGBFixedPoint), 3, CF_RGB, BD_32S, 128, PK_pixfmtNul,  2, 3, 32, 2},
    {&JXRLIB_API(GUID_PKPixelFormat128bppRGBFloat), 3, CF_RGB, BD_32F, 128, PK_pixfmtNul,       2, 3, 32, 3},

    // RGBA
    {&JXRLIB_API(GUID_PKPixelFormat32bppBGRA), 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha | PK_pixfmtBGR,  2, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppRGBA), 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha,                 2, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bppRGBA), 4, CF_RGB, BD_16, 64, PK_pixfmtHasAlpha,                2, 4, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bppRGBAFixedPoint), 4, CF_RGB, BD_16S, 64, PK_pixfmtHasAlpha,     2, 4, 16, 2},
    {&JXRLIB_API(GUID_PKPixelFormat64bppRGBAHalf), 4, CF_RGB, BD_16F, 64, PK_pixfmtHasAlpha,           2, 4, 16, 3},
    //{&GUID_PKPixelFormat128bppRGBA, 4, CF_RGB, BD_32, 128, PK_pixfmtHasAlpha,              2, 4, 32, 1},
    {&JXRLIB_API(GUID_PKPixelFormat128bppRGBAFixedPoint), 4, CF_RGB, BD_32S, 128, PK_pixfmtHasAlpha,   2, 4, 32, 2},
    {&JXRLIB_API(GUID_PKPixelFormat128bppRGBAFloat), 4, CF_RGB, BD_32F, 128, PK_pixfmtHasAlpha,        2, 4, 32, 3},

    // PRGBA
    {&JXRLIB_API(GUID_PKPixelFormat32bppPBGRA), 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha | PK_pixfmtPreMul | PK_pixfmtBGR,   2, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppPRGBA), 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha | PK_pixfmtPreMul,                  2, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bppPRGBA), 4, CF_RGB, BD_16, 64, PK_pixfmtHasAlpha | PK_pixfmtPreMul,                 2, 4, 16, 1},
    //{&GUID_PKPixelFormat64bppPRGBAFixedPoint, 4, CF_RGB, BD_16S, 64, PK_pixfmtHasAlpha,      2, 4, 16, 2},
    //{&GUID_PKPixelFormat64bppPRGBAHalf, 4, CF_RGB, BD_16F, 64, PK_pixfmtHasAlpha,            2, 4, 16, 3},
    //{&GUID_PKPixelFormat128bppPRGBAFixedPoint, 4, CF_RGB, BD_32S, 128, PK_pixfmtHasAlpha,    2, 4, 32, 2},
    {&JXRLIB_API(GUID_PKPixelFormat128bppPRGBAFloat), 4, CF_RGB, BD_32F, 128, PK_pixfmtHasAlpha | PK_pixfmtPreMul,         2, 4, 32, 3},

    // Packed formats
    {&JXRLIB_API(GUID_PKPixelFormat16bppRGB555), 3, CF_RGB,  BD_5, 16, PK_pixfmtNul,      2, 3, 5, 1},
    {&JXRLIB_API(GUID_PKPixelFormat16bppRGB565), 3, CF_RGB, BD_565, 16, PK_pixfmtNul,     2, 3, 6, 1},
    {&JXRLIB_API(GUID_PKPixelFormat32bppRGB101010), 3, CF_RGB, BD_10, 32, PK_pixfmtNul,   2, 3, 10, 1},

    // CMYK
    {&JXRLIB_API(GUID_PKPixelFormat32bppCMYK), 4, CMYK, BD_8, 32, PK_pixfmtNul,               5, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat40bppCMYKAlpha), 5, CMYK, BD_8, 40, PK_pixfmtHasAlpha,     5, 5, 8, 1},

    {&JXRLIB_API(GUID_PKPixelFormat64bppCMYK), 4, CMYK, BD_16, 64, PK_pixfmtNul,              5, 4, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat80bppCMYKAlpha), 5, CMYK, BD_16, 80, PK_pixfmtHasAlpha,    5, 5, 16, 1},

    // N_CHANNEL
    {&JXRLIB_API(GUID_PKPixelFormat24bpp3Channels), 3, NCOMPONENT, BD_8, 24, PK_pixfmtNul, PK_PI_NCH, 3, 8, 1},//the N channel TIF by PS has PhotometricInterpretation of PK_PI_RGB
    {&JXRLIB_API(GUID_PKPixelFormat32bpp4Channels), 4, NCOMPONENT, BD_8, 32, PK_pixfmtNul, PK_PI_NCH, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat40bpp5Channels), 5, NCOMPONENT, BD_8, 40, PK_pixfmtNul, PK_PI_NCH, 5, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat48bpp6Channels), 6, NCOMPONENT, BD_8, 48, PK_pixfmtNul, PK_PI_NCH, 6, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat56bpp7Channels), 7, NCOMPONENT, BD_8, 56, PK_pixfmtNul, PK_PI_NCH, 7, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bpp8Channels), 8, NCOMPONENT, BD_8, 64, PK_pixfmtNul, PK_PI_NCH, 8, 8, 1},

    {&JXRLIB_API(GUID_PKPixelFormat32bpp3ChannelsAlpha), 4, NCOMPONENT, BD_8, 32, PK_pixfmtHasAlpha, PK_PI_NCH, 4, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat40bpp4ChannelsAlpha), 5, NCOMPONENT, BD_8, 40, PK_pixfmtHasAlpha, PK_PI_NCH, 5, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat48bpp5ChannelsAlpha), 6, NCOMPONENT, BD_8, 48, PK_pixfmtHasAlpha, PK_PI_NCH, 6, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat56bpp6ChannelsAlpha), 7, NCOMPONENT, BD_8, 56, PK_pixfmtHasAlpha, PK_PI_NCH, 7, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bpp7ChannelsAlpha), 8, NCOMPONENT, BD_8, 64, PK_pixfmtHasAlpha, PK_PI_NCH, 8, 8, 1},
    {&JXRLIB_API(GUID_PKPixelFormat72bpp8ChannelsAlpha), 9, NCOMPONENT, BD_8, 72, PK_pixfmtHasAlpha, PK_PI_NCH, 9, 8, 1},

    {&JXRLIB_API(GUID_PKPixelFormat48bpp3Channels), 3, NCOMPONENT, BD_16, 48, PK_pixfmtNul, PK_PI_NCH, 3, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat64bpp4Channels), 4, NCOMPONENT, BD_16, 64, PK_pixfmtNul, PK_PI_NCH, 4, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat80bpp5Channels), 5, NCOMPONENT, BD_16, 80, PK_pixfmtNul, PK_PI_NCH, 5, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat96bpp6Channels), 6, NCOMPONENT, BD_16, 96, PK_pixfmtNul, PK_PI_NCH, 6, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat112bpp7Channels), 7, NCOMPONENT, BD_16, 112, PK_pixfmtNul, PK_PI_NCH, 7, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat128bpp8Channels), 8, NCOMPONENT, BD_16, 128, PK_pixfmtNul, PK_PI_NCH, 8, 16, 1},

    {&JXRLIB_API(GUID_PKPixelFormat64bpp3ChannelsAlpha), 4, NCOMPONENT, BD_16, 64, PK_pixfmtHasAlpha, PK_PI_NCH, 4, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat80bpp4ChannelsAlpha), 5, NCOMPONENT, BD_16, 80, PK_pixfmtHasAlpha, PK_PI_NCH, 5, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat96bpp5ChannelsAlpha), 6, NCOMPONENT, BD_16, 96, PK_pixfmtHasAlpha, PK_PI_NCH, 6, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat112bpp6ChannelsAlpha), 7, NCOMPONENT, BD_16, 112, PK_pixfmtHasAlpha, PK_PI_NCH, 7, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat128bpp7ChannelsAlpha), 8, NCOMPONENT, BD_16, 128, PK_pixfmtHasAlpha, PK_PI_NCH, 8, 16, 1},
    {&JXRLIB_API(GUID_PKPixelFormat144bpp8ChannelsAlpha), 9, NCOMPONENT, BD_16, 144, PK_pixfmtHasAlpha, PK_PI_NCH, 9, 16, 1},

    //RGBE
    {&JXRLIB_API(GUID_PKPixelFormat32bppRGBE), 4, CF_RGBE, BD_8, 32, PK_pixfmtNul, PK_PI_RGBE, 4, 8, 1},

    //YUV
    {&GUID_PKPixelFormat12bppYUV420, 3, YUV_420, BD_8, 48, PK_pixfmtNul},
    {&GUID_PKPixelFormat16bppYUV422, 3, YUV_422, BD_8, 32, PK_pixfmtNul},
    {&GUID_PKPixelFormat24bppYUV444, 3, YUV_444, BD_8, 24, PK_pixfmtNul},
};

//----------------------------------------------------------------
//ERR GetPixelInfo(PKPixelFormatGUID enPixelFormat, const PKPixelInfo** ppPI)
ERR JXRLIB_API(PixelFormatLookup)(PKPixelInfo* pPI, U8 uLookupType)
{
    ERR err = WMP_errSuccess;
    size_t i;

    for (i = 0; i < sizeof2(pixelInfo); ++i)
    {
        if (LOOKUP_FORWARD == uLookupType)
        {
            if (IsEqualGUID(pPI->pGUIDPixFmt, pixelInfo[i].pGUIDPixFmt))
            {
                *pPI = pixelInfo[i];
                goto Cleanup;
            }
        }
        else if (LOOKUP_BACKWARD_TIF == uLookupType)
        {
            if (pPI->uSamplePerPixel == pixelInfo[i].uSamplePerPixel &&
                pPI->uBitsPerSample == pixelInfo[i].uBitsPerSample &&
                pPI->uSampleFormat == pixelInfo[i].uSampleFormat &&
                pPI->uInterpretation == pixelInfo[i].uInterpretation)
            {
                // match alpha & premult
                if ((pPI->grBit & (PK_pixfmtHasAlpha | PK_pixfmtPreMul)) ==
                    (pixelInfo[i].grBit & (PK_pixfmtHasAlpha | PK_pixfmtPreMul)))
                {
                    *pPI = pixelInfo[i];
                    goto Cleanup;
                }
            }
        }
    }
    Call(WMP_errUnsupportedFormat);

Cleanup:
    return err;
}


const PKPixelFormatGUID* JXRLIB_API(GetPixelFormatFromHash)(const U8 uPFHash)
{
    int i;

    for (i = 0; i < sizeof2(pixelInfo); i++)
    {
        if (pixelInfo[i].pGUIDPixFmt->Data4[7] == uPFHash)
            return pixelInfo[i].pGUIDPixFmt;
    }

    // If we reached this point, we did not find anything which matched the hash
    return NULL;
}

//----------------------------------------------------------------
typedef struct tagPKIIDInfo
{
    const char* szExt;
    const PKIID* pIIDEnc;
    const PKIID* pIIDDec;
} PKIIDInfo;

static ERR GetIIDInfo(const char* szExt, const PKIIDInfo** ppInfo)
{
    ERR err = WMP_errSuccess;

    static PKIIDInfo iidInfo[] = {
        {".jxr", &JXRLIB_API(IID_PKImageWmpEncode), &JXRLIB_API(IID_PKImageWmpDecode)},
        {".wdp", &JXRLIB_API(IID_PKImageUnsupported), &JXRLIB_API(IID_PKImageWmpDecode)},
        {".hdp", &JXRLIB_API(IID_PKImageUnsupported), &JXRLIB_API(IID_PKImageWmpDecode)},
    };
    size_t i = 0;

    *ppInfo = NULL;
    for (i = 0; i < sizeof2(iidInfo); ++i)
    {
        if (0 == JXRLIB_API(PKStrnicmp)(szExt, iidInfo[i].szExt, strlen(iidInfo[i].szExt)))
        {
            *ppInfo = &iidInfo[i];
            goto Cleanup;
        }
    }

    Call(WMP_errUnsupportedFormat);

Cleanup:
    return err;
}

ERR JXRLIB_API(GetImageEncodeIID)(const char* szExt, const PKIID** ppIID)
{
    ERR err = WMP_errSuccess;

    const PKIIDInfo* pInfo = NULL;

    Call(GetIIDInfo(szExt, &pInfo));
    *ppIID = pInfo->pIIDEnc;

Cleanup:
    return err;
}

ERR JXRLIB_API(GetImageDecodeIID)(const char* szExt, const PKIID** ppIID)
{
    ERR err = WMP_errSuccess;

    const PKIIDInfo* pInfo = NULL;

    Call(GetIIDInfo(szExt, &pInfo));
    *ppIID = pInfo->pIIDDec;

Cleanup:
    return err;
}

//================================================================
// PKFactory
//================================================================
ERR JXRLIB_API(PKCreateFactory_CreateStream)(PKStream** ppStream)
{
    ERR err = WMP_errSuccess;

    Call(JXRLIB_API(PKAlloc)((void**)ppStream, sizeof(**ppStream)));

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCreateFactory_Release)(PKFactory** ppFactory)
{
    ERR err = WMP_errSuccess;

    Call(JXRLIB_API(PKFree)((void**)ppFactory));

Cleanup:
    return err;
}

//----------------------------------------------------------------
ERR JXRLIB_API(PKCreateFactory)(PKFactory** ppFactory, U32 uVersion)
{
    ERR err = WMP_errSuccess;
    PKFactory* pFactory = NULL;

    UNREFERENCED_PARAMETER(uVersion);

    Call(JXRLIB_API(PKAlloc)((void**)ppFactory, sizeof(**ppFactory)));
    pFactory = *ppFactory;

    pFactory->CreateStream = JXRLIB_API(PKCreateFactory_CreateStream);

    pFactory->CreateStreamFromFilename = JXRLIB_API(CreateWS_File);
    pFactory->CreateStreamFromMemory = JXRLIB_API(CreateWS_Memory);

    pFactory->Release = JXRLIB_API(PKCreateFactory_Release);

Cleanup:
    return err;
}


//================================================================
// PKCodecFactory
//================================================================
ERR JXRLIB_API(PKCodecFactory_CreateCodec)(const PKIID* iid, void** ppv)
{
    ERR err = WMP_errSuccess;

    if (JXRLIB_API(IID_PKImageWmpEncode) == *iid)
    {
        Call(JXRLIB_API(PKImageEncode_Create_WMP)((PKImageEncode**)ppv));
    }
    else if (JXRLIB_API(IID_PKImageWmpDecode) == *iid)
    {
        Call(JXRLIB_API(PKImageDecode_Create_WMP)((PKImageDecode**)ppv));
    }
    else
    {
        Call(WMP_errUnsupportedFormat);
    }

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCodecFactory_CreateDecoderFromFile)(const char* szFilename, PKImageDecode** ppDecoder)
{
    ERR err = WMP_errSuccess;

    char* pExt = NULL;
    const PKIID* pIID = NULL;

    struct tagWMPStream* pStream = NULL;
    PKImageDecode* pDecoder = NULL;

    // get file extension
    pExt = strrchr(szFilename, '.');
    FailIf(NULL == pExt, WMP_errUnsupportedFormat);

    // get decode PKIID
    Call(JXRLIB_API(GetImageDecodeIID)(pExt, &pIID));

    // create stream
    Call(JXRLIB_API(CreateWS_File)(&pStream, szFilename, "rb"));

    // Create decoder
    Call(JXRLIB_API(PKCodecFactory_CreateCodec)(pIID, (void**)ppDecoder));
    pDecoder = *ppDecoder;

    // attach stream to decoder
    Call(pDecoder->Initialize(pDecoder, pStream));
    pDecoder->fStreamOwner = !0;

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCodecFactory_CreateDecoderFromMemory)(char* inBuffer, size_t size, PKImageDecode** ppDecoder)
{
    ERR err = WMP_errSuccess;

    char* pExt = ".jxr";
    const PKIID* pIID = NULL;

    struct tagWMPStream* pStream = NULL;
    PKImageDecode* pDecoder = NULL;

    // get decode PKIID
    Call(JXRLIB_API(GetImageDecodeIID)(pExt, &pIID));

    // create stream
    Call(JXRLIB_API(CreateWS_Memory)(&pStream, inBuffer, size));

    // Create decoder
    Call(JXRLIB_API(PKCodecFactory_CreateCodec)(pIID, (void**)ppDecoder));
    pDecoder = *ppDecoder;

    // attach stream to decoder
    Call(pDecoder->Initialize(pDecoder, pStream));
    pDecoder->fStreamOwner = !0;

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCodecFactory_CreateDecoderFromStream)(struct tagWMPStream* pStream, PKImageDecode** ppDecoder)
{
    ERR err = WMP_errSuccess;

    // Create decoder
    Call(JXRLIB_API(PKCodecFactory_CreateCodec)(&JXRLIB_API(IID_PKImageWmpDecode), (void**)ppDecoder));

    // attach stream to decoder
    Call((*ppDecoder)->Initialize(*ppDecoder, pStream));
    (*ppDecoder)->fStreamOwner = 0;

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCodecFactory_CreateFormatConverter)(PKFormatConverter** ppFConverter)
{
    ERR err = WMP_errSuccess;
    PKFormatConverter* pFC = NULL;

    Call(JXRLIB_API(PKAlloc)((void**)ppFConverter, sizeof(**ppFConverter)));
    pFC = *ppFConverter;

    pFC->Initialize = JXRLIB_API(PKFormatConverter_Initialize);
    pFC->InitializeConvert = JXRLIB_API(PKFormatConverter_InitializeConvert);
    pFC->GetPixelFormat = JXRLIB_API(PKFormatConverter_GetPixelFormat);
    pFC->GetSourcePixelFormat = JXRLIB_API(PKFormatConverter_GetSourcePixelFormat);
    pFC->GetSize = JXRLIB_API(PKFormatConverter_GetSize);
    pFC->GetResolution = JXRLIB_API(PKFormatConverter_GetResolution);
    pFC->Copy = JXRLIB_API(PKFormatConverter_Copy);
    pFC->Convert = JXRLIB_API(PKFormatConverter_Convert);
    pFC->Release = JXRLIB_API(PKFormatConverter_Release);

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCreateCodecFactory_Release)(PKCodecFactory** ppCFactory)
{
    ERR err = WMP_errSuccess;

    Call(JXRLIB_API(PKFree)((void**)ppCFactory));

Cleanup:
    return err;
}

ERR JXRLIB_API(PKCreateCodecFactory)(PKCodecFactory** ppCFactory, U32 uVersion)
{
    ERR err = WMP_errSuccess;
    PKCodecFactory* pCFactory = NULL;

    UNREFERENCED_PARAMETER(uVersion);

    Call(JXRLIB_API(PKAlloc)((void**)ppCFactory, sizeof(**ppCFactory)));
    pCFactory = *ppCFactory;

    pCFactory->CreateCodec = JXRLIB_API(PKCodecFactory_CreateCodec);
    pCFactory->CreateDecoderFromFile = JXRLIB_API(PKCodecFactory_CreateDecoderFromFile);
    pCFactory->CreateDecoderFromMemory = JXRLIB_API(PKCodecFactory_CreateDecoderFromMemory);
    pCFactory->CreateDecoderFromStream = JXRLIB_API(PKCodecFactory_CreateDecoderFromStream);
    pCFactory->CreateFormatConverter = JXRLIB_API(PKCodecFactory_CreateFormatConverter);
    pCFactory->Release = JXRLIB_API(PKCreateCodecFactory_Release);

Cleanup:
    return err;
}


//================================================================
// PKImageEncode
//================================================================
ERR JXRLIB_API(PKImageEncode_Initialize)(
    PKImageEncode* pIE,
    struct tagWMPStream* pStream,
    void* pvParam,
    size_t cbParam)
{
    ERR err = WMP_errSuccess;

    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(pvParam);
    UNREFERENCED_PARAMETER(cbParam);

    pIE->pStream = pStream;
    pIE->guidPixFormat = JXRLIB_API(GUID_PKPixelFormatDontCare);
    pIE->fResX = 96;
    pIE->fResY = 96;
    pIE->cFrame = 1;

    Call(pIE->pStream->GetPos(pIE->pStream, &pIE->offStart));

Cleanup:
    return err;
}

ERR JXRLIB_API(PKImageEncode_Terminate)(
    PKImageEncode* pIE)
{
    UNREFERENCED_PARAMETER(pIE);
    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageEncode_SetPixelFormat)(
    PKImageEncode* pIE,
    PKPixelFormatGUID enPixelFormat)
{
    pIE->guidPixFormat = enPixelFormat;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageEncode_SetSize)(
    PKImageEncode* pIE,
    I32 iWidth,
    I32 iHeight)
{
    if (iWidth > 0 && iHeight > 0)
    {
        pIE->uWidth = (U32)iWidth;
        pIE->uHeight = (U32)iHeight;

        return WMP_errSuccess;
    }

    return WMP_errInvalidParameter;
}

ERR JXRLIB_API(PKImageEncode_SetResolution)(
    PKImageEncode* pIE,
    Float fResX,
    Float fResY)
{
    pIE->fResX = fResX;
    pIE->fResY = fResY;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageEncode_SetColorContext)(PKImageEncode* pIE,
    const U8* pbColorContext,
    U32 cbColorContext)
{
    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(pbColorContext);
    UNREFERENCED_PARAMETER(cbColorContext);
    return WMP_errNotYetImplemented;
}


ERR JXRLIB_API(PKImageEncode_SetDescriptiveMetadata)(PKImageEncode* pIE, const DESCRIPTIVEMETADATA* pDescMetadata)
{
    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(pDescMetadata);
    return WMP_errNotYetImplemented;
}

ERR JXRLIB_API(PKImageEncode_WritePixels)(
    PKImageEncode* pIE,
    U32 cLine,
    U8* pbPixels,
    U32 cbStride)
{
    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(cLine);
    UNREFERENCED_PARAMETER(pbPixels);
    UNREFERENCED_PARAMETER(cbStride);
    return WMP_errAbstractMethod;
}

ERR JXRLIB_API(PKImageEncode_WriteSource)(
    PKImageEncode* pIE,
    PKFormatConverter* pFC,
    PKRect* pRect)
{
    ERR err = WMP_errSuccess;

    PKPixelFormatGUID enPFFrom = JXRLIB_API(GUID_PKPixelFormatDontCare);
    PKPixelFormatGUID enPFTo = JXRLIB_API(GUID_PKPixelFormatDontCare);

    PKPixelInfo pPIFrom;
    PKPixelInfo pPITo;

    U32 cbStrideTo = 0;
    U32 cbStrideFrom = 0;
    U32 cbStride = 0;

    U8* pb = NULL;

    // CWMTranscodingParam* pParam = NULL; 

    // get pixel format
    Call(pFC->GetSourcePixelFormat(pFC, &enPFFrom));
    Call(pFC->GetPixelFormat(pFC, &enPFTo));
    FailIf(!IsEqualGUID(&pIE->guidPixFormat, &enPFTo), WMP_errUnsupportedFormat);

    // calc common stride
//    Call(GetPixelInfo(enPFFrom, &pPIFrom));
    pPIFrom.pGUIDPixFmt = &enPFFrom;
    JXRLIB_API(PixelFormatLookup)(&pPIFrom, LOOKUP_FORWARD);

    //    Call(GetPixelInfo(enPFTo, &pPITo));
    pPITo.pGUIDPixFmt = &enPFTo;
    JXRLIB_API(PixelFormatLookup)(&pPITo, LOOKUP_FORWARD);

    //    cbStrideFrom = (pPIFrom->cbPixel * pRect->Width + pPIFrom->cbPixelDenom - 1) / pPIFrom->cbPixelDenom;
    cbStrideFrom = (BD_1 == pPIFrom.bdBitDepth ? ((pPIFrom.cbitUnit * pRect->Width + 7) >> 3) : (((pPIFrom.cbitUnit + 7) >> 3) * pRect->Width));
    if (&GUID_PKPixelFormat12bppYUV420 == pPIFrom.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPIFrom.pGUIDPixFmt)
        cbStrideFrom >>= 1;

    //    cbStrideTo = (pPITo->cbPixel * pIE->uWidth + pPITo->cbPixelDenom - 1) / pPITo->cbPixelDenom;
    cbStrideTo = (BD_1 == pPITo.bdBitDepth ? ((pPITo.cbitUnit * pIE->uWidth + 7) >> 3) : (((pPITo.cbitUnit + 7) >> 3) * pIE->uWidth));
    if (&GUID_PKPixelFormat12bppYUV420 == pPITo.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPITo.pGUIDPixFmt)
        cbStrideTo >>= 1;

    cbStride = cbStrideFrom > cbStrideTo ? cbStrideFrom : cbStrideTo;//  max(cbStrideFrom, cbStrideTo);

    // actual dec/enc with local buffer
    Call(JXRLIB_API(PKAllocAligned)((void**)&pb, (size_t)cbStride * pRect->Height, 128));

    Call(pFC->Copy(pFC, pRect, pb, cbStride));

    Call(pIE->WritePixels(pIE, pRect->Height, pb, cbStride));

Cleanup:
    JXRLIB_API(PKFreeAligned)((void**)&pb);
    return err;
}

ERR JXRLIB_API(PKImageEncode_WritePixelsBandedBegin)(PKImageEncode* pEncoder, struct tagWMPStream* pPATempFile)
{
    UNREFERENCED_PARAMETER(pEncoder);
    UNREFERENCED_PARAMETER(pPATempFile);
    return WMP_errAbstractMethod;
}

ERR JXRLIB_API(PKImageEncode_WritePixelsBanded)(PKImageEncode* pEncoder, U32 cLines, U8* pbPixels, U32 cbStride, Bool fLastCall)
{
    UNREFERENCED_PARAMETER(pEncoder);
    UNREFERENCED_PARAMETER(cLines);
    UNREFERENCED_PARAMETER(pbPixels);
    UNREFERENCED_PARAMETER(cbStride);
    UNREFERENCED_PARAMETER(fLastCall);
    return WMP_errAbstractMethod;
}

ERR JXRLIB_API(PKImageEncode_WritePixelsBandedEnd)(PKImageEncode* pEncoder)
{
    UNREFERENCED_PARAMETER(pEncoder);
    return WMP_errAbstractMethod;
}


ERR JXRLIB_API(PKImageEncode_Transcode)(
    PKImageEncode* pIE,
    PKFormatConverter* pFC,
    PKRect* pRect)
{
    ERR err = WMP_errSuccess;

    PKPixelFormatGUID enPFFrom = JXRLIB_API(GUID_PKPixelFormatDontCare);
    PKPixelFormatGUID enPFTo = JXRLIB_API(GUID_PKPixelFormatDontCare);

    PKPixelInfo pPIFrom;
    PKPixelInfo pPITo;

    U32 cbStrideTo = 0;
    U32 cbStrideFrom = 0;
    U32 cbStride = 0;

    U8* pb = NULL;

    CWMTranscodingParam cParam = { 0 };

    // get pixel format
    Call(pFC->GetSourcePixelFormat(pFC, &enPFFrom));
    Call(pFC->GetPixelFormat(pFC, &enPFTo));
    FailIf(!IsEqualGUID(&pIE->guidPixFormat, &enPFTo), WMP_errUnsupportedFormat);

    // calc common stride
//    Call(GetPixelInfo(enPFFrom, &pPIFrom));
    pPIFrom.pGUIDPixFmt = &enPFFrom;
    JXRLIB_API(PixelFormatLookup)(&pPIFrom, LOOKUP_FORWARD);

    //    Call(GetPixelInfo(enPFTo, &pPITo));
    pPITo.pGUIDPixFmt = &enPFTo;
    JXRLIB_API(PixelFormatLookup)(&pPITo, LOOKUP_FORWARD);

    //    cbStrideFrom = (pPIFrom->cbPixel * pRect->Width + pPIFrom->cbPixelDenom - 1) / pPIFrom->cbPixelDenom;
    cbStrideFrom = (BD_1 == pPIFrom.bdBitDepth ? ((pPIFrom.cbitUnit * pRect->Width + 7) >> 3) : (((pPIFrom.cbitUnit + 7) >> 3) * pRect->Width));
    if (&GUID_PKPixelFormat12bppYUV420 == pPIFrom.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPIFrom.pGUIDPixFmt)
        cbStrideFrom >>= 1;

    //    cbStrideTo = (pPITo->cbPixel * pIE->uWidth + pPITo->cbPixelDenom - 1) / pPITo->cbPixelDenom;
    cbStrideTo = (BD_1 == pPITo.bdBitDepth ? ((pPITo.cbitUnit * pIE->uWidth + 7) >> 3) : (((pPITo.cbitUnit + 7) >> 3) * pIE->uWidth));
    if (&GUID_PKPixelFormat12bppYUV420 == pPITo.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPITo.pGUIDPixFmt)
        cbStrideTo >>= 1;

    cbStride = cbStrideFrom > cbStrideTo ? cbStrideFrom : cbStrideTo;//  max(cbStrideFrom, cbStrideTo);

    if (pIE->bWMP) {
        cParam.cLeftX = pFC->pDecoder->WMP.wmiI.cROILeftX;
        cParam.cTopY = pFC->pDecoder->WMP.wmiI.cROITopY;
        cParam.cWidth = pFC->pDecoder->WMP.wmiI.cROIWidth;
        cParam.cHeight = pFC->pDecoder->WMP.wmiI.cROIHeight;
        cParam.oOrientation = pFC->pDecoder->WMP.wmiI.oOrientation;
        //        cParam.cfColorFormat = pFC->pDecoder->WMP.wmiI.cfColorFormat;
        cParam.uAlphaMode = pFC->pDecoder->WMP.wmiSCP.uAlphaMode;
        cParam.bfBitstreamFormat = pFC->pDecoder->WMP.wmiSCP.bfBitstreamFormat;
        cParam.sbSubband = pFC->pDecoder->WMP.wmiSCP.sbSubband;
        cParam.bIgnoreOverlap = pFC->pDecoder->WMP.bIgnoreOverlap;

        Call(pIE->Transcode(pIE, pFC->pDecoder, &cParam));
    }
    else
    {
        // actual dec/enc with local buffer
        Call(JXRLIB_API(PKAllocAligned)((void**)&pb, (size_t)cbStride * pRect->Height, 128));
        Call(pFC->Copy(pFC, pRect, pb, cbStride));
        Call(pIE->WritePixels(pIE, pRect->Height, pb, cbStride));
    }

Cleanup:
    JXRLIB_API(PKFreeAligned)((void**)&pb);
    return err;
}

ERR JXRLIB_API(PKImageEncode_CreateNewFrame)(
    PKImageEncode* pIE,
    void* pvParam,
    size_t cbParam)
{
    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(pvParam);
    UNREFERENCED_PARAMETER(cbParam);
    // NYI
    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageEncode_Release)(
    PKImageEncode** ppIE)
{
    PKImageEncode* pIE = *ppIE;
    pIE->pStream->Close(&pIE->pStream);

    return JXRLIB_API(PKFree)((void**)ppIE);
}

ERR JXRLIB_API(PKImageEncode_Create)(PKImageEncode** ppIE)
{
    ERR err = WMP_errSuccess;
    PKImageEncode* pIE = NULL;

    Call(JXRLIB_API(PKAlloc)((void**)ppIE, sizeof(**ppIE)));

    pIE = *ppIE;
    pIE->Initialize = JXRLIB_API(PKImageEncode_Initialize);
    pIE->Terminate = JXRLIB_API(PKImageEncode_Terminate);
    pIE->SetPixelFormat = JXRLIB_API(PKImageEncode_SetPixelFormat);
    pIE->SetSize = JXRLIB_API(PKImageEncode_SetSize);
    pIE->SetResolution = JXRLIB_API(PKImageEncode_SetResolution);
    pIE->SetColorContext = JXRLIB_API(PKImageEncode_SetColorContext);
    pIE->SetDescriptiveMetadata = JXRLIB_API(PKImageEncode_SetDescriptiveMetadata);
    pIE->WritePixels = JXRLIB_API(PKImageEncode_WritePixels);
    //    pIE->WriteSource = PKImageEncode_WriteSource;

    pIE->WritePixelsBandedBegin = JXRLIB_API(PKImageEncode_WritePixelsBandedBegin);
    pIE->WritePixelsBanded = JXRLIB_API(PKImageEncode_WritePixelsBanded);
    pIE->WritePixelsBandedEnd = JXRLIB_API(PKImageEncode_WritePixelsBandedEnd);

    pIE->CreateNewFrame = JXRLIB_API(PKImageEncode_CreateNewFrame);
    pIE->Release = JXRLIB_API(PKImageEncode_Release);
    pIE->bWMP = FALSE;

Cleanup:
    return err;
}


//================================================================
// PKImageDecode
//================================================================
ERR JXRLIB_API(PKImageDecode_Initialize)(
    PKImageDecode* pID,
    struct tagWMPStream* pStream)
{
    ERR err = WMP_errSuccess;

    pID->pStream = pStream;
    pID->guidPixFormat = JXRLIB_API(GUID_PKPixelFormatDontCare);
    pID->fResX = 96;
    pID->fResY = 96;
    pID->cFrame = 1;

    Call(pID->pStream->GetPos(pID->pStream, &pID->offStart));

    memset(&pID->WMP.wmiDEMisc, 0, sizeof(pID->WMP.wmiDEMisc));

Cleanup:
    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_GetPixelFormat)(
    PKImageDecode* pID,
    PKPixelFormatGUID* pPF)
{
    *pPF = pID->guidPixFormat;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_GetSize)(
    PKImageDecode* pID,
    I32* piWidth,
    I32* piHeight)
{
    *piWidth = (I32)pID->uWidth;
    *piHeight = (I32)pID->uHeight;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_GetResolution)(
    PKImageDecode* pID,
    Float* pfResX,
    Float* pfResY)
{
    *pfResX = pID->fResX;
    *pfResY = pID->fResY;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_GetColorContext)(PKImageDecode* pID, U8* pbColorContext, U32* pcbColorContext)
{
    UNREFERENCED_PARAMETER(pID);
    UNREFERENCED_PARAMETER(pbColorContext);
    UNREFERENCED_PARAMETER(pcbColorContext);
    return WMP_errNotYetImplemented;
}

ERR JXRLIB_API(PKImageDecode_GetDescriptiveMetadata)(PKImageDecode* pIE, DESCRIPTIVEMETADATA* pDescMetadata)
{
    UNREFERENCED_PARAMETER(pIE);
    UNREFERENCED_PARAMETER(pDescMetadata);
    return WMP_errNotYetImplemented;
}

ERR JXRLIB_API(PKImageDecode_Copy)(
    PKImageDecode* pID,
    const PKRect* pRect,
    U8* pb,
    U32 cbStride)
{
    UNREFERENCED_PARAMETER(pID);
    UNREFERENCED_PARAMETER(pRect);
    UNREFERENCED_PARAMETER(pb);
    UNREFERENCED_PARAMETER(cbStride);
    return WMP_errAbstractMethod;
}

ERR JXRLIB_API(PKImageDecode_GetFrameCount)(
    PKImageDecode* pID,
    U32* puCount)
{
    *puCount = pID->cFrame;

    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_SelectFrame)(
    PKImageDecode* pID,
    U32 uFrame)
{
    UNREFERENCED_PARAMETER(pID);
    UNREFERENCED_PARAMETER(uFrame);
    // NYI
    return WMP_errSuccess;
}

ERR JXRLIB_API(PKImageDecode_Release)(
    PKImageDecode** ppID)
{
    PKImageDecode* pID = *ppID;

    pID->fStreamOwner&& pID->pStream->Close(&pID->pStream);

    return JXRLIB_API(PKFree)((void**)ppID);
}

ERR JXRLIB_API(PKImageDecode_Create)(
    PKImageDecode** ppID)
{
    ERR err = WMP_errSuccess;
    PKImageDecode* pID = NULL;

    Call(JXRLIB_API(PKAlloc)((void**)ppID, sizeof(**ppID)));

    pID = *ppID;
    pID->Initialize = JXRLIB_API(PKImageDecode_Initialize);
    pID->GetPixelFormat = JXRLIB_API(PKImageDecode_GetPixelFormat);
    pID->GetSize = JXRLIB_API(PKImageDecode_GetSize);
    pID->GetResolution = JXRLIB_API(PKImageDecode_GetResolution);
    pID->GetColorContext = JXRLIB_API(PKImageDecode_GetColorContext);
    pID->GetDescriptiveMetadata = JXRLIB_API(PKImageDecode_GetDescriptiveMetadata);
    pID->Copy = JXRLIB_API(PKImageDecode_Copy);
    pID->GetFrameCount = JXRLIB_API(PKImageDecode_GetFrameCount);
    pID->SelectFrame = JXRLIB_API(PKImageDecode_SelectFrame);
    pID->Release = JXRLIB_API(PKImageDecode_Release);

Cleanup:
    return err;
}
