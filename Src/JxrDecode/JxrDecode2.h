#pragma once

#include <functional>
#include <tuple>
#include <cstdint>

class JxrDecode2
{
public:
    enum class PixelFormat
    {
        kInvalid,
        kBgr24,
        kBgr48,
        kGray8,
        kGray16,
        /*dontCare,
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

        invalid*/
    };
    
    typedef void* codecHandle;

    void Decode(
           codecHandle h,
          // const WMPDECAPPARGS* decArgs,
           const void* ptrData,
           size_t size,
           const std::function<PixelFormat(PixelFormat, int width, int height)>& selectDestPixFmt,
           std::function<void(PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)> deliverData);

    void Decode(
            const void* ptrData,
            size_t size,
            const std::function<std::tuple<JxrDecode2::PixelFormat, std::uint32_t, void*>(PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height)>& get_destination_func);
};
