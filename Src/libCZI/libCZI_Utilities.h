// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Metadata.h"
#include "libCZI_Pixels.h"

namespace libCZI
{
	class ISubBlockRepository;
	class IDisplaySettings;
	class ICompressParameters;

	/// A bunch of utility functions.
	class LIBCZI_API Utils
	{
	public:
		/// Convert the specifed dimension enum to the corresponding "single char representation". The returned
		/// character will be uppercase. If the specified dimension enum cannot be converted, '?' is returned.
		///
		/// \param dim The dimension enum.
		///
		/// \return A character representing the specified dimension.
		static char DimensionToChar(libCZI::DimensionIndex dim);

		/// Convert the specifed single character to the corresponding dimension enum. The single character
		/// may be given uppercase or lowercase. In case that no corresponding dimension enum exists,
		/// `DimensionIndex::invalid` is returned.
		///
		/// \param c The "single char representation" of a dimension.
		///
		/// \return A enum value representing the specified dimension if it exists, `DimensionIndex::invalid` otherwise.
		static libCZI::DimensionIndex CharToDimension(char c);

		/// Calculates the MD5SUM hash for the pixels in the specified bitmap.
		/// \param [in] bm	    The bitmap.
		/// \param [in,out] ptrHash Pointer to the hash-code result. The result will be of size 16 bytes.
		/// \param hashSize		    Size of the hash-code result pointed to by <tt>ptrHash</tt>. We need 16 bytes.
		/// \return The count of bytes that were written to in ptrHash as the MD5SUM-hash (always 16).
		static int CalcMd5SumHash(libCZI::IBitmapData* bm, std::uint8_t* ptrHash, int hashSize);

		/// Calculates the MD5SUM hash for the specified data.
		/// \param [in] ptrData	    Pointer to the data (for which to calculate the MD5SUM-hash).
		/// \param [in] sizeData	The size of the data (pointed to by ptrData).
		/// \param [in,out] ptrHash Pointer to the hash-code result. The result will be of size 16 bytes.
		/// \param hashSize		    Size of the hash-code result pointed to by <tt>ptrHash</tt>. We need 16 bytes.
		/// \return The count of bytes that were written to in ptrHash as the MD5SUM-hash (always 16).
		static int CalcMd5SumHash(const void* ptrData, size_t sizeData, std::uint8_t* ptrHash, int hashSize);

		/// Creates an 8-bit look-up table from the specifed splines.
		/// A spline is sampled between \c blackPoint and \c whitePoint (i. e. points left of \c blackPoint are set to 0
		/// and right of \c whitePoint are set to 1). 
		/// \param tableElementCnt Number of points to sample - the result will have as many samples as specified here.
		/// \param blackPoint	   The black point.
		/// \param whitePoint	   The white point.
		/// \param splineData	   Information describing the spline.
		/// \return The new 8-bit look up table generated from the spline. The number is elements is as specified by <tt>tableElementCount</tt>.
		static std::vector<std::uint8_t> Create8BitLookUpTableFromSplines(int tableElementCnt, float blackPoint, float whitePoint, const std::vector<libCZI::IDisplaySettings::SplineData>& splineData);

		/// Creates 8-bit look-up table from the specified gamma value.
		/// An exponential with the specified gamma is sampled between \c blackPoint and \c whitePoint (i. e. points left of \c blackPoint are set to 0
		/// and right of \c whitePoint are set to 1).
		/// \param tableElementCnt Number of points to sample - the result will have as many samples as specified here.
		/// \param blackPoint	   The black point.
		/// \param whitePoint	   The white point.
		/// \param gamma		   The gamma.
		///
		/// \return The new 8-bit look up table generated from the spline. The number is elements is as specified by <tt>tableElementCount</tt>.
		static std::vector<std::uint8_t> Create8BitLookUpTableFromGamma(int tableElementCnt, float blackPoint, float whitePoint, float gamma);

		/// Calculates the spline coefficients from a list of control points.
		/// \param pointCnt Number of control points.
		/// \param getPoint A functor which will be used to retrieve the control point's coordinates.
		/// \return The calculated spline data from the specified control points.
		static std::vector<libCZI::IDisplaySettings::SplineData> CalcSplineDataFromPoints(int pointCnt, std::function< std::tuple<double, double>(int idx)> getPoint);

		/// Resize the specified bitmap to the specified width and height. This method employs a nearest-neighbor-scaling algorihm.
		/// \param [in] bmSrc The source bitmap.
		/// \param dstWidth		  Width of the destination.
		/// \param dstHeight	  Height of the destination.
		/// \return A std::shared_ptr&lt;libCZI::IBitmapData &gt; containing the scaled bitmap.
		static std::shared_ptr<libCZI::IBitmapData > NearestNeighborResize(libCZI::IBitmapData* bmSrc, int dstWidth, int dstHeight);

		/// Resize a ROI from the specified bitmap to the specified width and height. This method employs a nearest-neighbor-scaling algorihm.
		/// \param [in] bmSrc	  The source bitmap.
		/// \param dstWidth		  Width of the destination.
		/// \param dstHeight	  Height of the destination.
		/// \param roiSrc		  The ROI (in the source bitmap).
		/// \param roiDest		  The ROI (in the destination bitmap)
		/// \return A std::shared_ptr&lt;libCZI::IBitmapData &gt;
		static std::shared_ptr<libCZI::IBitmapData > NearestNeighborResize(libCZI::IBitmapData* bmSrc, int dstWidth, int dstHeight, const DblRect& roiSrc, const DblRect& roiDest);

		//// Calculate a zoom-factor from the physical- and logical size.
		/// \remark
		/// This calculation not really well-defined.
		/// \param logicalRect  The logical rectangle.
		/// \param physicalSize Physical size.
		/// \return The calculated zoom.
		static float CalcZoom(const libCZI::IntRect& logicalRect, const libCZI::IntSize& physicalSize)
		{
			if (physicalSize.w > physicalSize.h)
			{
				return static_cast<float>(physicalSize.w) / logicalRect.w;
			}
			else
			{
				return static_cast<float>(physicalSize.h) / logicalRect.h;
			}
		}

		//// Calculate a zoom-factor from the physical- and logical size.
		/// \remark
		/// This calculation not really well-defined.
		/// \param logicalSize  The logical size.
		/// \param physicalSize The physical size.
		/// \return The calculated zoom.
		static float CalcZoom(const libCZI::IntSize& logicalSize, const libCZI::IntSize& physicalSize)
		{
			if (physicalSize.w > physicalSize.h)
			{
				return static_cast<float>(physicalSize.w) / logicalSize.w;
			}
			else
			{
				return static_cast<float>(physicalSize.h) / logicalSize.h;
			}
		}

		/// Retrieves an informal string representing the specified pixeltype. 
		///
		/// \param pixeltype The pixel-type.
		///
		/// \return A pointer to a static string. Will always be non-null (even in case of an invalid value for <tt>pixeltype</tt>.
		static const char* PixelTypeToInformalString(libCZI::PixelType pixeltype);

        /// Gets the number of bytes which represent a pixel. In case of an invalid pixelType-argument, and invalid_argument is thrown.
        /// \param pixelType The pixel type.
        /// \returns The number of bytes which represent a pixel.
		static std::uint8_t GetBytesPerPixel(libCZI::PixelType pixelType);

		/// Retrieves an informal string representing the specified compression mode. 
		/// The string representation returned here is suitable for use with the compression-options
		/// parsing function (ParseCompressionOptions).
		///
		/// \param compressionMode The compression mode.
		///
		/// \return A pointer to a static string. Will always be non-null (even in case of an invalid value for <tt>compressionMode</tt>.
		static const char* CompressionModeToInformalString(libCZI::CompressionMode compressionMode);

		/// Get a string representation of the specified coordinate.
		/// \param coord The coordinate.
		/// \return A string representation of the specified coordinate.
		static std::string DimCoordinateToString(const libCZI::IDimCoordinate* coord);

		/// Convert the specified string into a dimension-coordinate instance.
		/// \param 		    sz    The string to convert.
		/// \param [out]	coord If non-null and if the parsing was successful, the information will be put here.
		/// \returns True if the string parsed successfully, false otherwise.
		static bool StringToDimCoordinate(const char* sz, libCZI::CDimCoordinate* coord);

		/// Get a string representation of the specified bounds.
		/// \param bounds The bounds.
		/// \return A string representation of the specified bounds.
		static std::string DimBoundsToString(const libCZI::IDimBounds* bounds);

		/// Create an index-set object from a string representation. The string is a list of intervals,
		/// seperated by comma (','). It can be of the form "5", "17", "3-5", "-3-5". The string
		/// "inf" (meaning 'infinity') is recognized in order to express "all numbers up to" or "all numbers after"
		/// , e. g. "-inf-4" or "5-inf".
		/// In case of an invalid string, an LibCZIStringParseException exception is thrown.
		/// \param s The string to convert to an index-set object.
		/// \return A newly create std::shared_ptr&lt;libCZI::IIndexSet&gt; object.
		static std::shared_ptr<libCZI::IIndexSet> IndexSetFromString(const std::wstring& s);

		/// Try to determine the pixel type for channel. This is done by looking at an (arbitrary) subblock within the specified
		/// channel. There are cases where this does not yield a result - e. g. if there is no subblock present
		/// with the specified channel-index.
		/// \param [in] repository	   The CZI-document.
		/// \param channelIdx	       The channel index.
		///
		/// \return The pixeltype if it can be determined. If it cannot be determined reliably (e.g. there is no subblock with
		/// 		the specified channel-index), then PixelType::Invalid is returned.
		static libCZI::PixelType TryDeterminePixelTypeForChannel(libCZI::ISubBlockRepository* repository, int channelIdx);

		/// Compares two coordinate-objects to determine their relative ordering.
		/// The algorithm employed is: we check for all coordinates which are marked valid in a or b (in the order of the numerical value or the enum)...
		/// 1. coordinateA(dim) is valid and coordinateB(dim) is invalid -> a > b  
		/// 2. coordinateA(dim) is invalid and coordinateB(dim) is valid -> a < b  
		/// 3. if both are valid, then the coordinate-values are determined and compared
		///
		/// \param a First coordinate to be compared.
		/// \param b Second coordinate to be compared.
		///
		/// \return Negative if 'a' is less than 'b', 0 if they are equal, or positive if it is greater.
		static int Compare(const IDimCoordinate* a, const IDimCoordinate* b);

		/// Test whether the two specified coordinates have the same set of valid dimensions.
		///
		/// \param a The first coordinate-object to compare.
		/// \param b The second coordinate-object to compare.
		///
		/// \return True if the two coordinates have the same set of valid dimensions, false otherwise.
		static bool HasSameDimensions(const IDimCoordinate* a, const IDimCoordinate* b);

		/// Creates a metadata-builder object suitable for generating sub-block metadata. It generates XML in the form
		/// \code{.unparsed}
		/// <METADATA>
		///		<Tags>
		///			<StageXPosition>-8906.346</StageXPosition>
		///			<StageYPosition>-648.51</StageYPosition>
		///		</Tags>
		///	</METADATA>
		/// \endcode
		/// The specified function can be used to give a set of nodename-value-pairs - which are added under the "Tags"-node.
		///
		/// \param [in] tagsEnum Optionally, a function which is called to provide a pair of strings, where the first element
		/// 			gives the node-name and the second the value. The first integer argument counts the number of calls made
		/// 			to this function. The function has to return true, if it provided valid information and it will be called
		/// 			again. If it returns false, it will not be called again.
		///
		/// \return The newly created metadata-builder object.
		static std::shared_ptr<ICziMetadataBuilder> CreateSubBlockMetadata(const std::function<bool(int, std::tuple<std::string, std::string>&)>& tagsEnum = nullptr);

		/// Enumerate all coordinates "contained" in the specified bounds. The specified function is called with all valid coordinates
		/// (which lie inside the bounds). The first argument to the function is a counter which increments for each generated coordinate
		/// (and starts with 0). If the function returns false, the enumeration is ended, and this function returns immediately with false.
		///
		/// \param bounds The bounds.
		/// \param func   The function to be called with the generated coordinates.
		///
		/// \return True if the enumeration completed, false if it was cancelled (by returning false from the callback).
		static bool EnumAllCoordinates(const libCZI::CDimBounds& bounds, const std::function<bool(std::uint64_t, const libCZI::CDimCoordinate& coord)>& func);

		/// Fill the specified bitmap with the specified color.
	   /// \param [in,out] bm         The bitmap.
	   /// \param          floatColor The color.
		static void FillBitmap(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor);

        /// Convert the "raw compression identifier" to an enumeration. Note that all unknown values (unknown to
        /// libCZI) are mapped to  CompressionMode::Invalid.
        /// Compression mode from raw compression identifier
        /// \param m The raw compression identifier to convert.
        /// \returns The corresponding compression enumeration.
		static libCZI::CompressionMode CompressionModeFromRawCompressionIdentifier(std::int32_t m);

        /// Convert the specified compression enumeration to the corresponding raw compression identifier.
        /// \param mode The compression enumeration mode.
        /// \returns The corresponding raw compression identifier.
		static std::int32_t CompressionModeToCompressionIdentifier(libCZI::CompressionMode mode);

		/// Determine if the specified value is a valid m-index.
		/// \param  mIndex The m-index to check.
		/// \returns True if the value is a valid m-index, false if not.
		static bool IsValidMindex(int mIndex)
		{
			return mIndex != (std::numeric_limits<int>::max)() && mIndex != (std::numeric_limits<int>::min)();
		}

		//! ZStdX compression level parameter. This parameter is used both with ZStd0 and ZStd1.
		//! Example: "zstd0:ExplicitLevel=2" or "zstd1:ExplicitLevel=2"
		static const char* const KEY_COMPRESS_EXPLICIT_LEVEL     /*= "ExplicitLevel"*/;

		//! ZStd1 compression preprocessing parameter. The valid value is "HiLoByteUnpack"
		//! The parameter is valid only for ZStd1 compression and ignored in case of ZStd0.
		//! Example:	"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack" 
		//!				and ignored in case of "zstd0:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		static const char* const KEY_COMPRESS_PRE_PROCESS		 /*= "PreProcess"*/;

		//! The valid (expected) value in case of Pre-Processing.
		//! The flag indicates whether the "HiLoByteUnpack" is enabled or not.
		//! The parameter and the value are valid only for 16- and 48-bit pixel images.
		//! For all other pixel types the parameter and the value are ignored.
		//! Example:	- Pixel type Bgr8  : Ignored	--> "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		//!             - Pixel type Bgr24 : Ignored	--> "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		//!				- Pixel type Gray16: Used		--> "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		//!				- Pixel type Gray48: Used		--> "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		static const char* const VALUE_COMPRESS_HILO_BYTE_UNPACK /*= "HiLoByteUnpack"*/;

		//! Define a type for compression options. It is a pair where one parameter is compression mode
		//! and the others are compression parameters.
		using CompressionOption = std::pair<libCZI::CompressionMode, std::shared_ptr<libCZI::ICompressParameters>>;

		//! Parse specified string as a compression option and return the compression option,
		//! which is a pair of compression mode and the compression parameters.
		//! The format of the string representation is: "<compression_method>: key=value; ...". 
		//! It starts with the name of the compression-method, followed by a colon,
		//! then followed by a list of key-value pairs which are separated by a semicolon.
		//! Examples: "zstd0:ExplicitLevel=3", "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"
		//! If parsing fails an excpetion of type "Logic_error" is thrown.
		static CompressionOption ParseCompressionOptions(const std::string& options);
	};
}