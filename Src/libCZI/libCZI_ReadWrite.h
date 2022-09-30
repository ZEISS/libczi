// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"

namespace libCZI
{
	/// Options for the CziReaderWriter.
	class LIBCZI_API ICziReaderWriterInfo
	{
	public:
		/// Gets a value indicating that the GUID in the existing file-handler should be replaced with the GUID reported by 'GetFileGuid()'.
		///
		/// \return True if the GUID should be replaced, false otherwise.
		virtual bool GetForceFileGuid() const = 0;

		/// Gets file's unique identifier. If we report GUID_NULL, then the file-writer will create a GUID on its own.
		/// Note that this GUID is only retrieved and used if the existing file is empty or if GetForceFileGuid() gives true.
		/// \return The file's unique identifier.
		virtual const GUID& GetFileGuid() const = 0;

		virtual ~ICziReaderWriterInfo() {}
	};

	/// Interface for "in-place-editing" of a CZI. All write-operations immediately go into the file. If the data does not fit into the
	/// existing segments, a new segment is appended at the end (and the existing one is marked "DELETED").
	/// All operation is strictly single-threaded. Only exactly one method may be executing at a given point in time.
	/// Notes:
	/// - The indices (or "keys") for a subblock/attachment do not change during the lifetime of the object (even if deleting some).  
	/// - Contrary to ICziWriter, this object does not attempt to verify the consistency of the coordinates - which is due the fact  
	///    that we aim at allowing arbitrary modifications. We do not require to specify in advance the number of dimensions or the bounds.
	/// - The information returned by ISubBlockRepository::GetStatistics is valid (taking into consideration the current state).
	class LIBCZI_API ICziReaderWriter : public ISubBlockRepository, public IAttachmentRepository
	{
	public:

		/// Initialize the object.
		///
		/// \param stream The read-write stream to operate on.
		/// \param info   (Optional) Parameters controlling the operation.
		virtual void Create(std::shared_ptr<IInputOutputStream> stream, std::shared_ptr<ICziReaderWriterInfo> info = nullptr) = 0;

		/// Replace an existing subblock. The subblock is identified by an index (as reported by ISubBlockRepository::EnumerateSubBlocks).
		///
		/// \param key		    The key (as retrieved by ISubBlockRepository::EnumerateSubBlocks).
		/// \param addSbBlkInfo Information describing the subblock to be added.
		virtual void ReplaceSubBlock(int key, const AddSubBlockInfo& addSbBlkInfo) = 0;

		/// Removes the specified subblock. Physically, it is marked as "DELETED".
		///
		/// \param key The key (as retrieved by ISubBlockRepository::EnumerateSubBlocks).
		virtual void RemoveSubBlock(int key) = 0;

		/// Replace an existing attachment. The attachment is identified by an index (as reported by IAttachmentRepository::EnumerateAttachments).
		///
		/// \param attchmntId		 Identifier for the attachmnt  (as reported by IAttachmentRepository::EnumerateAttachments).
		/// \param addAttachmentInfo Information describing attachment to be added.
		virtual void ReplaceAttachment(int attchmntId, const AddAttachmentInfo& addAttachmentInfo) = 0;

		/// Removes the specified attachment. Physically, it is marked as "DELETED".
		///
		/// \param attchmntId Identifier for the attachmnt  (as reported by IAttachmentRepository::EnumerateAttachments).
		virtual void RemoveAttachment(int attchmntId) = 0;

		/// Adds the specified subblock to the CZI-file.
		///
		/// \param addSbBlkInfo Information describing the subblock to be added.
		virtual void SyncAddSubBlock(const AddSubBlockInfo& addSbBlkInfo) = 0;

		/// Adds the specified attachment to the CZI-file.
		///
		/// \param addAttachmentInfo Information describing the attachment to be added.
		virtual void SyncAddAttachment(const AddAttachmentInfo& addAttachmentInfo) = 0;

		/// Write metadata segment.
		///
		/// \param metadataInfo Information describing the metadata.
		virtual void SyncWriteMetadata(const WriteMetadataInfo& metadataInfo) = 0;

		/// Reads the metadata-segment from the stream. If no metadata-segment is present, then an empty shared_ptr is returned.
		///
		/// \return The metadata segment if successful, otherwise an empty shared_ptr is returned.
		virtual std::shared_ptr<IMetadataSegment> ReadMetadataSegment() = 0;

		/// Gets the file header information.
		/// \return The file header information.
		virtual FileHeaderInfo GetFileHeaderInfo() = 0;

		/// Finalizes the CZI (ie. writes out the final directory-segments) and closes the file.
		/// Note that this method must be called explicitely in order to get a valid CZI - calling the destructor alone will
		/// close the file immediately without finalization.
		virtual void Close() = 0;

		virtual ~ICziReaderWriter() override = default;

		/// This helper method uses the structure 'AddSubBlockInfoMemPtr' in order to describe the
		/// subblock to be added. What it does is to cast the parameters into the form required by the
		/// ICziReaderWriterInfo::SyncAddSubBlock method and call it.
		///
		/// \param addSbBlkInfoMemPtr Information describing the subblock to be added.
		void SyncAddSubBlock(const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfoMemPtr);

		/// This helper method uses the structure 'AddSubBlockInfoLinewiseBitmap' in order to describe
		/// the subblock to be added. What it does is to cast the parameters into the form required by
		/// the ICziReaderWriterInfo::SyncAddSubBlock method and call it.
		///
		/// \param addSbInfoLinewise Information describing the subblock to be added.
		void SyncAddSubBlock(const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise);

		/// This helper method uses the structure 'AddSubBlockInfoStridedBitmap' in order to describe the
		/// subblock to be added. What it does is to cast the parameters into the form required by the
		/// ICziReaderWriterInfo::SyncAddSubBlock method and call it.
		///
		/// \param addSbBlkInfoStrideBitmap Information describing the subblock to be added.
		void SyncAddSubBlock(const libCZI::AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap);

		/// This helper method uses the structure 'AddSubBlockInfoMemPtr' in order to describe the
		/// subblock to be replaced. What it does is to cast the parameters into the form required by the
		/// ICziReaderWriterInfo::ReplaceSubBlock method and call it.
		///
		/// \param key				  The key identifying the subblock to be replaced.
		/// \param addSbBlkInfoMemPtr Information describing the subblock to be added.
		void ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoMemPtr& addSbBlkInfoMemPtr);

		/// This helper method uses the structure 'AddSubBlockInfoLinewiseBitmap' in order to describe
		/// the subblock to be replaced. What it does is to cast the parameters into the form required by
		/// the ICziReaderWriterInfo::ReplaceSubBlock method and call it.
		///
		/// \param key				 The key identifying the subblock to be replaced.
		/// \param addSbInfoLinewise Information describing the subblock to be added.
		void ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise);

		/// This helper method uses the structure 'AddSubBlockInfoStridedBitmap' in order to describe the
		/// subblock to be replaced. What it does is to cast the parameters into the form required by the
		/// ICziReaderWriterInfo::ReplaceSubBlock method and call it.
		///
		/// \param key				 The key identifying the subblock to be replaced.
		/// \param addSbBlkInfoStrideBitmap Information describing the subblock to be added.
		void ReplaceSubBlock(int key, const libCZI::AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap);
	};

	/// An implementation of the ICziReaderWriterInfo-interface.
	class LIBCZI_API CCziReaderWriterInfo :public libCZI::ICziReaderWriterInfo
	{
	private:
		bool forceFileGuid;
		GUID fileGuid;			///< The GUID to be set as the CZI's file-guid.
	public:
		/// Default constructor - sets all information to "invalid" and sets fileGuid to GUID_NULL.
		CCziReaderWriterInfo() : CCziReaderWriterInfo(GUID{ 0,0,0,{ 0,0,0,0,0,0,0,0 } })
		{}

		/// Constructor.
		///
		/// \param fileGuid Unique identifier for the file.
		CCziReaderWriterInfo(const GUID& fileGuid) : forceFileGuid(false)
		{
			this->fileGuid = fileGuid;
		}

		virtual bool GetForceFileGuid() const override { return this->forceFileGuid; }
		virtual const GUID& GetFileGuid() const override { return this->fileGuid; }

		/// Sets "force file GUID" flag.
		///
		/// \param forceFileGuid True to force the specified file-Guid.
		void SetForceFileGuid(bool forceFileGuid) { this->forceFileGuid = forceFileGuid; }
	};
}
