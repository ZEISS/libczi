// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "ImportExport.h"
#include <cstring>
#include <limits>
#include <vector>
#include <memory>
#include "libCZI_Pixels.h"
#include "libCZI_Metadata.h"

namespace libCZI
{
    class IBitmapData;
    class IDimCoordinate;

    /// Values that represent the accessor types.
    enum class AccessorType
    {
        SingleChannelTileAccessor,              ///< The single-channel-tile accessor (associated interface: ISingleChannelTileAccessor).
        SingleChannelPyramidLayerTileAccessor,  ///< The single-channel-pyramidlayer-tile accessor (associated interface: ISingleChannelPyramidLayerTileAccessor).
        SingleChannelScalingTileAccessor        ///< The scaling-single-channel-tile accessor (associated interface: ISingleChannelScalingTileAccessor).
    };

    /// This interface defines how status information about the cache-state can be queried.
    class ISubBlockCacheStatistics
    {
    public:
        static constexpr std::uint8_t kMemoryUsage = 1;     ///< Bit-mask identifying the memory-usage field in the statistics struct.
        static constexpr std::uint8_t kElementsCount = 2;   ///< Bit-mask identifying the elements-count field in the statistics struct.

        /// This struct defines the statistics which can be queried from the cache. There is a bitfield which
        /// defines which elements are valid. If the bit is set, then the corresponding member is valid.
        struct Statistics
        {
            /// A bit mask which indicates which members are valid. C.f. the constants kMemoryUsage and kElementsCount.
            std::uint8_t validityMask;

            /// The memory usage of all elements in the cache. This field is only valid if the bit kMemoryUsage is set in the validityMask.
            std::uint64_t memoryUsage;

            /// The number of elements in the cache. This field is only valid if the bit kElementsCount is set in the validityMask.
            std::uint32_t elementsCount;
        };

        /// Gets momentarily valid statistics about the cache. The mask defines which statistic/s is/are to be retrieved.
        /// In case of multiple fields being requested, it is guaranteed that all requested fields are a transactional 
        /// snapshot of the state.
        ///
        /// \param  mask A bitmask specifying which fields are requested. Only the fields requested are guaranteed to be valid
        ///              in the returned struct.
        ///
        /// \returns A consistent snapshot of the statistics.
        virtual Statistics GetStatistics(std::uint8_t mask) const = 0;

        virtual ~ISubBlockCacheStatistics() = default;

        ISubBlockCacheStatistics() = default;
        ISubBlockCacheStatistics(const ISubBlockCacheStatistics&) = delete;
        ISubBlockCacheStatistics& operator=(const ISubBlockCacheStatistics&) = delete;
        ISubBlockCacheStatistics(ISubBlockCacheStatistics&&) noexcept = delete;
        ISubBlockCacheStatistics& operator=(ISubBlockCacheStatistics&&) noexcept = delete;
    };

    /// This interface defines the global operations on the cache. It is used to control the memory usage of the cache.
    class ISubBlockCacheControl
    {
    public:
        /// Options for controlling the prune operation. There are two metrics which can be used to control what
        /// remains in the cache and what is discarded: the maximum memory usage (for all elements in the cache) and 
        /// the maximum number of sub-blocks. If the cache exceeds one of those limits, then elements are evicted from the cache
        /// until both conditions are met. Eviction is done in the order starting with elements which have been least recently accessed.
        /// As "access" we define either the Add-operation or the Get-operation - so, when an element is retrieved from the
        /// cache, it is considered as "accessed".
        /// If only one condition is desired, then the other condition can be set to the maximum value of the respective type (which is the
        /// default value).
        struct PruneOptions
        {
            /// The maximum memory usage (in bytes) for the cache. If the cache exceeds this limit, 
            /// then the least recently used sub-blocks are removed from the cache.
            std::uint64_t   maxMemoryUsage{ (std::numeric_limits<decltype(maxMemoryUsage)>::max)() };

            /// The maximum number of sub-blocks in the cache. If the cache exceeds this limit,
            /// then the least recently used sub-blocks are removed from the cache.
            std::uint32_t  maxSubBlockCount{ (std::numeric_limits<decltype(maxSubBlockCount)>::max)() };
        };

        /// Prunes the cache. This means that sub-blocks are removed from the cache until the cache satisfies the conditions given in the options.
        /// Note that the prune operation is not done automatically - it must be called manually. I.e. when adding an element to the cache, the cache
        /// is **not** pruned automatically.
        /// \param  options Options for controlling the operation.
        virtual void Prune(const PruneOptions& options) = 0;

        virtual ~ISubBlockCacheControl() = default;

        ISubBlockCacheControl() = default;
        ISubBlockCacheControl(const ISubBlockCacheControl&) = delete;
        ISubBlockCacheControl& operator=(const ISubBlockCacheControl&) = delete;
        ISubBlockCacheControl(ISubBlockCacheControl&&) noexcept = delete;
        ISubBlockCacheControl& operator=(ISubBlockCacheControl&&) noexcept = delete;
    };

    /// This interface defines the operations of adding and querying an element to/from the cache.
    class ISubBlockCacheOperation
    {
    public:
        /// Gets the bitmap for the specified subblock-index. If the subblock is not in the cache, then a nullptr is returned.
        /// \param  subblock_index  The subblock index to get.
        /// \returns    If the subblock is in the cache, then a std::shared_ptr&lt;libCZI::IBitmapData&gt; is returned. Otherwise a nullptr is returned.
        virtual std::shared_ptr<IBitmapData> Get(int subblock_index) = 0;

        /// Adds the specified bitmap for the specified subblock_index to the cache. If the subblock is already in the cache, then it is overwritten.
        /// \param  subblock_index  The subblock index to add.
        /// \param  pBitmap         The bitmap.
        virtual void Add(int subblock_index, std::shared_ptr<IBitmapData> pBitmap) = 0;

        virtual ~ISubBlockCacheOperation() = default;

        ISubBlockCacheOperation() = default;
        ISubBlockCacheOperation(const ISubBlockCacheOperation&) = delete;
        ISubBlockCacheOperation& operator=(const ISubBlockCacheOperation&) = delete;
        ISubBlockCacheOperation(ISubBlockCacheOperation&&) noexcept = delete;
        ISubBlockCacheOperation& operator=(ISubBlockCacheOperation&&) noexcept = delete;
    };

    /// Interface for a caching component (which can be used with the compositors). The intended use is as follows:
    /// * Whenever the bitmap corresponding to a subblock (c.f. ISubBlock::CreateBitmap) is accessed, the bitmap may be added  
    ///   to a cache object, where the subblock-index is the key.
    /// * Whenever a bitmap is needed (for a given subblock-index), the cache object is first queried whether it contains the bitmap. If yes, then the bitmap  
    ///   returned may be used instead of executing the subblock-read-and-decode operation.
    /// In order to control the memory usage of the cache, the cache object must be pruned (i.e. subblocks are removed from the cache). Currently this means,
    /// that the Prune-method must be called manually. The cache object does not do any pruning automatically.
    /// The operations of Adding, Querying and Pruning the cache object are thread-safe.
    class ISubBlockCache : public ISubBlockCacheStatistics, public ISubBlockCacheControl, public ISubBlockCacheOperation
    {
    public:
        ~ISubBlockCache() override = default;

        ISubBlockCache() = default;
        ISubBlockCache(const ISubBlockCache&) = delete;
        ISubBlockCache& operator=(const ISubBlockCache&) = delete;
        ISubBlockCache(ISubBlockCache&&) noexcept = delete;
        ISubBlockCache& operator=(ISubBlockCache&&) noexcept = delete;
    };

    /// The base interface (all accessor-interface must derive from this).
    class IAccessor
    {
    protected:
        virtual ~IAccessor() = default;
    };

    /// This accessor creates a multi-tile composite of a single channel (and a single plane).
    /// The accessor will request all tiles that intersect with the specified ROI and are on
    /// the specified plane and create a composite as shown here:
    /// \image html SingleChannelTileAccessor_1.PNG
    /// \image latex SingleChannelTileAccessor_1.PNG 
    /// The resulting output bitmap will look like this:
    /// \image html SingleChannelTileAccessor_2.PNG
    /// \image latex SingleChannelTileAccessor_2.PNG
    /// This accessor only operates on pyramid layer 0 - i. e. only sub-blocks with logical_size = physical_size
    /// will be considered. If the flag "drawTileBorder" is set, then the tiles will be sorted by their M-index
    /// (tiles with higher M-index are placed 'on top').\n
    /// The pixel type of the output bitmap is either specified as an argument or it is automatically
    /// determined. In the latter case the first sub-block found on the specified plane is examined for its
    /// pixeltype, and this pixeltype is used.\n
    /// The pixels in the output bitmap get converted from the source pixels (if their pixeltypes differ).
    class ISingleChannelTileAccessor : public IAccessor
    {
    public:
        /// Options for controlling the composition operation.
        struct Options
        {
            /// The back ground color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
            /// with the maximum pixel value (of the specific pixeltype). If it is a RGB-color type, then R, G and B are separately multiplied with
            /// the maximum pixel value.
            /// If any of R, G or B is NaN, then the background is not cleared.
            RgbFloatColor   backGroundColor;

            /// If true, then the tiles are sorted by their M-index (tile with highest M-index will be 'on top').
            /// Otherwise the Z-order is arbitrary.
            bool sortByM;

            /// If true, then the tile-visibility-check-optimization is used. When doing the multi-tile composition,
            /// all relevant tiles are checked whether they are visible in the destination bitmap. If a tile is not visible, then
            /// the corresponding sub-block is not read. This can speed up the operation considerably. The result is the same as
            /// without this optimization - i.e. there should be no reason to turn it off besides potential bugs.
            bool useVisibilityCheckOptimization;

            /// If true, then a one-pixel wide boundary will be drawn around 
            /// each tile (in black color).
            bool drawTileBorder;

            /// If specified, only subblocks with a scene-index contained in the set will be considered.
            std::shared_ptr<libCZI::IIndexSet> sceneFilter;

            /// If specified, then the sub-block cache is used. This can speed up the operation considerably.
            /// Bitmaps that are read by the accessor are added to the cache. If a bitmap is needed which is already 
            /// in the cache, then the bitmap from the cache is used instead of reading the sub-block from the file.
            std::shared_ptr<libCZI::ISubBlockCacheOperation> subBlockCache;

            /// If true, then only bitmaps from sub-blocks with compressed data are added to the cache. For uncompressed
            /// data, the time for reading the bitmap can be negligible, so the benefit of caching might not outweigh the
            /// increased memory usage.
            bool onlyUseSubBlockCacheForCompressedData;

            /// Clears this object to its blank state.
            void Clear()
            {
                this->backGroundColor.r = this->backGroundColor.g = this->backGroundColor.b = std::numeric_limits<float>::quiet_NaN();
                this->sortByM = true;
                this->useVisibilityCheckOptimization = false;
                this->drawTileBorder = false;
                this->sceneFilter.reset();
                this->subBlockCache.reset();
                this->onlyUseSubBlockCacheForCompressedData = true;
            }
        };

    public:
        /// <summary>   Gets the tile composite of the specified plane and the specified ROI. 
        ///             The pixeltype is determined by examing the first subblock found in the
        ///             specified plane (which is an arbitrary subblock). A newly allocated
        ///             bitmap is returned.
        /// </summary>
        /// <remarks>   
        ///             It needs to be defined what is supposed to happen if there is no subblock
        ///             found in the specified plane.
        /// </remarks>
        /// <param name="roi">              The ROI. </param>
        /// <param name="planeCoordinate">  The plane coordinate. </param>
        /// <param name="pOptions">         Options for controlling the operation. </param>
        /// <returns>   A std::shared_ptr&lt;libCZI::IBitmapData&gt; containing the tile-composite.</returns>
        virtual std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const IDimCoordinate* planeCoordinate, const Options* pOptions) = 0;

        /// Gets the tile composite of the specified plane and the specified ROI. 
        ///
        /// \param pixeltype       The pixeltype.
        /// \param roi             The ROI.
        /// \param planeCoordinate The plane coordinate.
        /// \param pOptions        Options for controlling the operation.
        ///
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt; containing the tile-composite.
        virtual std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const IDimCoordinate* planeCoordinate, const Options* pOptions) = 0;

        /// Copy the tile composite into the specified bitmap. The bitmap passed in here determines the width and the height of the ROI
        /// (and the pixeltype).
        ///
        /// \param [in] pDest      The destination bitmap.
        /// \param xPos            The x-position of the ROI (width and height are given by pDest).
        /// \param yPos            The y-position of the ROI ((width and height are given by pDest).
        /// \param planeCoordinate The plane coordinate.
        /// \param pOptions        Options for controlling the operation.
        virtual void Get(libCZI::IBitmapData* pDest, int xPos, int yPos, const IDimCoordinate* planeCoordinate, const Options* pOptions) = 0;

        /// Gets the tile composite of the specified plane and the specified ROI.
        /// The pixeltype is determined by examing the first subblock found in the
        /// specified plane (which is an arbitrary subblock). A newly allocated
        /// bitmap is returned.
        /// \param xPos            The x-position.
        /// \param yPos            The y-position.
        /// \param width           The width.
        /// \param height          The height.
        /// \param planeCoordinate The plane coordinate.
        /// \param pOptions        Options for controlling the operation.
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        inline std::shared_ptr<libCZI::IBitmapData> Get(int xPos, int yPos, int width, int height, const IDimCoordinate* planeCoordinate, const Options* pOptions) { return this->Get(libCZI::IntRect{ xPos,yPos,width,height }, planeCoordinate, pOptions); }

        /// Gets the tile composite of the specified plane and the specified ROI.
        /// \param pixeltype       The pixeltype.
        /// \param xPos            The x-position.
        /// \param yPos            The y-position.
        /// \param width           The width.
        /// \param height          The height.
        /// \param planeCoordinate The plane coordinate.
        /// \param pOptions        Options for controlling the operation.
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        inline std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, int xPos, int yPos, int width, int height, const IDimCoordinate* planeCoordinate, const Options* pOptions) { return this->Get(pixeltype, libCZI::IntRect{ xPos,yPos,width,height }, planeCoordinate, pOptions); }
    protected:
        virtual ~ISingleChannelTileAccessor() = default;
    };

    /// Interface for single-channel-pyramidlayer tile accessors. 
    /// It allows to directly address the pyramid-layer it operates on.
    /// This accessor creates a multi-tile composite of a single channel (and a single plane) from a specified pyramid-layer.
    class ISingleChannelPyramidLayerTileAccessor : public IAccessor
    {
    public:
        /// Options used for this accessor.
        struct Options
        {
            /// The back ground color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
            /// with the maximum pixel value (of the specific pixeltype). If it is a RGB-color type, then R, G and B are separately multiplied with
            /// the maximum pixel value.
            /// If any of R, G or B is NaN, then the background is not cleared.
            RgbFloatColor   backGroundColor;

            /// If true, then the tiles are sorted by their M-index (tile with highest M-index will be 'on top').
            /// Otherwise the Z-order is arbitrary.
            bool sortByM;

            /// If true, then a one-pixel wide boundary will be drawn around 
            /// each tile (in black color).
            bool drawTileBorder;

            /// If specified, only subblocks with a scene-index contained in the set will be considered.
            std::shared_ptr<libCZI::IIndexSet> sceneFilter;

            /// If specified, then the sub-block cache is used. This can speed up the operation considerably.
            /// Bitmaps that are read by the accessor are added to the cache. If a bitmap is needed which is already 
            /// in the cache, then the bitmap from the cache is used instead of reading the sub-block from the file.
            std::shared_ptr<libCZI::ISubBlockCacheOperation> subBlockCache;

            /// If true, then only bitmaps from sub-blocks with compressed data are added to the cache.
            bool onlyUseSubBlockCacheForCompressedData;

            /// Clears this object to its blank state.
            void Clear()
            {
                this->drawTileBorder = false;
                this->sortByM = true;
                this->backGroundColor.r = this->backGroundColor.g = this->backGroundColor.b = std::numeric_limits<float>::quiet_NaN();
                this->sceneFilter.reset();
                this->subBlockCache.reset();
                this->onlyUseSubBlockCacheForCompressedData = true;
            }
        };

        /// Information about the pyramid-layer.
        /// It consists of two parts: the minification factor and the layer number.
        /// The minification factor specifies by which factor two adjacent pyramid-layers are shrunk. Commonly used in
        /// CZI are 2 or 3.
        /// The layer number starts with 0 with the highest resolution layer.
        struct PyramidLayerInfo
        {
            std::uint8_t minificationFactor;    ///< Factor by which adjacent pyramid-layers are shrunk. Commonly used in CZI are 2 or 3.
            std::uint8_t pyramidLayerNo;        ///< The pyramid layer number.
        };

        /// Gets the tile composite of the specified plane and the specified ROI and the specified pyramid-layer.
        /// The pixeltype is determined by examining the first subblock found in the
        /// specified plane (which is an arbitrary subblock). A newly allocated
        /// bitmap is returned.
        /// \param roi             The ROI.
        /// \param planeCoordinate The plane coordinate.
        /// \param pyramidInfo     Information describing the pyramid-layer.
        /// \param pOptions        Options for controlling the operation.
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        virtual std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const libCZI::ISingleChannelPyramidLayerTileAccessor::Options* pOptions) = 0;

        /// Gets the tile composite of the specified plane and the specified ROI and the specified pyramid-layer.
        /// \param pixeltype       The pixeltype (of the destination bitmap).
        /// \param roi             The ROI.
        /// \param planeCoordinate The plane coordinate.
        /// \param pyramidInfo     Information describing the pyramid-layer.
        /// \param pOptions        Options for controlling the operation.
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        virtual std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const libCZI::ISingleChannelPyramidLayerTileAccessor::Options* pOptions) = 0;

        /// <summary>   Copy the composite to the specified bitmap. </summary>
        /// \param [out] pDest     The destination bitmap (also defining the width and height)
        /// \param xPos            The x-position.
        /// \param yPos            The y-position.
        /// \param planeCoordinate The plane coordinate.
        /// \param pyramidInfo     Information describing the pyramid-layer.
        /// \param pOptions        Options for controlling the operation.
        virtual void Get(libCZI::IBitmapData* pDest, int xPos, int yPos, const IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options* pOptions) = 0;
    };

    /// Interface for single channel scaling tile accessors.
    /// This accessor creates a multi-tile composite of a single channel (and a single plane) with a given zoom-factor.
    /// It will use pyramid sub-blocks (if present) in order to create the destination bitmap. In this operation, it will use
    /// the pyramid-layer just above the specified zoom-factor and scale down to the requested size.\n
    /// The scaling operation employed here is a simple nearest-neighbor algorithm.
    class ISingleChannelScalingTileAccessor : public IAccessor
    {
    public:
        /// Options used for this accessor.
        struct Options
        {
            /// The back ground color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
            /// with the maximum pixel value (of the specific pixeltype). If it is a RGB-color type, then R, G and B are separately multiplied with
            /// the maximum pixel value.
            /// If any of R, G or B is NaN, then the background is not cleared.
            RgbFloatColor backGroundColor;

            /// If true, then the tiles are sorted by their M-index (tile with highest M-index will be 'on top').
            /// Otherwise the Z-order is arbitrary.
            bool sortByM;

            /// If true, then a one-pixel wide boundary will be drawn around 
            /// each tile (in black color).
            bool drawTileBorder;

            /// If specified, only subblocks with a scene-index contained in the set will be considered. If an empty shared_ptr
            /// is given here, then no filtering is applied.
            std::shared_ptr<libCZI::IIndexSet> sceneFilter;

            /// If true, then the tile-visibility-check-optimization is used. When doing the multi-tile composition,
            /// all relevant tiles are checked whether they are visible in the destination bitmap. If a tile is not visible, then
            /// the corresponding sub-block is not read. This can speed up the operation considerably. The result is the same as
            /// without this optimization - i.e. there should be no reason to turn it off besides potential bugs.
            bool useVisibilityCheckOptimization;

            /// If specified, then the sub-block cache is used. This can speed up the operation considerably.
            /// Bitmaps that are read by the accessor are added to the cache. If a bitmap is needed which is already 
            /// in the cache, then the bitmap from the cache is used instead of reading the sub-block from the file.
            std::shared_ptr<libCZI::ISubBlockCacheOperation> subBlockCache;

            /// If true, then only bitmaps from sub-blocks with compressed data are added to the cache.
            bool onlyUseSubBlockCacheForCompressedData;

            /// Clears this object to its blank state.
            void Clear()
            {
                this->drawTileBorder = false;
                this->sortByM = true;
                this->backGroundColor.r = this->backGroundColor.g = this->backGroundColor.b = std::numeric_limits<float>::quiet_NaN();
                this->sceneFilter.reset();
                this->useVisibilityCheckOptimization = false;
                this->subBlockCache.reset();
                this->onlyUseSubBlockCacheForCompressedData = true;
            }
        };

        /// Calculates the size a bitmap will have (when created by this accessor) for the specified ROI and the specified Zoom.
        /// Since the exact size if subject to rounding errors, one should always use this method if the exact size must be known beforehand.
        /// The Get-method which operates on a pre-allocated bitmap will only work if the size (of the bitmap passed in) exactly matches.
        /// \param roi  The ROI.
        /// \param zoom The zoom factor.
        /// \return The size of the composite created by this accessor (for these parameters).
        virtual libCZI::IntSize CalcSize(const libCZI::IntRect& roi, float zoom) const = 0;

        /// Gets the scaled tile composite of the specified plane and the specified ROI with the specified zoom factor.\n
        /// The pixeltype is determined by examining the first subblock found in the
        /// specified plane (which is an arbitrary subblock). A newly allocated
        /// bitmap is returned.
        /// \param roi             The ROI.
        /// \param planeCoordinate The plane coordinate.
        /// \param zoom            The zoom.
        /// \param pOptions        Options for controlling the operation (may be nullptr).
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        virtual std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) = 0;

        /// Gets the scaled tile composite of the specified plane and the specified ROI with the specified zoom factor.
        /// \param pixeltype       The pixeltype (of the destination bitmap).
        /// \param roi             The ROI.
        /// \param planeCoordinate The plane coordinate.
        /// \param zoom            The zoom factor.
        /// \param pOptions        Options for controlling the operation (may be nullptr).
        /// \return A std::shared_ptr&lt;libCZI::IBitmapData&gt;
        virtual std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) = 0;

        /// <summary>   Copy the composite to the specified bitmap. </summary>
        /// <remarks>   The size of the bitmap must exactly match the size reported by the method "CalcSize" (for the same ROI and zoom),
        ///             otherwise an invalid_argument-exception is thrown. </remarks>
        /// <param name="pDest">            [in,out] The destination bitmap. </param>
        /// <param name="roi">              The ROI. </param>
        /// <param name="planeCoordinate">  The plane coordinate. </param>
        /// <param name="zoom">             The zoom factor. </param>
        /// <param name="pOptions">         Options controlling the operation. May be nullptr.</param>
        virtual void Get(libCZI::IBitmapData* pDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) = 0;
    };

    /// Composition operations are found in this class: multi-tile compositor and multi-channel compositor.
    class LIBCZI_API Compositors
    {
    public:
        /// Options for the libCZI::Compositors::ComposeSingleChannelTiles function.
        struct LIBCZI_API ComposeSingleTileOptions
        {
            /// If true, then a one-pixel wide boundary will be drawn around 
            /// each tile (in black color).
            bool drawTileBorder;

            /// Clears this object to its blank/initial state.
            void Clear() { this->drawTileBorder = false; }
        };

        /// <summary>
        /// Composes a set of tiles (which are retrieved by calling the getTiles-functor) in the
        /// following way: The destination bitmap is taken to be positioned at (xPos,yPos) - which
        /// specifies the top-left corner. The tiles (retrieved from the functor) are positioned at the
        /// coordinate as reported by the functor-call. Then the intersection area of source and
        /// destination is copied to the destination bitmap. If the intersection is empty, then nothing
        /// is copied.
        /// </summary>
        /// <param name="getTiles">
        /// [in] The functor which is called in order to retrieve the tiles to compose. The second and
        /// the third parameter specify the x- and y-position of this tile.
        /// We address a tile with the parameter index. If the index is out-of-range, then this functor
        /// is expected to return false.
        /// </param>
        /// <param name="dest">     [in,out] The destination bitmap. </param>
        /// <param name="xPos">     The x-coordinate of the top-left of the destination bitmap. </param>
        /// <param name="yPos">     The y-coordinate of the top-left of the destination bitmap. </param>
        /// <param name="pOptions"> Options for controlling the operation. This argument is optional (may be nullptr).</param>
        static void ComposeSingleChannelTiles(
            const std::function<bool(int index, std::shared_ptr<libCZI::IBitmapData>& src, int& x, int& y)>& getTiles,
            libCZI::IBitmapData* dest,
            int xPos,
            int yPos,
            const ComposeSingleTileOptions* pOptions);

        /// This structure defines the tinting color.
        struct TintingColor
        {
            /// The tinting color to be used given as RGB24.
            Rgb8Color   color;
        };


        /// Information about a channel for use in the multi-channel-composition operation.
        /// The gradation to be applied can be specified in two ways: either the black-point and
        /// white-point is provided, and the gradation curve is a straight line (between black-point
        /// and white-point) or a look-up table is used. In case of a look-up table being specified,
        /// black-point/white-point is not used. The size of the look-up table must match exactly the 
        /// bits in this channels, so far a Gray8/Bgr24 it must be of size 256 and for Gray16/Bgr48
        /// is must be of size 65536.
        struct ChannelInfo
        {
            /// The weight of the channel.
            float        weight;

            /// True if tinting is enabled for this channel (in which case the tinting member is to be 
            /// examined), false if no tinting is to be applied (the tinting member is then not used).
            bool         enableTinting;

            /// The tinting color (only examined if enableTinting is true).
            TintingColor tinting;

            /// The black point - it is a float between 0 and 1, where 0 corresponds to the lowest pixel value
            /// (of the pixeltype for the channel) and 1 to the highest pixel value (of the pixeltype of this channel).
            /// All pixel values below the black point are mapped to 0.
            float        blackPoint;

            /// The white point - it is a float between 0 and 1, where 0 corresponds to the lowest pixel value
            /// (of the pixeltype for the channel) and 1 to the highest pixel value (of the pixeltype of this channel).
            /// All pixel value above the white pointer are mapped to the highest pixel value.
            float        whitePoint;

            /// Number of elements in the look-up table. If 0, then the look-up table
            /// is not used. If this channelInfo applies to a Gray8/Bgr24-channel, then the size
            /// of the look-up table must be 256. In case of a Gray16/Bgr48-channel, the size must be
            /// 65536.
            /// \remark
            /// If a look-up table is provided, then \c blackPoint and \c whitePoint are not used any more.
            int         lookUpTableElementCount;

            /// The pointer to the look-up table. If lookUpTableElementCount is <> 0, then this pointer
            /// must be valid.
            const std::uint8_t* ptrLookUpTable;

            /// All members are set to zero.
            void Clear() { std::memset(this, 0, sizeof(*this)); }
        };

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to the specified destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap also
        /// has to have this same width/height. The pixeltype of the destination bitmap must be
        /// Bgr24.
        ///
        /// \param [in] dest     The destination bitmap - must have same width/height as the source bitmaps and must be Bgr24.
        /// \param channelCount  The number of channels. 
        /// \param srcBitmaps    An array of source bitmaps. The array must contain as many elements as specified by \c channelCount.
        /// \param channelInfos  An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        static void ComposeMultiChannel_Bgr24(
            libCZI::IBitmapData* dest,
            int channelCount,
            libCZI::IBitmapData* const* srcBitmaps,
            const ChannelInfo* channelInfos);

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to the specified destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap also
        /// has to have this same width/height. The pixeltype of the destination bitmap must be
        /// Bgra32. The value of the parameter 'alphaVal' is written to all alpha-pixels in the
        /// destination.
        ///
        /// \param [in] dest        The destination bitmap - must have same width/height as the source bitmaps and must be Bgra32.
        /// \param alphaVal         The alpha value.
        /// \param channelCount     The number of channels.
        /// \param srcBitmaps       An array of source bitmaps. The array must contain as many elements as specified by \c channelCount.
        /// \param channelInfos     An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        static void ComposeMultiChannel_Bgra32(
            libCZI::IBitmapData* dest,
            std::uint8_t alphaVal,
            int channelCount,
            libCZI::IBitmapData* const* srcBitmaps,
            const ChannelInfo* channelInfos);

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to a newly allocated destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap will also
        /// have this same width/height. The pixeltype of the destination bitmap will be
        /// Bgr24.
        ///
        /// \param channelCount  The number of channels. 
        /// \param srcBitmaps    An array of source bitmaps. The array must contain as many elements as specified by \c channelCount.
        /// \param channelInfos  An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        ///
        ///  \return A std::shared_ptr&lt;IBitmapData&gt;.
        static std::shared_ptr<IBitmapData> ComposeMultiChannel_Bgr24(
            int channelCount,
            libCZI::IBitmapData* const* srcBitmaps,
            const ChannelInfo* channelInfos);

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to a newly allocated destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap will also
        /// have this same width/height. The pixeltype of the destination bitmap will be
        /// Bgra32, and each alpha-pixel-value will be set to 'alphaVal'.
        ///
        /// \param alphaVal     The alpha value.
        /// \param channelCount The number of channels.
        /// \param srcBitmaps   An array of source bitmaps. The array must contain as many elements as specified by \c channelCount.
        /// \param channelInfos An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        ///
        /// \return A std::shared_ptr&lt;IBitmapData&gt;.
        static std::shared_ptr<IBitmapData> ComposeMultiChannel_Bgra32(
            std::uint8_t alphaVal,
            int channelCount,
            libCZI::IBitmapData* const* srcBitmaps,
            const ChannelInfo* channelInfos);

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to the specified destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap also
        /// has to have this same width/height. The pixeltype of the destination bitmap must be
        /// Bgr24.
        ///
        /// \param channelCount       Number of channels.
        /// \param srcBitmapsIterator Source bitmaps iterator.
        /// \param channelInfos An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        /// \return A std::shared_ptr&lt;IBitmapData&gt;.
        static std::shared_ptr<IBitmapData> ComposeMultiChannel_Bgr24(
            int channelCount,
            std::vector<std::shared_ptr<libCZI::IBitmapData>>::iterator srcBitmapsIterator,
            const ChannelInfo* channelInfos)
        {
            std::vector<IBitmapData*> vecBm; vecBm.reserve(channelCount);
            for (int i = 0; i < channelCount; ++i)
            {
                vecBm.emplace_back((*srcBitmapsIterator).get());
                ++srcBitmapsIterator;
            }

            return ComposeMultiChannel_Bgr24(channelCount, &vecBm[0], channelInfos);
        }

        /// Create the multi-channel-composite - applying tinting or gradation to the specified
        /// bitmaps and write the result to the specified destination bitmap.
        /// All source bitmaps must have same width and height, and the destination bitmap also
        /// has to have this same width/height. The pixeltype of the destination bitmap must be
        /// Bgra32.
        ///
        /// \param alphaVal           The alpha value.
        /// \param channelCount       Number of channels.
        /// \param srcBitmapsIterator Source bitmaps iterator.
        /// \param channelInfos       An array of \c channelInfo for the source channels. The array must contain as many elements as specified by \c channelCount.
        /// \return A std::shared_ptr&lt;IBitmapData&gt;.
        static std::shared_ptr<IBitmapData> ComposeMultiChannel_Bgra32(
            std::uint8_t alphaVal,
            int channelCount,
            std::vector<std::shared_ptr<libCZI::IBitmapData>>::iterator srcBitmapsIterator,
            const ChannelInfo* channelInfos)
        {
            std::vector<IBitmapData*> vecBm; vecBm.reserve(channelCount);
            for (int i = 0; i < channelCount; ++i)
            {
                vecBm.emplace_back((*srcBitmapsIterator).get());
                ++srcBitmapsIterator;
            }

            return ComposeMultiChannel_Bgra32(alphaVal, channelCount, &vecBm[0], channelInfos);
        }
    };
}
