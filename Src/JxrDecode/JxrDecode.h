// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <functional>
#include <string.h>

namespace JxrDecode
{
		enum class PixelFormat
		{
				dontCare,
				_24bppBGR,
				_1bppBlackWhite,
				_8bppGray,
				_16bppGray,
				_16bppGrayFixedPoint,
				_16bppGrayHalf,
				_32bppGrayFixedPoint,
				_32bppGrayFloat,
				_24bppRGB,
				_48bppRGB,
				_48bppRGBFixedPoint,
				_48bppRGBHalf,
				_96bppRGBFixedPoint,
				_128bppRGBFloat,
				_32bppRGBE,
				_32bppCMYK,
				_64bppCMYK,
				_32bppBGRA,
				_64bppRGBA,
				_64bppRGBAFixedPoint,
				_64bppRGBAHalf,
				_128bppRGBAFixedPoint,
				_128bppRGBAFloat,
				_16bppBGR555,
				_16bppBGR565,
				_32bppBGR101010,
				_40bppCMYKA,
				_80bppCMYKA,
				_32bppBGR,

				invalid
		};

		const char* PixelFormatAsInformalString(PixelFormat pfmt);

		enum class Orientation :std::uint8_t
		{
				// CRW: Clock Wise 90% Rotation; FlipH: Flip Horizontally;  FlipV: Flip Vertically
				// Peform rotation FIRST!
				//                CRW FlipH FlipV
				O_NONE = 0,    // 0    0     0
				O_FLIPV,       // 0    0     1
				O_FLIPH,       // 0    1     0
				O_FLIPVH,      // 0    1     1
				O_RCW,         // 1    0     0
				O_RCW_FLIPV,   // 1    0     1
				O_RCW_FLIPH,   // 1    1     0
				O_RCW_FLIPVH,  // 1    1     1
				/* add new ORIENTATION here */ O_MAX
		};

		enum class Subband : std::uint8_t
		{
				SB_ALL = 0,         // keep all subbands
				SB_NO_FLEXBITS,     // skip flex bits
				SB_NO_HIGHPASS,     // skip highpass
				SB_DC_ONLY,         // skip lowpass and highpass, DC only
				SB_ISOLATED,        // not decodable
				/* add new SUBBAND here */ SB_MAX
		};

		struct WMPDECAPPARGS
		{
				JxrDecode::PixelFormat pixFormat;

				// region decode
				size_t rLeftX;
				size_t rTopY;
				size_t rWidth;
				size_t rHeight;

				std::uint8_t cPostProcStrength;
				JxrDecode::Orientation  oOrientation;
				JxrDecode::Subband sbSubband;
				/*	// thumbnail
					size_t tThumbnailFactor;

					// orientation
					ORIENTATION oOrientation;

					// post processing
					U8 cPostProcStrength;
					*/
				std::uint8_t uAlphaMode; // 0:no alpha 1: alpha only else: something + alpha 
				/*
				SUBBAND sbSubband;  // which subbands to keep (for transcoding)

				BITSTREAMFORMAT bfBitstreamFormat; // desired bitsream format (for transcoding)

				CWMIStrCodecParam wmiSCP;

				Bool bIgnoreOverlap;*/
				bool bIgnoreOverlap;

				void Clear()
				{
						memset(this, 0, sizeof(*this));
						this->pixFormat = JxrDecode::PixelFormat::dontCare;
						//args->bVerbose = FALSE;
						//args->tThumbnailFactor = 0;
						this->oOrientation = Orientation::O_NONE;
						this->cPostProcStrength = 0;
						this->uAlphaMode = 255;
						this->sbSubband = JxrDecode::Subband::SB_ALL;
				}
		};


		typedef void* codecHandle;

		codecHandle Initialize();

		void Decode(
				codecHandle h,
				const WMPDECAPPARGS* decArgs,
				const void* ptrData,
				size_t size,
				const std::function<PixelFormat(PixelFormat, int width, int height)>& selectDestPixFmt,
				std::function<void(PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)> deliverData);

		void Destroy(codecHandle h);

}