// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"

#if !defined(_LIBCZISTATICLIB) && !defined(__GNUC__)

// When used as a DLL, we need to tinker with STL-classes used in exported structs/classes, or we get "warning C4251".
// Note that using a different compiler-version/STL-version is still problematic and should be avoided.
#if defined(LIBCZI_EXPORTS)
	// https://web.archive.org/web/20141227011407/http://support.microsoft.com/kb/168958
	template class LIBCZI_API std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)>;
	template class LIBCZI_API std::function<const void*(int line)>;
#else
	extern template class LIBCZI_API std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)>;
	extern template class LIBCZI_API std::function<const void*(int line)>;
#endif

#endif

namespace libCZI
{
	/// The options for the CZI-writer.
	class LIBCZI_API ICziWriterInfo
	{
	public:

		/// Gets bounds for the subblocks we are going to add to the CZI. If this is a valid bounds, then
		/// the coordinates of each subblock added are checked against this bounds. In case of a violation,
		/// an LibCZIWriteException-exception is thrown.
		///
		/// \return Null if the bounds is not specified, else the bounds for the subblock's coordinates we want to add.
		virtual const IDimBounds* GetDimBounds() const = 0;

		/// Gets file's unique identifier. If we report GUID_NULL, then the file-writer will create a GUID on its own.
		/// \return The file's unique identifier.
		virtual const GUID& GetFileGuid() const = 0;

		/// Attempts to get the minimum and the maximum (inclusive) for the m-index. If returning a valid interval (ie. if the
		/// return value is true), then the M-coordinate of each subblock added is checked against this. Furthermore, in this
		/// case we require that all subblocks have a valid M-index.
		///
		/// \param [out] min If non-null will receive the minimum M-index (inclusive).
		/// \param [out] max If non-null will receive the maximum M-index (inclusive).
		///
		/// \return True if it succeeds, false the is no bounds for the M-index.
		virtual bool TryGetMIndexMinMax(int* min, int* max) const = 0;

		/// Query whether to reserve space for the attachment-directory-segment at the start of the file.
		/// If returning true, the value returned for size has the following meaning: if it is >0, it is 
		/// interpreted as the number of attachment-entries (ie. how many attachments can be put into the CZI).
		/// If it is 0 (and returning true), some default is used.
		/// If returning false, no space is reserved, and the attachment-directory is put at the end of the CZI.
		/// If the reserved space is not sufficient, then the attachment-directory-segment is put at the end
		/// of the CZI (and the reserved space is unused).
		///
		/// \param [out] size If returning true, then this gives the number of attachment-entries to reserve.
		///
		/// \return True if space for the attachment-directory-segment is to be reserved, false otherwise.
		virtual bool TryGetReservedSizeForAttachmentDirectory(size_t* size) const = 0;

		/// Query whether to reserve space for the subblock-directory-segment at the start of the file.
		/// If returning true, the value returned for size has the following meaning: if it is >0, it is 
		/// interpreted as the number of subblock-entries (ie. how many subblocks can be put into the CZI).
		/// If it is 0 (and returning true), some default is used.
		/// If returning false, no space is reserved, and the subblock-directory is put at the end of the CZI.
		/// If the reserved space is not sufficient, then the subblock-directory-segment is put at the end
		/// of the CZI (and the reserved space is unused).
		///
		/// \param [out] size If returning true, then this gives the number of subblock-entries to reserve.
		///
		/// \return True if space for the subblock-directory-segment is to be reserved, false otherwise.
		virtual bool TryGetReservedSizeForSubBlockDirectory(size_t* size) const = 0;

		/// Query whether to reserve space for the metadata-segment at the start of the file.
		/// If returning true, the value returned for size has the following meaning: if it is >0, it is 
		/// interpreted as the number of bytes available for the metadata-segment.
		/// If it is 0 (and returning true), some default is used.
		/// If returning false, no space is reserved, and the metadata-segment is put at the end of the CZI.
		/// If the reserved space is not sufficient, then the metadata-segment is put at the end
		/// of the CZI (and the reserved space is unused).
		///
		/// \param [out] size If returning true, then this gives the number of bytes to be reserved in the metadata-segment.
		///
		/// \return True if space (in bytes) for the metadata is to be reserved, false otherwise.
		virtual bool TryGetReservedSizeForMetadataSegment(size_t* size) const = 0;

		virtual ~ICziWriterInfo() {};
	};

	/// An implementation of the ICziWriterInfo-interface.
	class LIBCZI_API CCziWriterInfo :public libCZI::ICziWriterInfo
	{
	private:
		bool dimBoundsValid;
		CDimBounds dimBounds;
		GUID fileGuid;			///< The GUID to be set as the CZI's file-guid.
		bool mBoundsValid;
		int mMin, mMax;
		size_t reservedSizeAttachmentsDir, reservedSizeSubBlkDir, reservedSizeMetadataSegment;
		bool reservedSizeAttachmentsDirValid, reservedSizeSubBlkDirValid, reservedSizeMetadataSegmentValid;
	public:
		/// Default constructor - sets all information to "invalid" and sets fileGuid to GUID_NULL.
		CCziWriterInfo() : CCziWriterInfo(GUID{ 0,0,0,{0,0,0,0,0,0,0,0} })
		{}

		/// Constructor - leaves the bounds undefined.
		///
		/// \param fileGuid The GUID to be set as the CZI's file-guid. If this is GUID_NULL (all 0's),
		/// then we will create a new Guid and use it.
		/// \param mMin	    (Optional) The minimum for the M-index (inclusive).
		/// \param mMax	    (Optional) The maximum for the M-index (inclusive).
		CCziWriterInfo(const GUID& fileGuid, int mMin = 1, int mMax = -1)
			: fileGuid(fileGuid), reservedSizeAttachmentsDirValid(false), reservedSizeSubBlkDirValid(false), reservedSizeMetadataSegmentValid(false)
		{
			this->SetDimBounds(nullptr);
			this->SetMIndexBounds(mMin, mMax);
		}

		/// Constructor.
		///
		/// \param fileGuid The GUID to be set as the CZI's file-guid. If this is GUID_NULL (all 0's),
		/// then we will create a new Guid and use it.
		/// \param bounds   The bounds.
		/// \param mMin	    (Optional) The minimum for the M-index (inclusive).
		/// \param mMax	    (Optional) The maximum for the M-index (inclusive).
		CCziWriterInfo(const GUID& fileGuid, const IDimBounds& bounds , int mMin = 1, int mMax = -1) 
			: fileGuid(fileGuid), reservedSizeAttachmentsDirValid(false), reservedSizeSubBlkDirValid(false), reservedSizeMetadataSegmentValid(false)
		{
			this->SetDimBounds(&bounds);
			this->SetMIndexBounds(mMin, mMax);
		}

		virtual const IDimBounds* GetDimBounds() const override { return this->dimBoundsValid ? &this->dimBounds : nullptr; }
		virtual const GUID& GetFileGuid() const override { return this->fileGuid; }
		virtual bool TryGetMIndexMinMax(int* min, int* max) const override;
		virtual bool TryGetReservedSizeForAttachmentDirectory(size_t* size) const override;
		virtual bool TryGetReservedSizeForSubBlockDirectory(size_t* size) const override;
		virtual bool TryGetReservedSizeForMetadataSegment(size_t* size) const override;

		/// Sets reserved size for the "attachments directory".
		///
		/// \param reserveSpace True to reserve space.
		/// \param s		    The size (in bytes) to reserve for the "attachments directory".
		void SetReservedSizeForAttachmentsDirectory(bool reserveSpace, size_t s);

		/// Sets reserved size for the "subblock directory".
		///
		/// \param reserveSpace True to reserve space.
		/// \param s		    The size (in bytes) to reserve for the "subblock directory".
		void SetReservedSizeForSubBlockDirectory(bool reserveSpace, size_t s);

		/// Sets reserved size for the "metadata segment".
		///
		/// \param reserveSpace True to reserve space.
		/// \param s		    The size (in bytes) to reserve for the "metadata segment".
		void SetReservedSizeForMetadataSegment(bool reserveSpace, size_t s);

		/// Sets the bounds. If null is specified, then we report "no valid bounds" with 'GetDimBounds()'.
		/// \param bounds The bounds (may be nullptr).
		void SetDimBounds(const IDimBounds* bounds);

		/// Sets the M-index bounds. If mMax < mMin, then we report "no valid M-index-bounds" with 'TryGetMIndexMinMax'.
		///
		/// \param mMin The minimum value for the m-Index (inclusive).
		/// \param mMax The maximum value for the m-Index (inclusive).
		void SetMIndexBounds(int mMin, int mMax);
	};

	/// Information about a subblock.
	struct LIBCZI_API AddSubBlockInfoBase
	{
		/// Default constructor
		AddSubBlockInfoBase() { this->Clear(); }

		libCZI::CDimCoordinate coordinate;	///< The subblock's coordinate.
		bool mIndexValid;					///< Whether the field 'mIndex' is valid;
		int mIndex;							///< The M-index of the subblock.
		int x;								///< The x-coordinate of the subblock.
		int y;								///< The x-coordinate of the subblock.
		int logicalWidth;					///< The logical with of the subblock (in pixels).
		int logicalHeight;					///< The logical height of the subblock (in pixels).
		int physicalWidth;					///< The physical with of the subblock (in pixels).
		int physicalHeight;					///< The physical height of the subblock (in pixels).
		libCZI::PixelType PixelType;		///< The pixel type of the subblock.

		/// The compression-mode (applying to the subblock-data). If using a compressed format, the data
		/// passed in must be already compressed - the writer does _not_ perform the compression.
        /// The value specified here is the "raw value", use "GetCompressionMode()" or "Utils::CompressionModeFromRawCompressionIdentifier" in
        /// order to identify well-known compression modes.
        std::int32_t compressionModeRaw;

		/// Clears this object to its blank/initial state.
		virtual void Clear();

        /// Sets compression mode (specifying a compression enumeration).
        /// \param m The compression enumeration.
        void SetCompressionMode(libCZI::CompressionMode m)
        {
            this->compressionModeRaw = libCZI::Utils::CompressionModeToCompressionIdentifier(m);
        }

        /// Gets compression mode enumeration. Note that unknown compression-modes (unknown to libCZI) are 
        /// mapped to CompressionMode::Invalid.
        /// \returns The compression mode.
        libCZI::CompressionMode GetCompressionMode() const
        {
            return libCZI::Utils::CompressionModeFromRawCompressionIdentifier(this->compressionModeRaw);
        }
	};

	/// Information for adding a subblock to a CZI-file with ICziWriter. Here we add the information
	/// about the payload-data. We employ a pull-based model, where the caller has to supply callback-functions
	/// for delivering the data.
	struct LIBCZI_API AddSubBlockInfo : AddSubBlockInfoBase
	{
		/// Default constructor
		AddSubBlockInfo() : sizeData(0), sizeMetadata(0), sizeAttachment(0) {}

		/// Copy-Constructor which copies all information from the specified base-class object.
		///
		/// \param other The other object of type AddSubBlockInfoBase from which information will be copied.
		explicit AddSubBlockInfo(const AddSubBlockInfoBase& other) :
			AddSubBlockInfoBase(other), sizeData(0), sizeMetadata(0), sizeAttachment(0)
		{}

		size_t sizeData;		///< The size of the subblock's data in bytes.

		/// The functor will be called to retrieve the subblock's data. The function must set the arguments 'ptr' and 'size',
		/// and the memory pointed to must be valid until the next call into the functor (or returning from the 'SyncAddSubBlock' method).
		/// The argument 'callCnt' is incremented with each call (starting from 0), and 'offset' is incremented by the number of bytes already
		/// retrieved. The functor will be called until the amount of bytes specified by 'sizeData" has been retrieved or if it returns false.
		/// If returning false, the subblock-data is filled with zeroes (if necessary) so that 'sizeData' bytes are reached.
		std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)> getData;

		size_t sizeMetadata;	///< The size of the subblock's metadata in bytes (note: max value is (numeric_limits<int>::max() ).

		/// The functor will be called to retrieve the subblock's metadata. The function must set the arguments 'ptr' and 'size',
		/// and the memory pointed to must be valid until the next call into the functor (or returning from the 'SyncAddSubBlock' method).
		/// The argument 'callCnt' is incremented with each call (starting from 0), and 'offset' is incremented by the number of bytes already
		/// retrieved. The functor will be called until the amount of bytes specified by 'sizeMetadata" has been retrieved or if it returns false.
		/// If returning false, the subblock-metadata is filled with zeroes (if necessary) so that 'sizeMetadata' bytes are reached.
		std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)> getMetaData;

		size_t sizeAttachment;	///< The size of the subblock's attachment in bytes (note: max value is (numeric_limits<int>::max() ).

		/// The functor will be called to retrieve the subblock's attachment. The function must set the arguments 'ptr' and 'size',
		/// and the memory pointed to must be valid until the next call into the functor (or returning from the 'SyncAddSubBlock' method).
		/// The argument 'callCnt' is incremented with each call (starting from 0), and 'offset' is incremented by the number of bytes already
		/// retrieved. The functor will be called until the amount of bytes specified by 'sizeAttachment" has been retrieved or if it returns false.
		/// If returning false, the subblock-attachment is filled with zeroes (if necessary) so that 'sizeAttachment' bytes are reached.
		std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)> getAttachment;

		/// Clears this object to its blank/initial state.
		virtual void Clear() override;
	};

	/// This struct defines the data to be added to the subblock segment. Unused entries (e.g. no subblock-metadata) must have a size of 0.
	/// This variant is used if the data is readily available in contiguous memory.
	/// Note that for an uncompressed bitmap, the stride must be exactly width*bytes_per_pixel.
	struct LIBCZI_API AddSubBlockInfoMemPtr : public AddSubBlockInfoBase
	{
		/// Default constructor
		AddSubBlockInfoMemPtr() :
			ptrData(nullptr), dataSize(0), ptrSbBlkMetadata(nullptr), sbBlkMetadataSize(0), ptrSbBlkAttachment(nullptr), sbBlkAttachmentSize(0)
		{}

		const void* ptrData;				///< Pointer to the data to be put into the subblock.
		std::uint32_t dataSize;				///< The size of the data in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-data written).

		const void* ptrSbBlkMetadata;		///< Pointer to the subblock-metadata.
		std::uint32_t sbBlkMetadataSize;	///< The size of the subblock-metadata in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		const void* ptrSbBlkAttachment;		///< Pointer to the subblock-attachment.
		std::uint32_t sbBlkAttachmentSize;  ///< The size of the subblock-attachment in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		/// Clears this object to its blank/initial state.
		virtual void Clear() override;
	};

	/// This struct defines the data to be added to the subblock segment.
	/// This variant is used if the uncompressed bitmap-data has an arbitrary stride.
	/// Note that when writing compressed data, this variant does not make much sense to use.
	struct LIBCZI_API AddSubBlockInfoStridedBitmap : public AddSubBlockInfoBase
	{
		/// Default constructor
		AddSubBlockInfoStridedBitmap() :
			ptrBitmap(nullptr), strideBitmap(0), ptrSbBlkMetadata(nullptr), sbBlkMetadataSize(0), ptrSbBlkAttachment(nullptr), sbBlkAttachmentSize(0)
		{}

		const void* ptrBitmap;				///< Pointer to the the bitmap to be put into the subblock. The size of the memory-block must be (strideBitmap * physicalWidth).
		std::uint32_t strideBitmap;			///< The stride of the bitmap.

		const void* ptrSbBlkMetadata;		///< Pointer to the subblock-metadata.
		std::uint32_t sbBlkMetadataSize;	///< The size of the subblock-metadata in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		const void* ptrSbBlkAttachment;		///< Pointer to the subblock-attachment.
		std::uint32_t sbBlkAttachmentSize;  ///< The size of the subblock-attachment in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		/// Clears this object to its blank/initial state.
		virtual void Clear() override;
	};

	/// This struct defines the data to be added to the subblock segment.
	/// This variant uses a callback-function in order to supply the writer with the bitmap-data, which will be called for every
	/// line of the bitmap.
	struct LIBCZI_API AddSubBlockInfoLinewiseBitmap : public AddSubBlockInfoBase
	{
		/// Default constructor
		AddSubBlockInfoLinewiseBitmap() :ptrSbBlkMetadata(nullptr), sbBlkMetadataSize(0), ptrSbBlkAttachment(nullptr), sbBlkAttachmentSize(0)
		{}

		/// This functor will be called for every line, ie. the parameter line will count from 0 to physicalHeight-1. The pointer
		/// returned by this function must be valid until the next call into the functor (or returning from the 'SyncAddSubBlock' method).
		std::function<const void*(int line)> getBitmapLine;

		const void* ptrSbBlkMetadata;		///< Pointer to the subblock-metadata.
		std::uint32_t sbBlkMetadataSize;	///< The size of the subblock-metadata in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		const void* ptrSbBlkAttachment;		///< Pointer to the subblock-attachment.
		std::uint32_t sbBlkAttachmentSize;  ///< The size of the subblock-attachment in bytes. If this is 0, then ptrSbBlkMetadata is not used (and no sub-block-metadata written).

		/// Clears this object to its blank/initial state.
		virtual void Clear() override;
	};
	
	/// This struct describes an attachment to be added to a CZI-file.
	struct LIBCZI_API AddAttachmentInfo
	{
		/// Unique identifier for the content.
		GUID contentGuid;

		/// The content file type.
		std::uint8_t contentFileType[8];

		/// The attachment's name.
		std::uint8_t name[80];

		const void* ptrData;	///< Pointer to the attachment data.
		std::uint32_t dataSize; ///< Size of the attachment data (in bytes).

		/// Sets content file type. Note that the 'content file type' is a fixed-length string (of length 8),
		/// longer strings will be truncated.
		///
		/// \param sz The 'content file type'-string.
		void SetContentFileType(const char* sz)
		{
			memset(this->contentFileType, 0, sizeof(this->contentFileType));
            const auto len = strlen(sz);
			memcpy(this->contentFileType, sz, (std::min)(sizeof(this->contentFileType), len));
		}

		/// Sets 'name' of the attachment. Note that the 'content file type' is a fixed-length string (of length 80),
		/// longer strings will be truncated.
		///
		/// \param sz The 'name'-string.
		void SetName(const char* sz)
		{
			memset(this->name, 0, sizeof(this->name));
			const auto len = strlen(sz);
			memcpy(this->name, sz, (std::min)(sizeof(this->name), len));
		}
	};

	/// This struct defines the data to be added as metadata-segment. Unused entries (e. g. no attachment) must have a size of 0.
	struct LIBCZI_API WriteMetadataInfo
	{
		const char* szMetadata;		///< The xml-string (in UTF-8 encoding)
		size_t      szMetadataSize;	///< The size of the XML-string (in bytes)
		const void* ptrAttachment;	///< The metadata-attachment (not commonly used).
		size_t		attachmentSize;	///< The size of the metadata-attachment.

		/// Clears this object to its blank/initial state.
		void Clear() { memset(this, 0, sizeof(*this)); }
	};

	/// Information which is used to construct the metadata-preparation.
	struct PrepareMetadataInfo
	{
		/// This function is called to generate the values for the attributes "Id" and "Name" for the channels. The argument
		/// is the channel-Index, and the return-value is a tuple with the Id-value as first item, and the Name-value as second
		/// item. The Id is mandatory (and it must be unique), the Name is optional. The boolean in the tuple of the second
		/// item indicates whether the "Name"-attribute is to be written.
		/// If no function is given, a default Id is constructed as "Channel:<channel-index>".
		/// The strings are expected in UTF-8 encoding.
		std::function<std::tuple<std::string, std::tuple<bool, std::string>>(int)> funcGenerateIdAndNameForChannel;
	};

	/// This interface is used in order to write a CZI-file. The sequence of operations is: the object is initialized
	/// by calling the Create-method. Then use SyncAddSubBlock, SyncAddAttachment and SyncWriteMetadata to put data
	/// into the document. Finally, call Close which will finalized the document.
	/// Note that this object is not thread-safe. Calls into any of the functions must be synchronized, i. e. at no
	/// point in time we may execute different methods (or the same method for that matter) concurrently. The class by
	/// itself does not guard itself against concurrent execution.
	class LIBCZI_API ICziWriter
	{
	public:
		/// Initialize the writer by passing in the output-stream-object. 
		/// \param stream The stream-object.
		/// \param info Settings for document-creation. May be null. 
		/// \remark
		/// If this method is called twice, then an exception of type std::logic_error is thrown.
		virtual void Create(std::shared_ptr<IOutputStream> stream, std::shared_ptr<ICziWriterInfo> info) = 0;

		/// Adds the specified subblock to the CZI-file. This is a synchronous method, meaning that it will return when all
		/// data has been written out to the file AND that it must not be called concurrently with other method-invocations of
		/// this object.
		/// If there are bounds specified (with the 'info'-argument to 'Create') then the coordinate is checked against the bounds.
		/// In case of any error, an exception is thrown.
		/// \param addSbBlkInfo Information describing the subblock to be added.
		virtual void SyncAddSubBlock(const AddSubBlockInfo& addSbBlkInfo) = 0;

		/// Adds the specified attachment to the CZI-file. This is a synchronous method, meaning that it will return when all
		/// data has been written out to the file AND that it must not be called concurrently with other method-invocations of
		/// this object.
		///
		/// \param addAttachmentInfo Information describing attachment to be added.
		virtual void SyncAddAttachment(const AddAttachmentInfo& addAttachmentInfo) = 0;

		/// Adds the specified metadata to the CZI-file. This is a synchronous method, meaning that it will return when all
		/// data has been written out to the file AND that it must not be called concurrently with other method-invocations of
		/// this object.
		/// \param metadataInfo Information describing the metadata to be added.
		virtual void SyncWriteMetadata(const WriteMetadataInfo& metadataInfo) = 0;

		/// Gets a "pre-filled" metadata object. This metadata object contains the information which is already known by the writer.
		/// \param info Information controlling the operation.
		/// \return The "pre-filled" metadata object if successful.
		virtual std::shared_ptr<libCZI::ICziMetadataBuilder> GetPreparedMetadata(const PrepareMetadataInfo& info) = 0;

		/// Finalizes the CZI (ie. writes out the final directory-segments) and closes the file.
		/// Note that this method must be called explicitely in order to get a valid CZI - calling the destructor alone will
		/// close the file immediately without finalization.
		virtual void Close() = 0;

		virtual ~ICziWriter() = default;

		/// This helper method uses the structure 'AddSubBlockInfoMemPtr' in order to describe the subblock to be added. What it does is
		/// to cast the parameters into the form required by the ICziWriter::SyncAddSubBlock method and call it.
		///
		/// \param addSbBlkInfo Information describing the subblock to be added.
		void SyncAddSubBlock(const AddSubBlockInfoMemPtr& addSbBlkInfo);

		/// This helper method uses the structure 'AddSubBlockInfoLinewiseBitmap' in order to describe the subblock to be added. What it does is
		/// to cast the parameters into the form required by the ICziWriter::SyncAddSubBlock method and call it.
		///
		/// \param addSbInfoLinewise Information describing the subblock to be added.
		void SyncAddSubBlock(const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise);

		/// This helper method uses the structure 'AddSubBlockInfoStridedBitmap' in order to describe the subblock to be added. What it does is
		/// to cast the parameters into the form required by the ICziWriter::SyncAddSubBlock method and call it.
		///
		/// \param addSbBlkInfoStrideBitmap Information describing the subblock to be added.
		void SyncAddSubBlock(const AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap);
	};

	//-------------------------------------------------------------------------------------------

	inline void AddSubBlockInfoBase::Clear()
	{
		this->coordinate.Clear();
		this->mIndexValid = false;
		this->x = (std::numeric_limits<int>::min)();
		this->y = (std::numeric_limits<int>::min)();
		this->logicalWidth = 0;
		this->logicalHeight = 0;
		this->physicalWidth = 0;
		this->physicalHeight = 0;
		this->PixelType = libCZI::PixelType::Invalid;
        this->SetCompressionMode(CompressionMode::UnCompressed);
	}

	inline void AddSubBlockInfoMemPtr::Clear()
	{
		this->AddSubBlockInfoBase::Clear();
		this->ptrData = nullptr;
		this->dataSize = 0;
		this->ptrSbBlkMetadata = nullptr;
		this->sbBlkMetadataSize = 0;
		this->ptrSbBlkAttachment = nullptr;
		this->sbBlkAttachmentSize = 0;
	}

	inline void AddSubBlockInfoLinewiseBitmap::Clear()
	{
		this->AddSubBlockInfoBase::Clear();
		this->getBitmapLine = nullptr;
		this->ptrSbBlkMetadata = nullptr;
		this->sbBlkMetadataSize = 0;
		this->ptrSbBlkAttachment = nullptr;
		this->sbBlkAttachmentSize = 0;
	}

	inline void AddSubBlockInfo::Clear()
	{
		this->AddSubBlockInfoBase::Clear();
		this->sizeData = 0;
		this->getData = nullptr;
		this->sizeMetadata = 0;
		this->getMetaData = nullptr;
		this->sizeAttachment = 0;
		this->getAttachment = nullptr;
	}

	inline void AddSubBlockInfoStridedBitmap::Clear()
	{
		this->AddSubBlockInfoBase::Clear();
		this->ptrBitmap = nullptr;
		this->strideBitmap = 0;
		this->ptrSbBlkMetadata = nullptr;
		this->sbBlkMetadataSize = 0;
		this->ptrSbBlkAttachment = nullptr;
		this->sbBlkAttachmentSize = 0;
	}

	inline /*virtual*/bool CCziWriterInfo::TryGetMIndexMinMax(int* min, int* max) const
	{
		if (!this->mBoundsValid)
		{
			return false;
		}

		if (min != nullptr) { *min = this->mMin; }
		if (max != nullptr) { *max = this->mMax; }
		return true;
	}

	inline /*virtual*/bool CCziWriterInfo::TryGetReservedSizeForAttachmentDirectory(size_t* size) const
	{
		if (this->reservedSizeAttachmentsDirValid)
		{
			if (size!=nullptr)
			{
				*size = this->reservedSizeAttachmentsDir;
			}

			return true;
		}

		return false;
	}

	inline /*virtual*/bool CCziWriterInfo::TryGetReservedSizeForSubBlockDirectory(size_t* size) const
	{
		if (this->reservedSizeSubBlkDirValid)
		{
			if (size != nullptr)
			{
				*size = this->reservedSizeSubBlkDir;
			}

			return true;
		}

		return false;
	}

	inline /*virtual*/bool CCziWriterInfo::TryGetReservedSizeForMetadataSegment(size_t* size) const
	{
		if (this->reservedSizeMetadataSegmentValid)
		{
			if (size != nullptr)
			{
				*size = this->reservedSizeMetadataSegment;
			}

			return true;
		}

		return false;
	}

	inline void CCziWriterInfo::SetReservedSizeForAttachmentsDirectory(bool reserveSpace, size_t s)
	{
		this->reservedSizeAttachmentsDirValid = reserveSpace;
		if (reserveSpace)
		{
			this->reservedSizeAttachmentsDir = s;
		}
	}

	inline void CCziWriterInfo::SetReservedSizeForSubBlockDirectory(bool reserveSpace, size_t s)
	{
		this->reservedSizeSubBlkDirValid = reserveSpace;
		if (reserveSpace)
		{
			this->reservedSizeSubBlkDir = s;
		}
	}

	inline void CCziWriterInfo::SetReservedSizeForMetadataSegment(bool reserveSpace, size_t s)
	{
		this->reservedSizeMetadataSegmentValid = reserveSpace;
		if (reserveSpace)
		{
			this->reservedSizeMetadataSegment = s;
		}
	}

	inline void CCziWriterInfo::SetDimBounds(const IDimBounds* bounds)
	{
		if (bounds == nullptr)
		{
			this->dimBoundsValid = false;
		}
		else
		{
			this->dimBounds = CDimBounds(bounds);
			this->dimBoundsValid = true;
		}
	}

	inline void CCziWriterInfo::SetMIndexBounds(int mMin, int mMax)
	{
		if (mMax < mMin)
		{
			this->mBoundsValid = false;
		}
		else
		{
			this->mMin = mMin;
			this->mMax = mMax;
			this->mBoundsValid = true;
		}
	}
}
