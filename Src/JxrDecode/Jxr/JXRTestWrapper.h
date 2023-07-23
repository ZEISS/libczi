#pragma once


#ifdef __cplusplus
extern "C" {
#endif
    typedef struct tagJxrTestWrapperInitializeInfo
    {
        void(*pfnPutData)(
                PKPixelFormatGUID pixeltype,
                unsigned int width,
                unsigned int height,
                unsigned int cLines,
                void* ptrData,
                unsigned int stride,
                void* userParam);
        void* userParamPutData;
    } JxrTestWrapperInitializeInfo;

    //ERR PKImageEncode_Initialize_Wrapper(struct tagPKImageEncode* pIE, struct WMPStream* stream, void* vp, size_t size);
    //ERR PKImageEncode_Create_Wrapper(struct tagPKImageEncode** ppIE);
    //ERR PKImageEncode_WritePixels_Wrapper(
    //struct PKImageEncode* pIE,
    //U32 cLine,
    //U8* pbPixel,
    //U32 cbStride);

#ifdef __cplusplus
}
#endif
