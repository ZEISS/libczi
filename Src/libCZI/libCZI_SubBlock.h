#pragma once

#include <tuple>
#include "libCZI_Metadata.h"
#include "libCZI_Utilities.h"

namespace libCZI
{
    /// This interface provides typed access to the metadata of a sub-block.
    class ISubBlockMetadataMetadataView
    {
    public:
        ISubBlockMetadataMetadataView() = default;
        virtual ~ISubBlockMetadataMetadataView() = default;

        /// Attempts to get "attachment data format" - the format of data in the attachment of the sub-block.
        /// This information is retrieved from the node "METADATA/AttachmentSchema/DataFormat".
        /// 
        /// \param [out]	data_format	If non-null, the data format is output here.
        /// \returns	True if it succeeds; false otherwise.
        virtual bool TryGetAttachmentDataFormat(std::wstring* data_format) = 0;

        /// Attempts to get the specified tag, parsed as a double, from the sub-block metadata.
        /// The data is retrieved from the node "METADATA/Tags/<tag-name>".
        ///
        /// \param 		   	tag_name	The tag name.
        /// \param [in,out]	value   	If non-null, the value is put here
        ///
        /// \returns	True if it succeeds; false otherwise.
        virtual bool TryGetTagAsDouble(const std::wstring& tag_name, double* value) = 0;

        /// Attempts to get the content of the specified tag from the sub-block metadata.
        /// The data is retrieved from the node "METADATA/Tags/<tag-name>".
        ///
        /// \param 		   	tag_name	The tag name.
        /// \param [in,out]	value   	If non-null, the content is put here
        ///
        /// \returns	True if it succeeds; false otherwise.
        virtual bool TryGetTagAsString(const std::wstring& tag_name, std::wstring* value) = 0;

        /// Attempts to get "stage position" from the sub-block metadata.
        /// This information is retrieved from the node "METADATA/Tags/StageXPosition" and "METADATA/Tags/StageYPosition".
        /// Note that X and Y need to be present in order to have this function return true.
        ///
        /// \param [in,out]	stage_position	If non-null, the stage position is put here.
        ///
        /// \returns	True if it succeeds; false otherwise.
        virtual bool TryGetStagePositionFromTags(std::tuple<double, double>* stage_position) = 0;

        // Delete copy constructor and copy assignment operator
        ISubBlockMetadataMetadataView(const ISubBlockMetadataMetadataView&) = delete;
        ISubBlockMetadataMetadataView& operator=(const ISubBlockMetadataMetadataView&) = delete;
        // Delete move constructor and move assignment operator
        ISubBlockMetadataMetadataView(ISubBlockMetadataMetadataView&&) = delete;
        ISubBlockMetadataMetadataView& operator=(ISubBlockMetadataMetadataView&&) = delete;
    };

    /// This interface is providing access to the sub-block metadata at XML-level via the IXmlNodeRead interface.
    /// Also, it has typed access to the metadata via the ISubBlockMetadataMetadataView interface.
    class ISubBlockMetadata : public IXmlNodeRead, public ISubBlockMetadataMetadataView
    {
    public:
        ISubBlockMetadata() = default;
        ~ISubBlockMetadata() override = default;

        /// Query if the sub-block metadata is well-formed and valid XML (and was parsed successfully).
        /// \returns	True if the XML is valid, false if not.
        virtual bool IsXmlValid() const = 0;

        /// Gets the sub-block metadata as an unprocessed UTF8-encoded XML-string.
        /// \returns	The XML.
        virtual std::string GetXml() const = 0;

        // Delete copy constructor and copy assignment operator
        ISubBlockMetadata(const ISubBlockMetadata&) = delete;
        ISubBlockMetadata& operator=(const ISubBlockMetadata&) = delete;

        // Delete move constructor and move assignment operator
        ISubBlockMetadata(ISubBlockMetadata&&) = delete;
        ISubBlockMetadata& operator=(ISubBlockMetadata&&) = delete;
    };

    struct SubBlockAttachmentMaskInfoGeneral
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t type_of_representation;
        size_t size_data;
        std::shared_ptr<const void> data;
    };

    struct SubBlockAttachmentMaskInfoUncompressedBitonalBitmap
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t stride;
        size_t size_data;
        std::shared_ptr<const void> data;
    };

    class ISubBlockAttachmentAccessor
    {
    public:
        struct ChunkInfo
        {
            libCZI::GUID  guid;   ///< The name of the chunk (if available).
            std::uint32_t offset; ///< The offset of the chunk in the attachment.
            std::uint32_t size;   ///< The size of the chunk in bytes.
        };

        ISubBlockAttachmentAccessor() = default;
        virtual ~ISubBlockAttachmentAccessor() = default;
        ISubBlockAttachmentAccessor(const ISubBlockAttachmentAccessor&) = delete;
        void operator=(const ISubBlockAttachmentAccessor&) = delete;

        virtual bool HasChunkContainer() const = 0;
        virtual bool EnumerateChunksInChunkContainer(const std::function<bool(int index, const ChunkInfo& info)>& functor_enum) const = 0;
        virtual libCZI::SubBlockAttachmentMaskInfoGeneral GetValidPixelMaskFromChunkContainer() const = 0;

        libCZI::SubBlockAttachmentMaskInfoUncompressedBitonalBitmap GetValidPixelMaskAsUncompressedBitonalBitmap() const
        {
            return libCZI::ISubBlockAttachmentAccessor::GetValidPixelMaskAsUncompressedBitonalBitmap(this);
        }

        std::shared_ptr<libCZI::IBitonalBitmapData> CreateBitonalBitmapFromMaskInfo() const
        {
            return libCZI::ISubBlockAttachmentAccessor::CreateBitonalBitmapFromMaskInfo(this);
        }

        static SubBlockAttachmentMaskInfoUncompressedBitonalBitmap  GetValidPixelMaskAsUncompressedBitonalBitmap(const ISubBlockAttachmentAccessor* accessor);
        static std::shared_ptr<libCZI::IBitonalBitmapData> CreateBitonalBitmapFromMaskInfo(const ISubBlockAttachmentAccessor* accessor);
    };


} // namespace libCZI
