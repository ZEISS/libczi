writing CZI				{#writing_czi}
===========


## Writing a CZI ##

The CZI-writer object implements this interface:

~~~~~~~~~~~~~~~~~~~~~{.cpp}
		virtual void Create(
			shared_ptr<IOutputStream> stream, 
			shared_ptr<ICziWriterInfo> info) 
		virtual void SyncAddSubBlock(const AddSubBlockInfo& addSbBlkInfo)
		virtual void SyncAddAttachment(const AddAttachmentInfo& addAttachmentInfo)
		virtual void SyncWriteMetadata(const WriteMetadataInfo& metadataInfo)
		virtual shared_ptr<libCZI::ICziMetadataBuilder> GetPreparedMetadata(
			const PrepareMetadataInfo& info)
		virtual void Close();
~~~~~~~~~~~~~~~~~~~~~

The intented mode of operation is:
* Create an "output-stream-object" and pass it to the Create-function.
* Add subblocks and attachments to the file as desired.
* In order to complete the CZI, the metadata-segment needs to be added. It may be added at any point in time; however the method "GetPreparedMetadata"
   which creates some "pre-filled-metadata" will do so based on the current state (ie. on the subblocks which have been added so far). So, if using
   this utility-method, one usually wants to call it when all subblocks have been added.
* It is important to explicitely call "Close" when done, because within "Close" the CZI gets finalized, meaning the file-management data will be
   written out to the file here. If skipping the "Close"-call, the file will be incomplete.

### notes on implementing the output-stream object ###

The model employed for the output-stream is:
* it is possible to write at arbitrary positions (ie. access is not strictly consecutively)
* end-of-stream is defined as the highest position written to, there is no explicit method for setting the size
* it is legal to have offsets to which no data is written, and the content of such areas is undefined

### considerations for writing a CZI ###

The ICziWriterInfo-object passed in with the Create-function allows to specify a bounds for the coordinates and for the M-index. If the 
dimensions to be used in creating the CZI are known beforehand, it is recommended to specify them. If they are specified, then the
writer-object will check the validity of the coordinates added, and so help to ensure that the resulting CZI is valid and well-formed.
The CZI-segments containing the subblock-directory, the attachment-directory and the metadata are written last (in normal operation), and 
therefore they get added at the end of the file. However, those same segments are usually read first (most likely directly when opening
the file), so there is some benefit of having them at the beginning of the file (we can save a seek-operation). For this purpose, it is
possible to reserve space for those segments. The size for the reservations must be specified with the ICziWriterInfo at creation time.
If the reserved space is not sufficient, then still the segment will be added at the end.

### passing in data

In order to write bitmap-data into a subblock, the method SyncAddSubBlock takes an argument of type AddSubBlockInfo. It employes a pull-based approach, where callbacks are used to provide the data (which is to be written into the CZI).

~~~~~~~~~~~~~~~~~~~~~{.cpp}
struct AddSubBlockInfo : AddSubBlockInfoBase
	{
		// simplified

		size_t sizeData;		
		std::function
			<bool(int callCnt, 
				  size_t offset, 
				  const void*& ptr, 
				  size_t& size)> getData;

		size_t sizeMetadata;	
		std::function
			<bool(int callCnt, 
				  size_t offset, 
				  const void*& ptr, 
				  size_t& size)> getMetaData;

		size_t sizeAttachment;	
		std::function
			<bool(int callCnt, 
				  size_t offset, 
				  const void*& ptr, 
				  size_t& size)> getAttachment;
	};
~~~~~~~~~~~~~~~~~~~~~

A subblock consists of three parts - the data, the metadata and the attachment. For each of those parts, the size (in bytes) needs to be specified. Metadata and attachment are optional, and a zero size means that the corresponding part is not used. The function objects are called in order to retrieve the data. The callback-function needs to give a pointer and the size of a data-block. The functions are only called while the method SyncAddSubBlock executes, and the pointer needs to be valid until either the same function is called again or the SyncAddSubBlock-method returns. The function will be called as many times as necessary in order to retrieve the amount of data specified. The parameter "callCnt" increments which each call, and the parameter offset gives how many data has already been retrieved.
If the callback returns false, the remainder of the subblock will be filled with zeroes. If it provides more data than required, then the surplus data will not be used.

Pixel-data in a subblock needs to a stored with a stride exactly equal to bytes-per-pel times width-in-pixels. However, this callback-based approach allows to store pixel-data with an arbitrary stride.
The library provides a couple of overloads for SyncAddSubBlock 

~~~~~~~~~~~~~~~~~~~~~{.cpp}
void SyncAddSubBlock(const AddSubBlockInfoMemPtr& addSbBlkInfo);
void SyncAddSubBlock(const AddSubBlockInfoLinewiseBitmap& addSbInfoLinewise);
void SyncAddSubBlock(const AddSubBlockInfoStridedBitmap& addSbBlkInfoStrideBitmap);
~~~~~~~~~~~~~~~~~~~~~

They provide a simplified way if dealing with bitmap-data when it is given as e. g. a bitmap consecutive in memory but with a specific stride (which may not be equal to the minimal stride required in storage in CZI).