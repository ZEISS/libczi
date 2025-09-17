// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "ImportExport.h"
#include <cstdint>
#include <memory>
#include <algorithm>
#include <ostream>
#include <utility>

namespace libCZI
{
    /// A rectangle (with integer coordinates).
    struct LIBCZI_API IntRect
    {
        int x;  ///< The x-coordinate of the upper-left point of the rectangle.
        int y;  ///< The y-coordinate of the upper-left point of the rectangle.
        int w;  ///< The width of the rectangle.
        int h;  ///< The height of the rectangle.

        /// Invalidates this object.
        void Invalidate() { this->w = this->h = -1; }

        /// Returns a boolean indicating whether this rectangle contains valid information.
        bool IsValid() const { return this->w >= 0 && this->h >= 0; }

        /// Returns a boolean indicating whether this rectangle is valid and non-empty.
        bool IsNonEmpty() const { return this->w > 0 && this->h > 0; }

        /// Determine whether this rectangle intersects with the specified one.
        /// \param r The other rectangle.
        /// \return True if the two rectangles intersect, false otherwise.
        bool IntersectsWith(const IntRect& r) const
        {
            const IntRect is = this->Intersect(r);
            if (is.w <= 0 || is.h <= 0)
            {
                return false;
            }

            return true;
        }

        /// Calculate the intersection with the specified rectangle.
        ///
        /// \param r The rectangle for which the intersection is to be calculated.
        ///
        /// \return A rectangle which is the intersection of the two rectangles. If the two rectangles do not intersect, an empty rectangle is returned (width=height=0).
        IntRect Intersect(const IntRect& r) const
        {
            return IntRect::Intersect(*this, r);
        }

        /// Calculate the intersection of the two specified rectangle.
        ///
        /// \param a The first rectangle.
        /// \param b The second rectangle.
        ///
        /// \return A rectangle which is the intersection of the two rectangles. If the two rectangles do not intersect, an empty rectangle is returned (width=height=0).
        static IntRect Intersect(const IntRect& a, const IntRect& b)
        {
            const int x1 = (std::max)(a.x, b.x);
            const int x2 = (std::min)(a.x + a.w, b.x + b.w);
            const int y1 = (std::max)(a.y, b.y);
            const int y2 = (std::min)(a.y + a.h, b.y + b.h);
            if (x2 >= x1 && y2 >= y1)
            {
                return IntRect{ x1, y1, x2 - x1, y2 - y1 };
            }

            return IntRect{ 0,0,0,0 };
        }
    };

    /// A point (with integer coordinates).
    struct IntPoint
    {
        int x;  ///< The x coordinate.
        int y;  ///< The y coordinate.
    };

    /// Values that represent different frame of reference.
    enum class CZIFrameOfReference : std::uint8_t
    {
        Invalid,                        ///< Invalid frame of reference.
        Default,                        ///< The default frame of reference.
        RawSubBlockCoordinateSystem,    ///< The raw sub-block coordinate system.
        PixelCoordinateSystem,          ///< The pixel coordinate system.
    };

    /// This structure combines a rectangle with a specification of the frame of reference.
    struct IntRectAndFrameOfReference
    {
        libCZI::CZIFrameOfReference frame_of_reference{ libCZI::CZIFrameOfReference::Invalid }; ///< The frame of reference.
        libCZI::IntRect rectangle;  ///< The rectangle.
    };

    /// This structure combines a point with a specification of the frame of reference.
    struct IntPointAndFrameOfReference
    {
        libCZI::CZIFrameOfReference frame_of_reference{ libCZI::CZIFrameOfReference::Invalid }; ///< The frame of reference.
        IntPoint point;  ///< The point.
    };

    /// A rectangle (with double coordinates).
    struct LIBCZI_API DblRect
    {
        double x;   ///< The x-coordinate of the upper-left point of the rectangle.
        double y;   ///< The y-coordinate of the upper-left point of the rectangle.
        double w;   ///< The width of the rectangle.
        double h;   ///< The height of the rectangle.

        /// Invalidates this object.
        void Invalidate() { this->w = this->h = -1; }
    };

    ///  A structure representing a size (width and height) in integers.
    struct IntSize
    {
        std::uint32_t w;    ///< The width
        std::uint32_t h;    ///< The height
    };

    /// A structure representing an R-G-B-color triple (as bytes).
    struct Rgb8Color
    {
        std::uint8_t r; ///< The red component.
        std::uint8_t g; ///< The green component.
        std::uint8_t b; ///< The blue component.
    };

    /// A structure representing an R-G-B-color triple (as floats).
    struct RgbFloatColor
    {
        float r; ///< The red component.
        float g; ///< The green component.
        float b; ///< The blue component.
    };

    /// An enum representing a pixel-type.
    enum class PixelType : std::uint8_t
    {
        Invalid = 0xff,             ///< Invalid pixel type.
        Gray8 = 0,                  ///< Grayscale 8-bit unsigned.
        Gray16 = 1,                 ///< Grayscale 16-bit unsigned.
        Gray32Float = 2,            ///< Grayscale 4 byte float.
        Bgr24 = 3,                  ///< BGR-color 8-bytes triples (memory order B, G, R).
        Bgr48 = 4,                  ///< BGR-color 16-bytes triples (memory order B, G, R).
        Bgr96Float = 8,             ///< BGR-color 4 byte float triples (memory order B, G, R).
        Bgra32 = 9,                 ///< Currently not supported in libCZI.
        Gray64ComplexFloat = 10,    ///< Currently not supported in libCZI.
        Bgr192ComplexFloat = 11,    ///< Currently not supported in libCZI.
        Gray32 = 12,                ///< Currently not supported in libCZI.
        Gray64Float = 13,           ///< Currently not supported in libCZI.
    };

    /// An enum specifying the compression method.
    enum class CompressionMode : std::uint8_t
    {
        Invalid = 0xff,     ///< Invalid compression type.
        UnCompressed = 0,   ///< The data is uncompressed.
        Jpg = 1,            ///< The data is JPG-compressed.
        JpgXr = 4,          ///< The data is JPG-XR-compressed.
        Zstd0 = 5,          ///< The data is compressed with zstd.
        Zstd1 = 6           ///< The data contains a header, followed by a zstd-compressed block. 
    };

    /// This enum is used in the context of a subblock to describe which "type of pyramid" is represented by the subblock.
    /// The significance and importance of this enum is not yet fully understood, and seems questionable. It is not recommended
    /// to make use of it at this point for any purposes.
    enum class SubBlockPyramidType : std::uint8_t
    {
        Invalid = 0xff,     ///< Invalid pyramid type.
        None = 0,           ///< No pyramid (indicating that the subblock is not a pyramid subblock, but a layer-0 subblock).    
        SingleSubBlock = 1, ///< The subblock is a pyramid subblock, and it covers a single subblock of the lower layer (or: it is a minification of a single lower-layer-subblock).
        MultiSubBlock = 2   ///< The subblock is a pyramid subblock, and it covers multiple subblocks of the lower layer.
    };

    /// Information about a locked bitmap - allowing direct access to the image data in memory.
    struct BitmapLockInfo
    {
        void* ptrData;              ///< Not currently used, to be ignored.
        void* ptrDataRoi;           ///< The pointer to the first (top-left) pixel of the bitmap.
        std::uint32_t   stride;     ///< The stride of the bitmap data (pointed to by `ptrDataRoi`).
        std::uint64_t   size;       ///< The size of the bitmap data (pointed to by `ptrDataRoi`) in bytes.
    };

    /// This interface is used to represent a bitmap.
    ///
    /// In order to access the pixel data, the Lock-method must be called. The information returned 
    /// from the Lock-method is to be considered valid only until Unlock is called. If a bitmap is
    /// destroyed while it is locked, this is considered to be a fatal error. It is legal to call Lock
    /// multiple times, but the calls to Lock and Unlock must be balanced.
    class LIBCZI_API IBitmapData
    {
    public:
        /// Default constructor.
        IBitmapData() = default;

        /// Gets pixel type.
        ///
        /// \return The pixel type.
        virtual PixelType       GetPixelType() const = 0;

        /// Gets the size of the bitmap (i.e. its width and height in pixels).
        ///
        /// \return The size (in pixels).
        virtual IntSize         GetSize() const = 0;

        /// Gets a data structure allowing for direct access of the bitmap.
        /// 
        /// The BitmapLockInfo returned must only be considered to be valid until Unlock is called.
        /// It is legal to call Lock multiple time (also from different threads concurrently).
        /// In any case, calls to Lock and Unlock must be balanced. It is considered to be a
        /// fatal error if the object is destroyed when it is locked.
        ///
        /// \return The BitmapLockInfo allowing to directly access the data representing the bitmap.
        virtual BitmapLockInfo  Lock() = 0;

        /// Inform the bitmap object that the data (previously retrieved by a call to Lock)
        /// is no longer used.
        /// If Unlock is called with the lock-count being zero, an exception of type std::logic_error
        /// will be thrown (and the lock-count will be unchanged).
        /// 
        /// The BitmapLockInfo returned must only be considered to be valid until Unlock is called.
        virtual void            Unlock() = 0;

        /// Get the lock count. Note that this value is only momentarily valid.
        ///
        /// \returns    The lock count.
        virtual int             GetLockCount() const = 0;

        virtual ~IBitmapData() = default;

        /// Copy-Constructor - deleted.
        IBitmapData(const IBitmapData& other) = delete;

        /// move constructor - deleted.
        IBitmapData(IBitmapData&& other) noexcept = delete;

        /// move assignment - deleted.
        IBitmapData& operator=(IBitmapData&& other) noexcept = delete;

        /// Copy assignment operator - deleted.
        IBitmapData& operator=(const IBitmapData& other) = delete;

        /// Gets the width of the bitmap in pixels.
        ///
        /// \return The width in pixels.
        std::uint32_t GetWidth() const { return this->GetSize().w; }

        /// Gets the height of the bitmap in pixels.
        ///
        /// \return The height in pixels
        std::uint32_t GetHeight() const { return this->GetSize().h; }
    };

    /// A helper class used to scope the lock state of a bitmap.
    ///
    /// It is intended to be used like this:
    /// \code{.cpp}
    ///          
    /// libCZI::IBitmapData* bm = ... // assume that we have a pointer to a bitmap
    /// 
    /// // this example assumes that the pixel type is Gray8 and nothing else...
    /// if (bm->GetPixelType() != libCZI::PixelType::Gray8) throw std::exception();
    /// 
    /// {
    ///     // access the bitmap's pixels directly within this scope
    ///     libCZI::ScopedBitmapLocker<libCZI::IBitmapData*> lckBm{ bm };   // <- will call bm->Lock here
    ///     for (std::uint32_t y  = 0; y < bm->GetHeight(); ++y)
    ///     {
    ///         const std::uint8_t* ptrLine = ((const std::uint8_t*)lckBm.ptrDataRoi) + y * lckBm.stride;
    ///         for (std::uint32_t x  = 0; x < bm->GetWidth(); ++x)
    ///         {
    ///             auto pixelVal = ptrLine[x];
    ///             // do something with the pixel...
    ///         }
    ///     }
    ///     
    ///     // when lckBm goes out of scope, bm->Unlock will be called
    ///  }
    ///
    /// \endcode
    /// 
    /// For convenience two typedef are provided: `ScopedBitmapLockerP` and `ScopedBitmapLockerSP` for
    /// use with the types `IBitmapData*` and `std::shared_ptr<IBitmapData>`.
    ///
    /// \code{.cpp}
    /// typedef ScopedBitmapLocker<IBitmapData*> ScopedBitmapLockerP;
    /// typedef ScopedBitmapLocker<std::shared_ptr<IBitmapData>> ScopedBitmapLockerSP;
    /// \endcode
    /// 
    /// So in above sample we could have used 
    /// \code{.cpp}
    /// libCZI::ScopedBitmapLockerP lckBm{ bm };
    /// \endcode
    /// 
    /// This utility is intended to help adhering to the RAII-pattern, since it makes writing exception-safe
    /// code easier - in case of an exception (within the scope of the ScopedBitmapLocker object) the bitmap's
    /// Unlock method will be called (which is cumbersome to achieve otherwise).
    template <typename TBitmap>
    class ScopedBitmapLocker : public BitmapLockInfo
    {
    private:
        TBitmap bitmap_data_;
    public:
        ScopedBitmapLocker() = delete;

        /// Constructor taking the object for which we provide the scope-guard.
        /// \param bitmap_data The object for which we are to provide the scope-guard.
        explicit ScopedBitmapLocker(const TBitmap& bitmap_data) : bitmap_data_(bitmap_data)
        {
            if (!bitmap_data)
            {
                throw std::invalid_argument("bitmap_data must not be null");
            }

            auto lockInfo = bitmap_data->Lock();
            this->ptrData = lockInfo.ptrData;
            this->ptrDataRoi = lockInfo.ptrDataRoi;
            this->stride = lockInfo.stride;
            this->size = lockInfo.size;
        }

        /// Copy-Constructor.
        /// \param other The other object.
        ScopedBitmapLocker(const ScopedBitmapLocker<TBitmap>& other) : bitmap_data_(other.bitmap_data_)
        {
            auto lockInfo = other.bitmap_data_->Lock();
            this->ptrData = lockInfo.ptrData;
            this->ptrDataRoi = lockInfo.ptrDataRoi;
            this->stride = lockInfo.stride;
            this->size = lockInfo.size;
        }

        /// Move constructor.
        ScopedBitmapLocker(ScopedBitmapLocker<TBitmap>&& other) noexcept
        {
            *this = std::move(other);
        }

        /// Move assignment operator.
        ScopedBitmapLocker<TBitmap>& operator=(ScopedBitmapLocker<TBitmap>&& other) noexcept
        {
            if (this != &other)
            {
                if (this->bitmap_data_)
                {
                    this->bitmap_data_->Unlock();
                }

                this->bitmap_data_ = std::move(other.bitmap_data_);
                this->ptrData = other.ptrData;
                this->ptrDataRoi = other.ptrDataRoi;
                this->stride = other.stride;
                this->size = other.size;
                other.ptrData = other.ptrDataRoi = nullptr;
                other.bitmap_data_ = TBitmap();
            }

            return *this;
        }

        /// Copy assignment operator.
        /// \param other The other object.
        ScopedBitmapLocker<TBitmap>& operator=(const ScopedBitmapLocker<TBitmap>& other)
        {
            if (this != &other)
            {
                // Unlock current bitmap if we have one
                if (this->bitmap_data_)
                {
                    this->bitmap_data_->Unlock();
                }

                // Copy the bitmap reference and lock it
                this->bitmap_data_ = other.bitmap_data_;
                auto lockInfo = this->bitmap_data_->Lock();
                this->ptrData = lockInfo.ptrData;
                this->ptrDataRoi = lockInfo.ptrDataRoi;
                this->stride = lockInfo.stride;
                this->size = lockInfo.size;
            }

            return *this;
        }

        ~ScopedBitmapLocker()
        {
            if (this->bitmap_data_)
            {
                this->bitmap_data_->Unlock();
            }
        }
    };

    /// Defines an alias representing the scoped bitmap locker for use with libCZI::IBitmapData.
    typedef ScopedBitmapLocker<IBitmapData*> ScopedBitmapLockerP;

    /// Defines an alias representing the scoped bitmap locker for use with a shared_ptr of type libCZI::IBitmapData.
    typedef ScopedBitmapLocker<std::shared_ptr<IBitmapData>> ScopedBitmapLockerSP;

    //-------------------------------------------------------------------------

    /// Information about a locked bitonal bitmap - allowing direct access to the image data in memory.
    struct BitonalBitmapLockInfo
    {
        void* ptrData;              ///< Pointer to the start of the bitonal bitmap data.
        std::uint32_t   stride;     ///< The stride of the bitmap data (pointed to by `ptrData`) in units of bytes.
        std::uint64_t   size;       ///< The size of the bitmap data (pointed to by `ptrData`) in bytes.
    };

    /// This interface is used to represent a bitonal bitmap - i.e. a bitmap where each pixel is represented by a single bit.
    ///
    /// In order to access the pixel data, the Lock-method must be called. The information returned 
    /// from the Lock-method is to be considered valid only until Unlock is called. If a bitmap is
    /// destroyed while it is locked, this is considered to be a fatal error. It is legal to call Lock
    /// multiple times, but the calls to Lock and Unlock must be balanced.
    class LIBCZI_API IBitonalBitmapData
    {
    public:
        /// Default constructor.
        IBitonalBitmapData() = default;

        /// Gets the size of the bitmap (i.e. its width and height in pixels).
        ///
        /// \return The size (in pixels).
        virtual IntSize GetSize() const = 0;

        /// Gets a data structure allowing for direct access of the bitmap.
        ///
        /// \return The BitmapLockInfo allowing to directly access the data representing the bitmap.
        virtual BitonalBitmapLockInfo Lock() = 0;

        /// Inform the bitmap object that the data (previously retrieved by a call to Lock)
        /// is no longer used.
        virtual void Unlock() = 0;

        /// Get the lock count. Note that this value is only momentarily valid.
        ///
        /// \returns    The lock count.
        virtual int GetLockCount() const = 0;

        virtual ~IBitonalBitmapData() = default;

        /// Copy-Constructor - deleted.
        IBitonalBitmapData(const IBitonalBitmapData& other) = delete;

        /// move constructor - deleted.
        IBitonalBitmapData(IBitonalBitmapData&& other) noexcept = delete;

        /// move assignment - deleted.
        IBitonalBitmapData& operator=(IBitonalBitmapData&& other) noexcept = delete;

        /// Copy assignment operator - deleted.
        IBitonalBitmapData& operator=(const IBitonalBitmapData& other) = delete;

        /// Gets the width of the bitmap in pixels.
        ///
        /// \return The width in pixels.
        std::uint32_t GetWidth() const { return this->GetSize().w; }

        /// Gets the height of the bitmap in pixels.
        ///
        /// \return The height in pixels
        std::uint32_t GetHeight() const { return this->GetSize().h; }
    };

    //-------------------------------------------------------------------------

    /// A helper class used to scope the lock state of a bitonal bitmap.
    ///
    /// It is intended to be used like this:
    /// \code{.cpp}
    ///          
    /// libCZI::IBitonalBitmapData* bm = ... // assume that we have a pointer to a bitonal bitmap
    /// 
    /// // access the bitonal bitmap's pixels directly within this scope
    /// libCZI::ScopedBitonalBitmapLocker<libCZI::IBitonalBitmapData*> lckBm{ bm };   // <- will call bm->Lock here
    /// for (std::uint32_t y  = 0; y < bm->GetHeight(); ++y)
    /// {
    ///     const std::uint8_t* ptrLine = ((const std::uint8_t*)lckBm.ptrData) + y * lckBm.stride;
    ///     // do something with the pixel data...
    /// }
    ///     
    /// // when lckBm goes out of scope, bm->Unlock will be called
    ///
    /// \endcode
    /// 
    /// For convenience two typedef are provided: `ScopedBitonalBitmapLockerP` and `ScopedBitonalBitmapLockerSP` for
    /// use with the types `IBitonalBitmapData*` and `std::shared_ptr<IBitonalBitmapData>`.
    ///
    /// \code{.cpp}
    /// typedef ScopedBitonalBitmapLocker<IBitonalBitmapData*> ScopedBitonalBitmapLockerP;
    /// typedef ScopedBitonalBitmapLocker<std::shared_ptr<IBitonalBitmapData>> ScopedBitonalBitmapLockerSP;
    /// \endcode
    /// 
    /// So in above sample we could have used 
    /// \code{.cpp}
    /// libCZI::ScopedBitonalBitmapLockerP lckBm{ bm };
    /// \endcode
    /// 
    /// This utility is intended to help adhering to the RAII-pattern, since it makes writing exception-safe
    /// code easier - in case of an exception (within the scope of the ScopedBitonalBitmapLocker object) the bitmap's
    /// Unlock method will be called (which is cumbersome to achieve otherwise).
    template <typename TBitonalBitmap>
    class ScopedBitonalBitmapLocker : public BitonalBitmapLockInfo
    {
    private:
        TBitonalBitmap bitonal_bitmap_data_;
    public:
        ScopedBitonalBitmapLocker() = delete;

        /// Constructor taking the object for which we provide the scope-guard.
        /// \param bitonal_bitmap_data The object for which we are to provide the scope-guard.
        explicit ScopedBitonalBitmapLocker(const TBitonalBitmap& bitonal_bitmap_data) : bitonal_bitmap_data_(bitonal_bitmap_data)
        {
            if (!bitonal_bitmap_data)
            {
                throw std::invalid_argument("bitonal_bitmap_data must not be null");
            }

            auto lock_info = bitonal_bitmap_data->Lock();
            this->ptrData = lock_info.ptrData;
            this->stride = lock_info.stride;
            this->size = lock_info.size;
        }

        /// Copy-Constructor.
        /// \param other The other object.
        ScopedBitonalBitmapLocker(const ScopedBitonalBitmapLocker<TBitonalBitmap>& other) : bitonal_bitmap_data_(other.bitonal_bitmap_data_)
        {
            auto lock_info = other.bitonal_bitmap_data_->Lock();
            this->ptrData = lock_info.ptrData;
            this->stride = lock_info.stride;
            this->size = lock_info.size;
        }

        /// Move constructor.
        ScopedBitonalBitmapLocker(ScopedBitonalBitmapLocker<TBitonalBitmap>&& other) noexcept : bitonal_bitmap_data_(TBitonalBitmap())
        {
            *this = std::move(other);
        }

        /// Move assignment operator.
        ScopedBitonalBitmapLocker<TBitonalBitmap>& operator=(ScopedBitonalBitmapLocker<TBitonalBitmap>&& other) noexcept
        {
            if (this != &other)
            {
                if (this->bitonal_bitmap_data_)
                {
                    this->bitonal_bitmap_data_->Unlock();
                }

                this->bitonal_bitmap_data_ = std::move(other.bitonal_bitmap_data_);
                this->ptrData = other.ptrData;
                this->stride = other.stride;
                this->size = other.size;
                other.ptrData = nullptr;
                other.bitonal_bitmap_data_ = TBitonalBitmap();
            }

            return *this;
        }

        /// Copy assignment operator.
        /// \param other The other object.
        ScopedBitonalBitmapLocker<TBitonalBitmap>& operator=(const ScopedBitonalBitmapLocker<TBitonalBitmap>& other)
        {
            if (this != &other)
            {
                // Unlock current bitmap if we have one
                if (this->bitonal_bitmap_data_)
                {
                    this->bitonal_bitmap_data_->Unlock();
                }

                // Copy the bitonal bitmap reference and lock it
                this->bitonal_bitmap_data_ = other.bitonal_bitmap_data_;
                auto lockInfo = this->bitonal_bitmap_data_->Lock();
                this->ptrData = lockInfo.ptrData;
                this->stride = lockInfo.stride;
                this->size = lockInfo.size;
            }

            return *this;
        }

        ~ScopedBitonalBitmapLocker()
        {
            if (this->bitonal_bitmap_data_)
            {
                this->bitonal_bitmap_data_->Unlock();
            }
        }
    };

    /// Defines an alias representing the scoped bitmap locker for use with libCZI::IBitonalBitmapData.
    typedef ScopedBitonalBitmapLocker<IBitonalBitmapData*> ScopedBitonalBitmapLockerP;

    /// Defines an alias representing the scoped bitmap locker for use with a shared_ptr of type libCZI::IBitonalBitmapData.
    typedef ScopedBitonalBitmapLocker<std::shared_ptr<IBitonalBitmapData>> ScopedBitonalBitmapLockerSP;

    /// \brief Utility functions for working with 1‑bit‑per‑pixel (bitonal) bitmaps.
    ///
    /// This class contains static helpers to read, write, and bulk-edit bitonal images that
    /// implement libCZI::IBitonalBitmapData. It provides overloads that:
    /// - Accept IBitonalBitmapData* or std::shared_ptr<IBitonalBitmapData> and manage Lock/Unlock
    ///   internally via ScopedBitonalBitmapLocker.
    /// - Accept a pre-acquired BitonalBitmapLockInfo plus the bitmap extent for zero-overhead
    ///   operations where the caller controls the lock lifetime.
    ///
    /// Provided operations:
    /// - GetPixelValue(...) - read a single pixel.
    /// - SetPixelValue(...) - write a single pixel.
    /// - Fill(...) - fill a rectangular ROI; the ROI is clipped to the bitmap extent.
    /// - SetAllPixels(...) - set all pixels to a uniform value.
    /// - CopyAt(...) - masked copy from a source bitmap to a destination bitmap; only pixels for
    ///   which the mask bit is set are copied, starting at the given destination offset. Regions
    ///   outside the destination are ignored.
    ///
    /// Locking and thread-safety:
    /// - Pointer/shared_ptr overloads perform Lock/Unlock internally and are safe to call even when
    ///   other threads also lock the same bitmap (IBitonalBitmapData permits multiple locks).
    /// - LockInfo overloads assume the caller holds a valid lock for the duration of the call. When
    ///   writing, ensure exclusive access to avoid data races.
    ///
    /// Memory layout:
    /// - Pixels are packed at 1 bit per pixel in scanlines separated by stride bytes.
    /// - Callers should not depend on the bit ordering within each byte; use these helpers instead
    ///   of decoding bits directly.
    ///
    /// Coordinates and clipping:
    /// - x and y are 0-based. All operations clip to [0, width) x [0, height).
    ///
    /// Example:
    /// \code{.cpp}
    /// // Clear an ROI and then set one pixel
    /// libCZI::IntRect roi{10, 20, 32, 8};
    /// libCZI::BitonalBitmapOperations::Fill(bitonal.get(), roi, false);
    /// libCZI::BitonalBitmapOperations::SetPixelValue(bitonal.get(), 12, 22, true);
    ///
    /// // High-performance path with a pre-acquired lock
    /// libCZI::ScopedBitonalBitmapLockerSP lock{ bitonal };
    /// libCZI::BitonalBitmapOperations::Fill(lock, bitonal->GetSize(), roi, true);
    ///
    /// // Masked copy into destination at (100, 50)
    /// libCZI::BitonalBitmapOperations::CopyAt(src.get(), mask.get(),
    ///     libCZI::IntPoint{100, 50}, dst.get());
    /// \endcode
    class LIBCZI_API BitonalBitmapOperations
    {
    public:
        /// Gets the value of a specific pixel in a bitonal bitmap.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bmData  The bitonal bitmap.
        /// \param  x       The x-coordinate of the pixel (0-based).
        /// \param  y       The y-coordinate of the pixel (0-based).
        /// \return         The value of the pixel (true for set/1, false for clear/0).
        static bool GetPixelValue(const std::shared_ptr<IBitonalBitmapData>& bmData, std::uint32_t x, std::uint32_t y)
        {
            ScopedBitonalBitmapLockerSP lock_bitmap{ bmData };
            return GetPixelValue(lock_bitmap, bmData->GetSize(), x, y);
        }

        /// Gets the value of a specific pixel in a locked bitonal bitmap.
        ///
        /// \param  lock_info   Information describing the locked bitonal bitmap data
        ///                     (ptrData points to the first scanline; stride is in bytes).
        /// \param  extent      The bitmap extent (width/height).
        /// \param  x           The x-coordinate of the pixel (0-based).
        /// \param  y           The y-coordinate of the pixel (0-based).
        /// \return             The value of the pixel (true for set/1, false for clear/0).
        static bool GetPixelValue(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y);

        /// Gets the value of a specific pixel in a locked bitonal bitmap.
        ///
        /// Convenience overload when the extent can be taken from the bitmap.
        ///
        /// \param  bitonal_bitmap  The bitonal bitmap the lock was acquired from.
        /// \param  lock_info       The lock information corresponding to bitonal_bitmap.
        /// \param  x               The x-coordinate of the pixel (0-based).
        /// \param  y               The y-coordinate of the pixel (0-based).
        /// \return                 The value of the pixel (true for set/1, false for clear/0).
        static bool GetPixelValue(const IBitonalBitmapData* bitonal_bitmap, const BitonalBitmapLockInfo& lock_info, std::uint32_t x, std::uint32_t y)
        {
            return GetPixelValue(lock_info, bitonal_bitmap->GetSize(), x, y);
        }

        /// Sets the value of a specific pixel in a bitonal bitmap.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify.
        /// \param  x                   The x-coordinate of the pixel (0-based).
        /// \param  y                   The y-coordinate of the pixel (0-based).
        /// \param  value               The value to write (true for set/1, false for clear/0).
        static void SetPixelValue(IBitonalBitmapData* bitonal_bitmap_data, std::uint32_t x, std::uint32_t y, bool value)
        {
            ScopedBitonalBitmapLockerP bitonal_bitmap_locker{ bitonal_bitmap_data };
            BitonalBitmapOperations::SetPixelValue(bitonal_bitmap_locker, bitonal_bitmap_data->GetSize(), x, y, value);
        }

        /// Sets the value of a specific pixel in a bitonal bitmap.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify (shared_ptr).
        /// \param  x                   The x-coordinate of the pixel (0-based).
        /// \param  y                   The y-coordinate of the pixel (0-based).
        /// \param  value               The value to write (true for set/1, false for clear/0).
        static void SetPixelValue(const std::shared_ptr<IBitonalBitmapData>& bitonal_bitmap_data, std::uint32_t x, std::uint32_t y, bool value)
        {
            BitonalBitmapOperations::SetPixelValue(bitonal_bitmap_data.get(), x, y, value);
        }

        /// Sets the value of a specific pixel in a locked bitonal bitmap.
        ///
        /// \param  lock_info   Information describing the locked bitonal bitmap data.
        /// \param  extent      The bitmap extent (width/height).
        /// \param  x           The x-coordinate of the pixel (0-based).
        /// \param  y           The y-coordinate of the pixel (0-based).
        /// \param  value       The value to write (true for set/1, false for clear/0).
        static void SetPixelValue(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y, bool value);

        /// Fills a rectangular region of interest (ROI) in a bitonal bitmap with a specified value.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify.
        /// \param  rect                The rectangle to fill. The ROI is clipped to the bitmap extent.
        /// \param  value               The value to fill the ROI with (true for set/1, false for clear/0).
        static void Fill(IBitonalBitmapData* bitonal_bitmap_data, const libCZI::IntRect& rect, bool value)
        {
            ScopedBitonalBitmapLockerP bitonal_bitmap_locker{ bitonal_bitmap_data };
            BitonalBitmapOperations::Fill(bitonal_bitmap_locker, bitonal_bitmap_data->GetSize(), rect, value);
        }

        /// Fills a rectangular region of interest (ROI) in a bitonal bitmap with a specified value.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify.
        /// \param  rect                The rectangle to fill. The ROI is clipped to the bitmap extent.
        /// \param  value               The value to fill the ROI with (true for set/1, false for clear/0).
        static void Fill(const std::shared_ptr<IBitonalBitmapData>& bitonal_bitmap_data, const libCZI::IntRect& rect, bool value)
        {
            BitonalBitmapOperations::Fill(bitonal_bitmap_data.get(), rect, value);
        }

        /// Fills a rectangular region of interest (ROI) in a bitonal bitmap with a specified value.
        ///
        /// \param 	lock_info	Information describing the locked bitonal bitmap data.
        /// \param 	extent  	The extent of the bitonal bitmap.
        /// \param 	roi		    The region of interest to fill. The ROI is clipped to the bitmap extent.
        /// \param 	value   	The value to fill the ROI with.
        static void Fill(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, const libCZI::IntRect& roi, bool value);

        /// Sets all pixels in the bitonal bitmap to a uniform value.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify (shared_ptr).
        /// \param  value               The value to assign to every pixel (true for set/1, false for clear/0).
        static void SetAllPixels(const std::shared_ptr<IBitonalBitmapData>& bitonal_bitmap_data, bool value)
        {
            BitonalBitmapOperations::SetAllPixels(bitonal_bitmap_data.get(), value);
        }

        /// Sets all pixels in the bitonal bitmap to a uniform value.
        ///
        /// This overload locks and unlocks the bitmap internally.
        ///
        /// \param  bitonal_bitmap_data The bitonal bitmap to modify.
        /// \param  value               The value to assign to every pixel (true for set/1, false for clear/0).
        static void SetAllPixels(IBitonalBitmapData* bitonal_bitmap_data, bool value)
        {
            ScopedBitonalBitmapLockerP bitonal_bitmap_locker{ bitonal_bitmap_data };
            SetAllPixels(bitonal_bitmap_locker, bitonal_bitmap_data->GetSize(), value);
        }

        /// Sets all pixels in a locked bitonal bitmap to a uniform value.
        ///
        /// \param  lock_info   Information describing the locked bitonal bitmap data.
        /// \param  extent      The bitmap extent (width/height).
        /// \param  value       The value to assign to every pixel (true for set/1, false for clear/0).
        static void SetAllPixels(const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent, bool value);

        /// Copies pixels from a source bitmap into a destination bitmap using a bitonal mask.
        ///
        /// For every mask pixel that is set (true), the corresponding source pixel is copied
        /// to destination at position (offset.x + x, offset.y + y), where (x, y) is the mask/source
        /// coordinate. Regions outside any involved extent are ignored.
        ///
        /// Preconditions and notes:
        /// - source_bitmap and destination_bitmap must have the same PixelType and compatible strides.
        /// - The effective copy area is the intersection of:
        ///   mask extent shifted by offset, destination extent, and source extent.
        /// - The function will lock/unlock the involved bitmaps internally as needed.
        ///
        /// \param  source_bitmap       Source image providing the pixels to copy.
        /// \param  mask                Bitonal mask controlling which pixels to copy.
        /// \param  offset              Destination offset where mask (0,0) maps to.
        /// \param  destination_bitmap  Destination image receiving copied pixels.
        static void CopyAt(libCZI::IBitmapData* source_bitmap, libCZI::IBitonalBitmapData* mask, const libCZI::IntPoint& offset, libCZI::IBitmapData* destination_bitmap);

        /// Decimates a bitonal bitmap - every second pixel is discarded in each direction. A pixel is set to 1
        /// in the decimated bitmap if all the pixels in the neighborhood in the source bitmap are set to 1. The neighborhood
        /// is specified by the neighborhood_size parameter, and is a square of size (2 x neighborhood_size + 1) x (2 x neighborhood_size + 1)
        /// around the pixel in question. The size of the decimated bitmap is half the size of the source bitmap (rounded down).
        ///
        /// \param 		   	neighborhood_size	Size of the neighborhood - this parameter must be in the range 0 to 7.
        ///                                     A value of 0 means only the center pixel is considered, while larger values
        ///                                     create a larger square neighborhood for the filtering operation.
        /// \param [in]	    source_bitmap	 	The source bitmap.
        ///
        /// \returns	The decimated bitmap.
        static std::shared_ptr<libCZI::IBitonalBitmapData> Decimate(int neighborhood_size, libCZI::IBitonalBitmapData* source_bitmap)
        {
            ScopedBitonalBitmapLockerP bitonal_bitmap_locker{ source_bitmap };
            return BitonalBitmapOperations::Decimate(neighborhood_size, bitonal_bitmap_locker, source_bitmap->GetSize());
        }

        /// Decimates a bitonal bitmap using pre-acquired lock information - every second pixel is discarded in each direction. 
        /// A pixel is set to 1 in the decimated bitmap if all the pixels in the neighborhood in the source bitmap are set to 1. 
        /// The neighborhood is specified by the neighborhood_size parameter, and is a square of size 
        /// (2 x neighborhood_size + 1) x (2 x neighborhood_size + 1) around the pixel in question. 
        /// The size of the decimated bitmap is half the size of the source bitmap (rounded down).
        ///
        /// This overload operates on pre-acquired lock information for optimal performance when the caller
        /// already holds a lock on the source bitmap.
        ///
        /// \param neighborhood_size    Size of the neighborhood - this parameter must be in the range 0 to 7.
        ///                             A value of 0 means only the center pixel is considered, while larger values
        ///                             create a larger square neighborhood for the filtering operation.
        /// \param lock_info            Information describing the locked bitonal bitmap data of the source bitmap.
        /// \param extent               The extent (width and height) of the source bitmap.
        ///
        /// \returns                   A new decimated bitonal bitmap with dimensions (extent.w/2, extent.h/2).
        ///                            The returned bitmap is a newly allocated IBitonalBitmapData object.
        static std::shared_ptr<libCZI::IBitonalBitmapData> Decimate(int neighborhood_size, const BitonalBitmapLockInfo& lock_info, const libCZI::IntSize& extent);
    };

    //-------------------------------------------------------------------------

    /// Stream insertion operator for the libCZI::IntRect type. A string of the form '(x,y,width,height)' is output to the stream.
    /// \param [in] os     The stream to output the rect to.
    /// \param rect        The rectangle.
    /// \return The ostream object <tt>os</tt>.
    inline std::ostream& operator<<(std::ostream& os, const IntRect& rect)
    {
        os << "(" << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << ")";
        return os;
    }

    /// Stream insertion operator for the libCZI::IntSize type. A string of the form '(width,height)' is output to the stream.
    /// \param [in] os     The stream to output the size to.
    /// \param size        The size structure.
    /// \return The ostream object <tt>os</tt>.
    inline std::ostream& operator<<(std::ostream& os, const IntSize& size)
    {
        os << "(" << size.w << "," << size.h << ")";
        return os;
    }
}
