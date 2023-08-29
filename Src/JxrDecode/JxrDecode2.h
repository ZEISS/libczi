#pragma once

#include <functional>
#include <memory>
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
        kGray32Float,
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

    /// This class is used to represent a blob containing the compressed data.
    /// Note that it is movable but not copyable.
    class CompressedData
    {
    private:
        void* obj_handle_;
    public:
        CompressedData() :obj_handle_(nullptr) {} // default constructor
        CompressedData& operator=(CompressedData&& other) noexcept
        {
            // "other" is soon going to be destroyed, so we let it destroy our current resource instead and we take "other"'s current resource via swapping
            std::swap(this->obj_handle_, other.obj_handle_);
            return *this;
        }

        // move constructor, takes a rvalue reference &&
        CompressedData(CompressedData&& other) noexcept
        {
            // we "steal" the resource from "other"
            this->obj_handle_ = other.obj_handle_;
            // "other" will soon be destroyed and its destructor will do nothing because we null out its resource here
            other.obj_handle_ = nullptr;
        }

        CompressedData(const CompressedData&) = delete; // prevent copy constructor to be used
        CompressedData& operator=(const CompressedData&) = delete; // prevent copy assignment to be used

        void* GetMemory();
        size_t GetSize();
        ~CompressedData();
    protected:
        friend class JxrDecode2;
        CompressedData(void* obj_handle) :obj_handle_(obj_handle) {}
    };

    void Decode(
           codecHandle h,
          // const WMPDECAPPARGS* decArgs,
           const void* ptrData,
           size_t size,
           const std::function<PixelFormat(PixelFormat, int width, int height)>& selectDestPixFmt,
           std::function<void(PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)> deliverData);

    /// Decodes the specified data, giving an uncompressed bitmap.
    /// The course of action is as follows:
    /// * The decoder will be initialized with the specified compressed data.  
    /// * The characteristics of the compressed data will be determined - i.e the pixel type, width, height, etc. are determined.  
    /// * The 'get_destination_func' is called, passing in the pixel type, width, and height.    
    /// * The function should return a pointer to a buffer that can hold the uncompressed bitmap.
    ///    The tuple it returns is the pixel type, the stride and the pointer to the buffer.
    /// Notes:
    /// * The decoder will call the 'get_destination_func' function only once, and the buffer must be valid  
    ///   until the method returns.
    /// * The 'get_destination_func' function may choose to throw an exception (if the memory cannot be allocated,  
    ///   or the reported characteristics are determined to be invalid, etc). 
    ///
    /// \param  ptrData                 Information describing the pointer.
    /// \param  size                    The size.
    /// \param  get_destination_func    The get destination function.
    void Decode(
            const void* ptrData,
            size_t size,
            const std::function<std::tuple<JxrDecode2::PixelFormat, std::uint32_t, void*>(PixelFormat pixel_format, std::uint32_t  width, std::uint32_t  height)>& get_destination_func);

    CompressedData Encode(
            JxrDecode2::PixelFormat pixel_format,
            std::uint32_t  width,
            std::uint32_t  height,
            std::uint32_t  stride,
            const void* ptr_bitmap,
            float quality = 1.f);
};
