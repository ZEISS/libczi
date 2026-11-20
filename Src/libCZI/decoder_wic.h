// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Config_Internal.h"

#if LIBCZI_WINDOWSAPI_AVAILABLE

#include <memory>
#include <cstdint>
#include "libCZI_Pixels.h"
#include "libCZI_Site.h"

struct IWICBitmapDecoder;
struct IWICImagingFactory;

namespace libCZI
{
    namespace detail
    {
        class WicDecoderBase
        {
            static std::once_flag factory_created;
            static IWICImagingFactory* pFactory;
        public:
            enum class WicContainerFormat : std::uint8_t
            {
                Jpeg,
                JpegXR,
            };
            
            WicDecoderBase();
            ~WicDecoderBase();

        protected:
            std::shared_ptr<libCZI::IBitmapData> Decode(WicContainerFormat container_format, const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height);

            IWICImagingFactory* GetFactory() const
            {
                return WicDecoderBase::pFactory;
            }
        };

        class CWicJpgxrDecoder : public libCZI::IDecoder, WicDecoderBase
        {
            /*
        private:
            IWICImagingFactory* pFactory;*/
        public:
            static std::shared_ptr<CWicJpgxrDecoder> Create();
            /*
            CWicJpgxrDecoder() = delete;
            explicit CWicJpgxrDecoder(IWICImagingFactory* pFactory);
            virtual ~CWicJpgxrDecoder();
            */
        public:
            std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;
        };

        class CWicJpgDecoder : public libCZI::IDecoder, WicDecoderBase
        {
        //private:
        //    IWICImagingFactory* pFactory;
        public:
            static std::shared_ptr<CWicJpgDecoder> Create();

            //CWicJpgDecoder() = delete;
            //explicit CWicJpgDecoder(IWICImagingFactory* pFactory);
            //virtual ~CWicJpgDecoder();

        public:
            std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;
        };

    }  // namespace detail
} // namespace libCZI

#endif
