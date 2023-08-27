// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "decoder.h"
#include "../JxrDecode/JxrDecode.h"
#include "../JxrDecode/JxrDecode2.h"
#include "bitmapData.h"
#include "stdAllocator.h"
#include "BitmapOperations.h"
#include "Site.h"

using namespace libCZI;
using namespace std;

class MemoryBlockOnCompressedData : public libCZI::IMemoryBlock
{
private:
    mutable JxrDecode2::CompressedData compressed_data_;
public:
    MemoryBlockOnCompressedData(JxrDecode2::CompressedData&& compressed_data)
        : compressed_data_(std::move(compressed_data))
    {}

    void* GetPtr() override
    {
        return this->compressed_data_.GetMemory();
    }

    size_t GetSizeOfData() const override
    {
        return this->compressed_data_.GetSize();
    }
};

static libCZI::PixelType PixelTypeFromJxrPixelFormat(JxrDecode2::PixelFormat pixel_format)
{
    switch (pixel_format)
    {
    case JxrDecode2::PixelFormat::kBgr24:
        return PixelType::Bgr24;
    case JxrDecode2::PixelFormat::kGray8:
        return PixelType::Gray8;
    case JxrDecode2::PixelFormat::kBgr48:
        return PixelType::Bgr48;
    case JxrDecode2::PixelFormat::kGray16:
        return PixelType::Gray16;
    default:
        return PixelType::Invalid;
    }
}

/*static*/std::shared_ptr<CJxrLibDecoder> CJxrLibDecoder::Create()
{
    //return make_shared<CJxrLibDecoder>(JxrDecode::Initialize());
    return make_shared<CJxrLibDecoder>();
}

std::shared_ptr<libCZI::IMemoryBlock> CJxrLibDecoder::Encode(libCZI::PixelType pixel_type, std::uint32_t width, std::uint32_t height, std::uint32_t stride, const void* ptrData, float quality)
{
    JxrDecode2::PixelFormat jxrdecode_pixel_format;
    switch (pixel_type)
    {
    case PixelType::Bgr24:
        jxrdecode_pixel_format = JxrDecode2::PixelFormat::kBgr24;
        break;
    case PixelType::Bgr48:
        jxrdecode_pixel_format = JxrDecode2::PixelFormat::kBgr48;
        break;
    case PixelType::Gray8:
        jxrdecode_pixel_format = JxrDecode2::PixelFormat::kGray8;
        break;
    case PixelType::Gray16:
        jxrdecode_pixel_format = JxrDecode2::PixelFormat::kGray16;
        break;
    case PixelType::Gray32Float:
        jxrdecode_pixel_format = JxrDecode2::PixelFormat::_32bppGrayFloat;
        break;
    default:
        throw std::logic_error("unsupported pixel type");
    }

    JxrDecode2 decoder2;
    auto compressed_data = decoder2.Encode(
        jxrdecode_pixel_format,
        width,
        height,
        stride,
        ptrData,
        quality);

    return make_shared<MemoryBlockOnCompressedData>(std::move(compressed_data));
}

std::shared_ptr<libCZI::IBitmapData> CJxrLibDecoder::Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
    std::shared_ptr<IBitmapData> bitmap;
    bool bitmap_is_locked = false;
    JxrDecode2 decoder2;
    try
    {
        decoder2.Decode(
            ptrData,
            size,
            [&](JxrDecode2::PixelFormat actual_pixel_format, std::uint32_t actual_width, std::uint32_t actual_height)
            -> tuple<JxrDecode2::PixelFormat, uint32_t, void*>
            {
                const auto pixel_type_from_compressed_data = PixelTypeFromJxrPixelFormat(actual_pixel_format);
                if (pixel_type_from_compressed_data == PixelType::Invalid)
                {
                    throw std::logic_error("unsupported pixel type");
                }

                if (pixel_type_from_compressed_data != pixelType)
                {
                    ostringstream ss;
                    ss << "pixel type mismatch: expected \"" << Utils::PixelTypeToInformalString(pixelType) << "\", but got \"" << Utils::PixelTypeToInformalString(pixel_type_from_compressed_data) << "\"";
                    throw std::logic_error(ss.str());
                }

                bitmap = GetSite()->CreateBitmap(pixel_type_from_compressed_data , actual_width, actual_height);
                const auto lock_info = bitmap->Lock();
                bitmap_is_locked = true;
                return make_tuple(actual_pixel_format, lock_info.stride, lock_info.ptrDataRoi);
            });
    }
    catch (const std::exception& e)
    {
        GetSite()->Log(LOGLEVEL_ERROR, e.what());
        if (bitmap_is_locked)
        {
            bitmap->Unlock();
        }

        throw;
    }

    bitmap->Unlock();

    return bitmap;
}

#if false
std::shared_ptr<libCZI::IBitmapData> CJxrLibDecoder::Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
    std::shared_ptr<IBitmapData> bm;
    JxrDecode2 decoder2;
    decoder2.Decode(nullptr,
        ptrData,
        size,
        nullptr,
        [&](JxrDecode2::PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)->void
            {
                /*  if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
                  {
                      stringstream ss; ss << "JxrDecode: decode done - pixelfmt=" << JxrDecode::PixelFormatAsInformalString(pixFmt) << " width=" << width << " height=" << height << " linesCount=" << linesCount << " stride=" << stride;
                      GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
                  }*/

                  // TODO: it seems feasible to directly decode to the buffer (saving us the copy)
                PixelType px_type;
                switch (pixFmt)
                {
                case JxrDecode2::PixelFormat::kBgr24: px_type = PixelType::Bgr24; break;
                case JxrDecode2::PixelFormat::kGray8: px_type = PixelType::Gray8; break;
                case JxrDecode2::PixelFormat::kBgr48: px_type = PixelType::Bgr48; break;
                case JxrDecode2::PixelFormat::kGray16: px_type = PixelType::Gray16; break;
                    //case JxrDecode2::PixelFormat::_32bppGrayFloat: px_type = PixelType::Gray32Float; break;
                default: throw std::logic_error("need to look into these formats...");
                }

                bm = GetSite()->CreateBitmap(px_type, width, height);
                auto bmLckInfo = ScopedBitmapLockerSP(bm);
                if (bmLckInfo.stride != stride)
                {
                    for (uint32_t i = 0; i < linesCount; ++i)
                    {
                        memcpy(static_cast<char*>(bmLckInfo.ptrDataRoi) + i * bmLckInfo.stride, static_cast<const char*>(ptrData) + i * stride, stride);
                    }
                }
                else
                {
                    memcpy(bmLckInfo.ptrDataRoi, ptrData, static_cast<size_t>(stride) * linesCount);
                }

                // since BGR48 is not available as output, we need to convert (#36)
                if (px_type == PixelType::Bgr48)
                {
                    CBitmapOperations::RGB48ToBGR48(width, height, (uint16_t*)bmLckInfo.ptrDataRoi, bmLckInfo.stride);
                }
}
    );

    return bm;
}
#endif

#if false
std::shared_ptr<libCZI::IBitmapData> CJxrLibDecoder::Decode2(const void* ptrData, size_t size, libCZI::PixelType pixelType, uint32_t width, uint32_t height)
{
    std::shared_ptr<IBitmapData> bm;

    JxrDecode::WMPDECAPPARGS args; args.Clear();
    args.uAlphaMode = 0;    // we don't need any alpha, never

    try
    {
        if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
        {
            stringstream ss; ss << "Begin JxrDecode with " << size << " bytes";
            GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
        }

        JxrDecode::Decode(this->handle, &args, ptrData, size,
            [pixelType, width, height](JxrDecode::PixelFormat decPixFmt, int decodedWidth, int decodedHeight)->JxrDecode::PixelFormat
            {
                // We get the "original pixelformat" of the compressed data, and we need to respond
                // which pixelformat we want to get from the decoder.
                // ...one problem is that not all format-conversions are possible - we choose the "closest"
                JxrDecode::PixelFormat destFmt;
                switch (decPixFmt)
                {
                case JxrDecode::PixelFormat::_24bppBGR:
                case JxrDecode::PixelFormat::_32bppRGBE:
                case JxrDecode::PixelFormat::_32bppCMYK:
                case JxrDecode::PixelFormat::_32bppBGR:
                case JxrDecode::PixelFormat::_16bppBGR555:
                case JxrDecode::PixelFormat::_16bppBGR565:
                case JxrDecode::PixelFormat::_32bppBGR101010:
                case JxrDecode::PixelFormat::_24bppRGB:
                case JxrDecode::PixelFormat::_32bppBGRA:
                    destFmt = JxrDecode::PixelFormat::_24bppBGR;
                    if (pixelType != libCZI::PixelType::Bgr24)
                    {
                        throw std::runtime_error("pixel type validation failed...");
                    }

                    break;
                case JxrDecode::PixelFormat::_1bppBlackWhite:
                case JxrDecode::PixelFormat::_8bppGray:
                    destFmt = JxrDecode::PixelFormat::_8bppGray;
                    if (pixelType != libCZI::PixelType::Gray8)
                    {
                        throw std::runtime_error("pixel type validation failed...");
                    }

                    break;
                case JxrDecode::PixelFormat::_16bppGray:
                case JxrDecode::PixelFormat::_16bppGrayFixedPoint:
                case JxrDecode::PixelFormat::_16bppGrayHalf:
                    destFmt = JxrDecode::PixelFormat::_16bppGray;
                    if (pixelType != libCZI::PixelType::Gray16)
                    {
                        throw std::runtime_error("pixel type validation failed...");
                    }

                    break;
                case JxrDecode::PixelFormat::_32bppGrayFixedPoint:
                case JxrDecode::PixelFormat::_32bppGrayFloat:
                    destFmt = JxrDecode::PixelFormat::_32bppGrayFloat;
                    if (pixelType != libCZI::PixelType::Bgra32)
                    {
                        throw std::runtime_error("pixel type validation failed...");
                    }

                    break;
                case JxrDecode::PixelFormat::_48bppRGB:
                    destFmt = JxrDecode::PixelFormat::_48bppRGB;
                    if (pixelType != libCZI::PixelType::Bgr48)
                    {
                        throw std::runtime_error("pixel type validation failed...");
                    }

                    break;
                case JxrDecode::PixelFormat::_48bppRGBFixedPoint:
                case JxrDecode::PixelFormat::_48bppRGBHalf:
                case JxrDecode::PixelFormat::_96bppRGBFixedPoint:
                case JxrDecode::PixelFormat::_128bppRGBFloat:
                case JxrDecode::PixelFormat::_64bppCMYK:
                case JxrDecode::PixelFormat::_64bppRGBA:
                case JxrDecode::PixelFormat::_64bppRGBAFixedPoint:
                case JxrDecode::PixelFormat::_64bppRGBAHalf:
                case JxrDecode::PixelFormat::_128bppRGBAFixedPoint:
                case JxrDecode::PixelFormat::_128bppRGBAFloat:
                case JxrDecode::PixelFormat::_40bppCMYKA:
                case JxrDecode::PixelFormat::_80bppCMYKA:
                default:
                    throw std::logic_error("need to look into these formats...");
                }

                if (width != decodedWidth || height != decodedHeight)
                {
                    throw std::runtime_error("width and/or height validation failed...");
                }

                if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
                {
                    stringstream ss; ss << "JxrDecode: original pixelfmt: " << JxrDecode::PixelFormatAsInformalString(decPixFmt) << ", requested pixelfmt: " << JxrDecode::PixelFormatAsInformalString(destFmt);
                    GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
                }

                if (destFmt == JxrDecode::PixelFormat::invalid)
                {
                    throw std::logic_error("need to look into these formats...");
                }

                return destFmt;
            },
            [&](JxrDecode::PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)->void
            {
                if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
                {
                    stringstream ss; ss << "JxrDecode: decode done - pixelfmt=" << JxrDecode::PixelFormatAsInformalString(pixFmt) << " width=" << width << " height=" << height << " linesCount=" << linesCount << " stride=" << stride;
                    GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
                }

                // TODO: it seems feasible to directly decode to the buffer (saving us the copy)
                PixelType px_type;
                switch (pixFmt)
                {
                case JxrDecode::PixelFormat::_24bppBGR: px_type = PixelType::Bgr24; break;
                case JxrDecode::PixelFormat::_8bppGray: px_type = PixelType::Gray8; break;
                case JxrDecode::PixelFormat::_48bppRGB: px_type = PixelType::Bgr48; break;
                case JxrDecode::PixelFormat::_16bppGray: px_type = PixelType::Gray16; break;
                case JxrDecode::PixelFormat::_32bppGrayFloat: px_type = PixelType::Gray32Float; break;
                default: throw std::logic_error("need to look into these formats...");
                }

                bm = GetSite()->CreateBitmap(px_type, width, height);
                auto bmLckInfo = ScopedBitmapLockerSP(bm);
                if (bmLckInfo.stride != stride)
                {
                    for (uint32_t i = 0; i < linesCount; ++i)
                    {
                        memcpy(static_cast<char*>(bmLckInfo.ptrDataRoi) + i * bmLckInfo.stride, static_cast<const char*>(ptrData) + i * stride, stride);
                    }
                }
                else
                {
                    memcpy(bmLckInfo.ptrDataRoi, ptrData, static_cast<size_t>(stride) * linesCount);
                }

                // since BGR48 is not available as output, we need to convert (#36)
                if (px_type == PixelType::Bgr48)
                {
                    CBitmapOperations::RGB48ToBGR48(width, height, (uint16_t*)bmLckInfo.ptrDataRoi, bmLckInfo.stride);
                }
            });
    }
    catch (std::runtime_error& err)
    {
        // TODO: now what...?
        if (GetSite()->IsEnabled(LOGLEVEL_ERROR))
        {
            stringstream ss;
            ss << "Exception 'runtime_error' caught from JXR-decoder-invocation -> \"" << err.what() << "\".";
            GetSite()->Log(LOGLEVEL_ERROR, ss);
        }

        throw;
    }
    catch (std::exception& excp)
    {
        // TODO: now what...?
        if (GetSite()->IsEnabled(LOGLEVEL_ERROR))
        {
            stringstream ss;
            ss << "Exception caught from JXR-decoder-invocation -> \"" << excp.what() << "\".";
            GetSite()->Log(LOGLEVEL_ERROR, ss);
        }

        throw;
    }

    return bm;
}
#endif
