// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

// This file contains the definitions of the macros used to export/import the libCZIAPI.
// * when running doxygen, the macro "LIBCZIAPI_EXPORTS_FOR_DOXYGEN" and we use defines which are then shown in the documentation.
// * when compiling the libCZIAPI, the macro "LIBCZIAPI_EXPORTS" is defined and we use the __declspec(dllexport) attribute.
// * other cases are currently not worked out 

#ifdef LIBCZIAPI_EXPORTS_FOR_DOXYGEN
    #define LIBCZIAPI_STDCALL
#elif __GNUC__
    #if defined(__i386__)
        #define LIBCZIAPI_STDCALL __attribute__((stdcall))
    #else
        #define LIBCZIAPI_STDCALL __attribute__((stdcall))
    #endif
#else
    #define LIBCZIAPI_STDCALL __stdcall
#endif

#ifdef LIBCZIAPI_EXPORTS_FOR_DOXYGEN
    #define EXTERNALLIBCZIAPI_API(_returntype_) _returntype_
#elif LIBCZIAPI_EXPORTS_SHARED
    #ifdef __GNUC__
        #define LIBCZIAPI_API __attribute__ ((visibility ("default")))
        #if defined(__i386__)
        #define EXTERNALLIBCZIAPI_API(_returntype_)  extern "C"  _returntype_ __attribute__ ((visibility ("default"))) __attribute__((stdcall))
        #else
        #define EXTERNALLIBCZIAPI_API(_returntype_)  extern "C"  _returntype_ __attribute__ ((visibility ("default"))) 
        #endif
    #else
        #define EXTERNALLIBCZIAPI_API(_returntype_)  extern "C"  _returntype_  __declspec(dllexport)  __stdcall
    #endif
#elif _LIBCZIAPISTATICLIB
    #define EXTERNALLIBCZIAPI_API(_returntype_) _returntype_
    #define LIBCZIAPI_API
#else
    #define LIBCZIAPI_API __declspec(dllexport)
    #define EXTERNALLIBCZIAPI_API(_returntype_)  extern "C"  _returntype_ LIBCZIAPI_API  __stdcall 
#endif
