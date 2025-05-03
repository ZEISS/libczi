// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "../inc/libCZIApi.h"
#include "sharedptrwrapper.h"
#include <string>
#include <sstream>
#include <libCZI.h>

#include "parameterhelpers.h"

#include <limits>
#include <algorithm>

using namespace libCZI;
using namespace std;

namespace
{
    IntRectInterop ConvertIntRect(const IntRect& rect)
    {
        IntRectInterop result;
        result.x = rect.x;
        result.y = rect.y;
        result.w = rect.w;
        result.h = rect.h;
        return result;
    }

    DimBoundsInterop ConvertDimBounds(const libCZI::IDimBounds* dim_bounds)
    {
        DimBoundsInterop result = {};
        int result_index = 0;
        for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
        {
            int start, size;
            if (dim_bounds->TryGetInterval(static_cast<DimensionIndex>(i), &start, &size))
            {
                int index = i - static_cast<int>(libCZI::DimensionIndex::MinDim);
                result.dimensions_valid |= (1 << index);
                result.start[result_index] = start;
                result.size[result_index] = size;
                ++result_index;
            }
        }

        return result;
    }

    void CopyFromAttachmentInfoToAttachmentInfoInterop(const libCZI::AttachmentInfo& source, AttachmentInfoInterop& destination)
    {
        static_assert(sizeof(libCZI::GUID) == sizeof(destination.guid), "sizeof(..) of the GUID-structure is found to differ from expectation");
        memcpy(destination.guid, &source.contentGuid, sizeof(libCZI::GUID));
        static_assert(sizeof(source.contentFileType) == sizeof(destination.content_file_type), "sizeof(..) of the contentFileType-field is found to differ from expectation");
        memcpy(destination.content_file_type, &source.contentFileType, sizeof(source.contentFileType));
        destination.name_overflow = ParameterHelpers::CopyUtf8StringTruncate(source.name, destination.name, sizeof(destination.name));
        if (destination.name_overflow)
        {
            // if the name is too long, we need to allocate memory for the name
            destination.name_in_case_of_overflow = ParameterHelpers::AllocString(source.name);
        }
        else
        {
            destination.name_in_case_of_overflow = nullptr;
        }
    }

    void CopyFromFileHeaderInfoToFileHeaderInfoInterop(const libCZI::FileHeaderInfo& source, FileHeaderInfoInterop& destination)
    {
        static_assert(sizeof(libCZI::GUID) == sizeof(destination.guid), "sizeof(..) of the GUID-structure is found to differ from expectation");
        memcpy(destination.guid, &source.fileGuid, sizeof(libCZI::GUID));
        destination.majorVersion = source.majorVersion;
        destination.minorVersion = source.minorVersion;
    }
}

void libCZI_Free(void* data)
{
    if (data == nullptr)
    {
        return;
    }

    ParameterHelpers::FreeMemory(data);
}

LibCZIApiErrorCode libCZI_AllocateMemory(std::uint64_t size, void** data)
{
    if (data == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *data = nullptr;
    if (size == 0)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    if (size > std::numeric_limits<size_t>::max())
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *data = ParameterHelpers::AllocateMemory(size);
    if (*data == nullptr)
    {
        return LibCZIApi_ErrorCode_OutOfMemory;
    }

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_GetLibCZIVersionInfo(LibCZIVersionInfoInterop* version_info)
{
    if (version_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    int major, minor, patch, tweak;
    GetLibCZIVersion(&major, &minor, &patch, &tweak);
    version_info->major = major;
    version_info->minor = minor;
    version_info->patch = patch;
    version_info->tweak = tweak;
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_GetLibCZIBuildInformation(LibCZIBuildInformationInterop* build_info)
{
    if (build_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    BuildInformation info;
    GetLibCZIBuildInformation(info);
    build_info->compilerIdentification = ParameterHelpers::AllocString(info.compilerIdentification);
    build_info->repositoryUrl = ParameterHelpers::AllocString(info.repositoryUrl);
    build_info->repositoryBranch = ParameterHelpers::AllocString(info.repositoryBranch);
    build_info->repositoryTag = ParameterHelpers::AllocString(info.repositoryTag);

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_CreateReader(CziReaderObjectHandle* reader_object)
{
    if (reader_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *reader_object = kInvalidObjectHandle;

    auto reader = CreateCZIReader();
    if (!reader)
    {
        return LibCZIApi_ErrorCode_OutOfMemory;
    }

    auto shared_czi_reader_wrapping_object = new SharedPtrWrapper<ICZIReader>{ reader };
    *reader_object = reinterpret_cast<CziReaderObjectHandle>(shared_czi_reader_wrapping_object);

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_ReaderOpen(CziReaderObjectHandle reader_object, const ReaderOpenInfoInterop* open_info)
{
    if (reader_object == kInvalidObjectHandle || open_info == nullptr || open_info->streamObject == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    auto shared_input_stream_wrapping_object = reinterpret_cast<SharedPtrWrapper<IStream>*>(open_info->streamObject);
    if (!shared_czi_reader_wrapping_object->IsValid() || !shared_input_stream_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        shared_czi_reader_wrapping_object->shared_ptr_->Open(shared_input_stream_wrapping_object->shared_ptr_, nullptr);
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_ReaderGetFileHeaderInfo(CziReaderObjectHandle reader_object, FileHeaderInfoInterop* file_header_info_interop)
{
    if (reader_object == kInvalidObjectHandle || file_header_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_file_header_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_file_header_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto& file_header_info_data = shared_file_header_wrapping_object->shared_ptr_->GetFileHeaderInfo();
        CopyFromFileHeaderInfoToFileHeaderInfoInterop(file_header_info_data, *file_header_info_interop);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetStatisticsSimple(CziReaderObjectHandle reader_object, SubBlockStatisticsInterop* statistics)
{
    if (reader_object == kInvalidObjectHandle || statistics == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto statistics_data = shared_czi_reader_wrapping_object->shared_ptr_->GetStatistics();
        statistics->sub_block_count = statistics_data.subBlockCount;
        statistics->min_m_index = statistics_data.minMindex;
        statistics->max_m_index = statistics_data.maxMindex;
        statistics->bounding_box = ConvertIntRect(statistics_data.boundingBox);
        statistics->bounding_box_layer0 = ConvertIntRect(statistics_data.boundingBoxLayer0Only);
        statistics->dim_bounds = ConvertDimBounds(&statistics_data.dimBounds);

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetStatisticsEx(CziReaderObjectHandle reader_object, SubBlockStatisticsInteropEx* statistics, std::int32_t* number_of_per_channel_bounding_boxes)
{
    if (reader_object == kInvalidObjectHandle || statistics == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const std::int32_t number_of_scenes_available = number_of_per_channel_bounding_boxes != nullptr ? *number_of_per_channel_bounding_boxes : 0;

        const auto statistics_data = shared_czi_reader_wrapping_object->shared_ptr_->GetStatistics();
        statistics->sub_block_count = statistics_data.subBlockCount;
        statistics->min_m_index = statistics_data.minMindex;
        statistics->max_m_index = statistics_data.maxMindex;
        statistics->bounding_box = ConvertIntRect(statistics_data.boundingBox);
        statistics->bounding_box_layer0 = ConvertIntRect(statistics_data.boundingBoxLayer0Only);
        statistics->dim_bounds = ConvertDimBounds(&statistics_data.dimBounds);

        int32_t i = 0;
        for (const auto& item : statistics_data.sceneBoundingBoxes)
        {
            if (i < number_of_scenes_available)
            {
                statistics->per_scenes_bounding_boxes[i].sceneIndex = item.first;
                statistics->per_scenes_bounding_boxes[i].bounding_box = ConvertIntRect(item.second.boundingBox);
                statistics->per_scenes_bounding_boxes[i].bounding_box_layer0_only = ConvertIntRect(item.second.boundingBoxLayer0);
                ++i;
            }
            else
            {
                break;
            }
        }

        statistics->number_of_per_scenes_bounding_boxes = i;

        if (number_of_per_channel_bounding_boxes != nullptr)
        {
            *number_of_per_channel_bounding_boxes = static_cast<int32_t>(statistics_data.sceneBoundingBoxes.size());
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetPyramidStatistics(CziReaderObjectHandle reader_object, char** pyramid_statistics_as_json)
{
    if (reader_object == kInvalidObjectHandle || pyramid_statistics_as_json == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto pyramid_statistics = shared_czi_reader_wrapping_object->shared_ptr_->GetPyramidStatistics();
        *pyramid_statistics_as_json = ParameterHelpers::AllocString(ParameterHelpers::ConvertLibCZIPyramidStatisticsToJsonString(pyramid_statistics));
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderReadSubBlock(CziReaderObjectHandle reader_object, std::int32_t index, SubBlockObjectHandle* sub_block_object)
{
    if (reader_object == kInvalidObjectHandle || sub_block_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        auto sub_block = shared_czi_reader_wrapping_object->shared_ptr_->ReadSubBlock(index);

        if (!sub_block)
        {
            *sub_block_object = kInvalidObjectHandle;
        }
        else
        {
            auto shared_sub_block_wrapping_object = new SharedPtrWrapper<ISubBlock>{ sub_block };
            *sub_block_object = reinterpret_cast<SubBlockObjectHandle>(shared_sub_block_wrapping_object);
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetMetadataSegment(CziReaderObjectHandle reader_object, MetadataSegmentObjectHandle* metadata_segment_object)
{
    if (reader_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        auto metadata_segment = shared_czi_reader_wrapping_object->shared_ptr_->ReadMetadataSegment();
        if (!metadata_segment)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_metadata_segment_wrapping_object = new SharedPtrWrapper<IMetadataSegment>{ metadata_segment };
        *metadata_segment_object = reinterpret_cast<MetadataSegmentObjectHandle>(shared_metadata_segment_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetAttachmentCount(CziReaderObjectHandle reader_object, std::int32_t* count)
{
    if (reader_object == kInvalidObjectHandle || count == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        *count = shared_czi_reader_wrapping_object->shared_ptr_->GetAttachmentCount();
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderGetAttachmentInfoFromDirectory(CziReaderObjectHandle reader_object, std::int32_t index, AttachmentInfoInterop* attachment_info_interop)
{
    if (reader_object == kInvalidObjectHandle || attachment_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        AttachmentInfo attachment_info;
        const bool b = shared_czi_reader_wrapping_object->shared_ptr_->TryGetAttachmentInfo(index, &attachment_info);
        if (!b)
        {
            return LibCZIApi_ErrorCode_IndexOutOfRange;
        }

        CopyFromAttachmentInfoToAttachmentInfoInterop(attachment_info, *attachment_info_interop);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReaderReadAttachment(CziReaderObjectHandle reader_object, std::int32_t index, AttachmentObjectHandle* attachment_object)
{
    if (reader_object == kInvalidObjectHandle && attachment_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        auto attachment = shared_czi_reader_wrapping_object->shared_ptr_->ReadAttachment(index);
        if (!attachment)
        {
            *attachment_object = kInvalidObjectHandle;
        }
        else
        {
            auto shared_attachment_wrapping_object = new SharedPtrWrapper<IAttachment>{ attachment };
            *attachment_object = reinterpret_cast<SubBlockObjectHandle>(shared_attachment_wrapping_object);
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseReader(CziReaderObjectHandle reader_object)
{
    if (reader_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_czi_reader_wrapping_object->Invalidate();
    delete shared_czi_reader_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_GetStreamClassesCount(std::int32_t* count)
{
    if (count == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *count = StreamsFactory::GetStreamClassesCount();
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_GetStreamClassInfo(std::int32_t index, InputStreamClassInfoInterop* input_stream_class_info)
{
    if (input_stream_class_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    StreamsFactory::StreamClassInfo stream_class_info;
    const bool b = StreamsFactory::GetStreamInfoForClass(index, stream_class_info);
    if (!b)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    input_stream_class_info->name = ParameterHelpers::AllocString(stream_class_info.class_name);
    input_stream_class_info->description = ParameterHelpers::AllocString(stream_class_info.short_description);

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_CreateInputStream(const char* stream_class_name, const char* creation_property_bag, const char* stream_identifier, InputStreamObjectHandle* stream_object)
{
    if (stream_class_name == nullptr || stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    StreamsFactory::CreateStreamInfo create_stream_info;

    if (creation_property_bag != nullptr && *creation_property_bag != '\0')
    {
        if (!ParameterHelpers::TryParseInputStreamCreationPropertyBag(creation_property_bag, &create_stream_info.property_bag))
        {
            return LibCZIApi_ErrorCode_InvalidArgument;
        }
    }

    create_stream_info.class_name = stream_class_name;

    try
    {
        shared_ptr<libCZI::IStream> stream = StreamsFactory::CreateStream(create_stream_info, stream_identifier);
        if (!stream)
        {
            // The documentation states that an empty shared_ptr is returned in case the class_name is not found.
            // All other kinds of errors are reported via exceptions.
            return LibCZIApi_ErrorCode_InvalidArgument;
        }

        auto shared_stream_wrapping_object = new SharedPtrWrapper<IStream>{ stream };
        *stream_object = reinterpret_cast<InputStreamObjectHandle>(shared_stream_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CreateInputStreamFromFileWide(const wchar_t* filename, InputStreamObjectHandle* stream_object)
{
    if (filename == nullptr || stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *stream_object = kInvalidObjectHandle;
    try
    {
        const auto stream = StreamsFactory::CreateDefaultStreamForFile(filename);
        if (!stream)
        {
            // note: CreateDefaultStreamForFile is documented to throw an exception in case of an error,
            //        so we don't expect to reach this point. However - if we do for whatever reasons, we better
            //        get out of here.
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_stream_wrapping_object = new SharedPtrWrapper<IStream>{ stream };
        *stream_object = reinterpret_cast<InputStreamObjectHandle>(shared_stream_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CreateInputStreamFromFileUTF8(const char* filename, InputStreamObjectHandle* stream_object)
{
    if (filename == nullptr || stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *stream_object = kInvalidObjectHandle;
    try
    {
        const auto stream = StreamsFactory::CreateDefaultStreamForFile(filename);
        if (!stream)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_stream_wrapping_object = new SharedPtrWrapper<IStream>{ stream };
        *stream_object = reinterpret_cast<InputStreamObjectHandle>(shared_stream_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

namespace
{
    /// This class implements the IStream-interface based on two externally provided
    /// functions.
    class InputStreamWrapper : public IStream
    {
    private:
        ExternalInputStreamStructInterop external_input_stream_struct_;
    public:
        InputStreamWrapper(const ExternalInputStreamStructInterop& input_stream_struct)
            : external_input_stream_struct_(input_stream_struct)
        {
        }

        void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override
        {
            if (ptrBytesRead != nullptr)
            {
                *ptrBytesRead = numeric_limits<uint64_t>::max();
            }

            ExternalStreamErrorInfoInterop error_info = {};
            auto return_code = this->external_input_stream_struct_.read_function(
                external_input_stream_struct_.opaque_handle1,
                external_input_stream_struct_.opaque_handle2,
                offset,
                pv,
                size,
                ptrBytesRead,
                &error_info);

            if (return_code != 0)
            {
                if (error_info.error_message != kInvalidObjectHandle)
                {
                    ostringstream error_message;
                    error_message << "Error reading from external input stream. Error code: " << error_info.error_code << ". Error message: \"" << reinterpret_cast<const char*>(error_info.error_message) << "\"";
                    libCZI_Free(error_info.error_message);
                    throw runtime_error(error_message.str());
                }
                else
                {
                    throw runtime_error("Error reading from external input stream.");
                }
            }
        }

        ~InputStreamWrapper() override
        {
            this->external_input_stream_struct_.close_function(this->external_input_stream_struct_.opaque_handle1, this->external_input_stream_struct_.opaque_handle2);
        }
    };

    /// This class implements the IOutputStream-interface based on two externally provided
    /// functions.
    class OutputStreamWrapper : public IOutputStream
    {
    private:
        ExternalOutputStreamStructInterop external_output_stream_struct_;
    public:
        OutputStreamWrapper(const ExternalOutputStreamStructInterop& output_stream_struct)
            : external_output_stream_struct_(output_stream_struct)
        {
        }

        void Write(std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* ptrBytesWritten) override
        {
            if (ptrBytesWritten != nullptr)
            {
                *ptrBytesWritten = numeric_limits<uint64_t>::max();
            }

            ExternalStreamErrorInfoInterop error_info = {};
            auto return_code = this->external_output_stream_struct_.write_function(
                this->external_output_stream_struct_.opaque_handle1,
                this->external_output_stream_struct_.opaque_handle2,
                offset,
                pv,
                size,
                ptrBytesWritten,
                &error_info);

            if (return_code != 0)
            {
                if (error_info.error_message != kInvalidObjectHandle)
                {
                    ostringstream error_message;
                    error_message << "Error reading from external input stream. Error code: " << error_info.error_code << ". Error message: \"" << reinterpret_cast<const char*>(error_info.error_message) << "\"";
                    throw runtime_error(error_message.str());
                }
                else
                {
                    throw runtime_error("Error reading from external input stream.");
                }
            }
        }

        ~OutputStreamWrapper() override
        {
            this->external_output_stream_struct_.close_function(this->external_output_stream_struct_.opaque_handle1, this->external_output_stream_struct_.opaque_handle2);
        }
    };
}

LibCZIApiErrorCode libCZI_CreateInputStreamFromExternal(const ExternalInputStreamStructInterop* external_input_stream_struct, InputStreamObjectHandle* stream_object)
{
    if (external_input_stream_struct == nullptr || stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *stream_object = kInvalidObjectHandle;
    auto stream = make_shared<InputStreamWrapper>(*external_input_stream_struct);
    if (!stream)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }

    auto shared_stream_wrapping_object = new SharedPtrWrapper<IStream>{ stream };
    *stream_object = reinterpret_cast<InputStreamObjectHandle>(shared_stream_wrapping_object);
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_ReleaseInputStream(InputStreamObjectHandle stream_object)
{
    if (stream_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_stream_wrapping_object = reinterpret_cast<SharedPtrWrapper<IStream>*>(stream_object);
    if (!shared_stream_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_stream_wrapping_object->Invalidate();
    delete shared_stream_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_SubBlockCreateBitmap(SubBlockObjectHandle sub_block_object, BitmapObjectHandle* bitmap_object)
{
    if (sub_block_object == kInvalidObjectHandle || bitmap_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_sub_block_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISubBlock>*>(sub_block_object);
    if (!shared_sub_block_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    *bitmap_object = kInvalidObjectHandle;

    try
    {
        auto bitmap = shared_sub_block_wrapping_object->shared_ptr_->CreateBitmap();
        if (!bitmap)
        {
            return LibCZIApi_ErrorCode_OutOfMemory;
        }

        auto shared_bitmap_wrapping_object = new SharedPtrWrapper<IBitmapData>{ bitmap };
        *bitmap_object = reinterpret_cast<BitmapObjectHandle>(shared_bitmap_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_SubBlockGetInfo(SubBlockObjectHandle sub_block_object, SubBlockInfoInterop* sub_block_info)
{
    if (sub_block_object == kInvalidObjectHandle || sub_block_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_sub_block_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISubBlock>*>(sub_block_object);
    if (!shared_sub_block_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto& sub_block_info_data = shared_sub_block_wrapping_object->shared_ptr_->GetSubBlockInfo();
        sub_block_info->compression_mode_raw = sub_block_info_data.compressionModeRaw;
        sub_block_info->pixel_type = static_cast<int32_t>(sub_block_info_data.pixelType);
        sub_block_info->coordinate = ParameterHelpers::ConvertIDimCoordinateToCoordinateInterop(&sub_block_info_data.coordinate);
        sub_block_info->logical_rect.x = sub_block_info_data.logicalRect.x;
        sub_block_info->logical_rect.y = sub_block_info_data.logicalRect.y;
        sub_block_info->logical_rect.w = sub_block_info_data.logicalRect.w;
        sub_block_info->logical_rect.h = sub_block_info_data.logicalRect.h;
        sub_block_info->physical_size.w = sub_block_info_data.physicalSize.w;
        sub_block_info->physical_size.h = sub_block_info_data.physicalSize.h;
        if (sub_block_info_data.IsMindexValid())
        {
            sub_block_info->m_index = sub_block_info_data.mIndex;
        }
        else
        {
            sub_block_info->m_index = numeric_limits<int32_t>::min();
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_SubBlockGetRawData(SubBlockObjectHandle sub_block_object, std::int32_t type, std::uint64_t* size, void* data)
{
    if (sub_block_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    ISubBlock::MemBlkType mem_blk_type;
    switch (type)
    {
    case 0:
        mem_blk_type = ISubBlock::MemBlkType::Data;
        break;
    case 1:
        mem_blk_type = ISubBlock::MemBlkType::Metadata;
        break;
    default:
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    if (size == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_sub_block_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISubBlock>*>(sub_block_object);
    if (!shared_sub_block_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    uint64_t size_of_destination = *size;
    try
    {
        size_t actual_size;
        const auto raw_data = shared_sub_block_wrapping_object->shared_ptr_->GetRawData(mem_blk_type, &actual_size);
        *size = actual_size;

        if (data != nullptr)
        {
            memcpy(data, raw_data.get(), min(size_of_destination, static_cast<uint64_t>(actual_size)));
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseSubBlock(SubBlockObjectHandle sub_block_object)
{
    if (sub_block_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_sub_block_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISubBlock>*>(sub_block_object);
    if (!shared_sub_block_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_sub_block_wrapping_object->Invalidate();
    delete shared_sub_block_wrapping_object;

    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_BitmapGetInfo(BitmapObjectHandle bitmap_object, BitmapInfoInterop* bitmap_info)
{
    if (bitmap_object == kInvalidObjectHandle || bitmap_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }
    auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(bitmap_object);
    if (!shared_bitmap_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    const auto extent = shared_bitmap_wrapping_object->shared_ptr_->GetSize();
    bitmap_info->pixelType = static_cast<int32_t>(shared_bitmap_wrapping_object->shared_ptr_->GetPixelType());
    bitmap_info->width = extent.w;
    bitmap_info->height = extent.h;

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_BitmapLock(BitmapObjectHandle bitmap_object, BitmapLockInfoInterop* lockInfo)
{
    if (bitmap_object == kInvalidObjectHandle || lockInfo == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }
    auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(bitmap_object);
    if (!shared_bitmap_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    auto lock_info = shared_bitmap_wrapping_object->shared_ptr_->Lock();
    lockInfo->ptrData = lock_info.ptrData;
    lockInfo->ptrDataRoi = lock_info.ptrDataRoi;
    lockInfo->stride = lock_info.stride;
    lockInfo->size = lock_info.size;
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_BitmapUnlock(BitmapObjectHandle bitmap_object)
{
    if (bitmap_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(bitmap_object);
    if (!shared_bitmap_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        shared_bitmap_wrapping_object->shared_ptr_->Unlock();
        return LibCZIApi_ErrorCode_OK;
    }
    catch (std::logic_error&)
    {
        // we get here if the bitmap was already unlocked (lock count == 0)
        return LibCZIApi_ErrorCode_LockUnlockSemanticViolated;
    }
    catch (std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseBitmap(BitmapObjectHandle bitmap_object)
{
    if (bitmap_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(bitmap_object);
    if (!shared_bitmap_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    if (shared_bitmap_wrapping_object->shared_ptr_.use_count() == 1)
    {
        // Ok, this means that we are holding the last reference to the bitmap-object, there it will
        //  be deleted when we delete the wrapping object. Note that there might still be the chance
        //  of a concurrent access to the bitmap-object, but we ignore this loophole for now. And
        //  then, the lock-count on the object should better be zero.
        if (shared_bitmap_wrapping_object->shared_ptr_->GetLockCount() != 0)
        {
            // If the lock-count is not zero, we have a problem. For the time being, we return with
            // an error and do not delete the bitmap-object. When the bitmap-object is actually deleted
            // (i.e. the destructor is called), then there is no recovery possible anymore.
            // This might not be water-tight in all conceivable situations, but it fits the bill for now.
            return LibCZIApi_ErrorCode_LockUnlockSemanticViolated;
        }
    }

    shared_bitmap_wrapping_object->Invalidate();
    delete shared_bitmap_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_BitmapCopyTo(BitmapObjectHandle bitmap_object, std::uint32_t width, std::uint32_t height, std::int32_t pixel_type, std::uint32_t stride, void* ptr)
{
    if (bitmap_object == kInvalidObjectHandle || ptr == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }
    auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(bitmap_object);
    if (!shared_bitmap_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    const auto extent = shared_bitmap_wrapping_object->shared_ptr_->GetSize();
    if (width != extent.w || height != extent.h)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    if (shared_bitmap_wrapping_object->shared_ptr_->GetPixelType() != static_cast<PixelType>(pixel_type))
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    size_t line_length = static_cast<size_t>(Utils::GetBytesPerPixel(static_cast<PixelType>(pixel_type))) * width;
    if (stride < line_length)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    ScopedBitmapLockerSP lckBm{ shared_bitmap_wrapping_object->shared_ptr_ };
    for (std::uint32_t y = 0; y < height; ++y)
    {
        const std::uint8_t* ptrSourceLine = static_cast<const std::uint8_t*>(lckBm.ptrDataRoi) + y * static_cast<size_t>(lckBm.stride);
        std::uint8_t* ptrDestinationLine = static_cast<std::uint8_t*>(ptr) + y * static_cast<size_t>(stride);
        memcpy(ptrDestinationLine, ptrSourceLine, line_length);
    }

    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************
LibCZIApiErrorCode libCZI_MetadataSegmentGetMetadataAsXml(MetadataSegmentObjectHandle metadata_segment_object, MetadataAsXmlInterop* metadata_as_xml_interop)
{
    if (metadata_segment_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_metadata_segment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IMetadataSegment>*>(metadata_segment_object);
    if (!shared_czi_metadata_segment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    size_t size;
    auto data = shared_czi_metadata_segment_wrapping_object->shared_ptr_->GetRawData(IMetadataSegment::XmlMetadata, &size);
    if (!data)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }

    // we want to ensure that the string is null-terminated, just in case - so if the data is not null-terminated, we will allocate
    //  one byte more
    size_t size_to_allocate = size;
    if (static_cast<const uint8_t*>(data.get())[size - 1] != '\0')
    {
        ++size_to_allocate;
    }

    uint8_t* allocated_memory = static_cast<uint8_t*>(ParameterHelpers::AllocateMemory(size_to_allocate));
    if (allocated_memory == nullptr)
    {
        return LibCZIApi_ErrorCode_OutOfMemory;
    }

    memcpy(allocated_memory, data.get(), size);
    allocated_memory[size_to_allocate - 1] = '\0';

    metadata_as_xml_interop->data = allocated_memory;
    metadata_as_xml_interop->size = size;

    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_MetadataSegmentGetCziDocumentInfo(MetadataSegmentObjectHandle metadata_segment_object, CziDocumentInfoHandle* czi_document_info_handle)
{
    if (metadata_segment_object == kInvalidObjectHandle || czi_document_info_handle == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_metadata_segment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IMetadataSegment>*>(metadata_segment_object);
    if (!shared_czi_metadata_segment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto czi_metadata = shared_czi_metadata_segment_wrapping_object->shared_ptr_->CreateMetaFromMetadataSegment();
        if (!czi_metadata)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        const auto czi_document_info = czi_metadata->GetDocumentInfo();

        *czi_document_info_handle = reinterpret_cast<CziDocumentInfoHandle>(new SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>{ czi_document_info });

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseMetadataSegment(MetadataSegmentObjectHandle metadata_segment_object)
{
    if (metadata_segment_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_metadata_segment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IMetadataSegment>*>(metadata_segment_object);
    if (!shared_czi_metadata_segment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_czi_metadata_segment_wrapping_object->Invalidate();
    delete shared_czi_metadata_segment_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_CziDocumentInfoGetScalingInfo(CziDocumentInfoHandle czi_document_info, ScalingInfoInterop* scaling_info_interop)
{
    if (czi_document_info == kInvalidObjectHandle || scaling_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto scaling_info = shared_czi_document_info_wrapping_object->shared_ptr_->GetScalingInfo();
        scaling_info_interop->scale_x = scaling_info.IsScaleXValid() ? scaling_info.scaleX : numeric_limits<double>::quiet_NaN();
        scaling_info_interop->scale_y = scaling_info.IsScaleYValid() ? scaling_info.scaleY : numeric_limits<double>::quiet_NaN();
        scaling_info_interop->scale_z = scaling_info.IsScaleZValid() ? scaling_info.scaleZ : numeric_limits<double>::quiet_NaN();
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CziDocumentInfoGetGeneralDocumentInfo(CziDocumentInfoHandle czi_document_info, void** general_document_info_json)
{
    if (czi_document_info == kInvalidObjectHandle || general_document_info_json == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }
    try
    {
        const auto general_document_info = shared_czi_document_info_wrapping_object->shared_ptr_->GetGeneralDocumentInfo();
        string json_text = ParameterHelpers::FormatGeneralDocumentInfoAsJsonString(general_document_info);
        *general_document_info_json = ParameterHelpers::AllocString(json_text);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CziDocumentInfoGetAvailableDimension(CziDocumentInfoHandle czi_document_info, std::uint32_t available_dimensions_count, std::uint32_t* available_dimensions)
{
    if (czi_document_info == kInvalidObjectHandle || available_dimensions == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }
    try
    {
        const auto available_dimensions_data = shared_czi_document_info_wrapping_object->shared_ptr_->GetDimensions();
        if (available_dimensions_count < available_dimensions_data.size() + 1)
        {
            return LibCZIApi_ErrorCode_InvalidArgument;
        }

        for (size_t i = 0; i < available_dimensions_data.size(); ++i)
        {
            available_dimensions[i] = static_cast<std::uint32_t>(available_dimensions_data[i]);
        }

        available_dimensions[available_dimensions_data.size()] = kDimensionInvalid;

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CziDocumentInfoGetDisplaySettings(CziDocumentInfoHandle czi_document_info, DisplaySettingsHandle* display_settings_handle)
{
    if (czi_document_info == kInvalidObjectHandle || display_settings_handle == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }


    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto display_settings = shared_czi_document_info_wrapping_object->shared_ptr_->GetDisplaySettings();
        *display_settings_handle = reinterpret_cast<DisplaySettingsHandle>(new SharedPtrWrapper<libCZI::IDisplaySettings>{ display_settings });
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CziDocumentInfoGetDimensionInfo(CziDocumentInfoHandle czi_document_info, std::uint32_t dimension_index, void** dimension_info_json)
{
    if (czi_document_info == kInvalidObjectHandle || dimension_info_json == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        switch (dimension_index)
        {
        case kDimensionZ:
        {
            const auto dimension_info = shared_czi_document_info_wrapping_object->shared_ptr_->GetDimensionZInfo();
            if (!dimension_info)
            {
                return LibCZIApi_ErrorCode_InvalidArgument;
            }

            string json_text = ParameterHelpers::FormatZDimensionInfoAsJsonString(dimension_info.get());
            *dimension_info_json = ParameterHelpers::AllocString(json_text);
        }

        break;

        case kDimensionT:
        {
            const auto dimension_info = shared_czi_document_info_wrapping_object->shared_ptr_->GetDimensionTInfo();
            if (!dimension_info)
            {
                return LibCZIApi_ErrorCode_InvalidArgument;
            }

            string json_text = ParameterHelpers::FormatTDimensionInfoAsJsonString(dimension_info.get());
            *dimension_info_json = ParameterHelpers::AllocString(json_text);
        }

        break;

        case kDimensionC:
        {
            const auto dimension_info = shared_czi_document_info_wrapping_object->shared_ptr_->GetDimensionChannelsInfo();
            if (!dimension_info)
            {
                return LibCZIApi_ErrorCode_InvalidArgument;
            }

            string json_text = ParameterHelpers::FormatCDimensionInfoAsJsonString(dimension_info.get());
            *dimension_info_json = ParameterHelpers::AllocString(json_text);
        }

        break;

        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseCziDocumentInfo(CziDocumentInfoHandle czi_document_info)
{
    if (czi_document_info == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    const auto shared_czi_document_info_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo>*>(czi_document_info);
    if (!shared_czi_document_info_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_czi_document_info_wrapping_object->Invalidate();
    delete shared_czi_document_info_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_AttachmentGetInfo(AttachmentObjectHandle attachment_object, AttachmentInfoInterop* attachment_info)
{
    if (attachment_object == kInvalidObjectHandle || attachment_info == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_attachment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IAttachment>*>(attachment_object);
    if (!shared_attachment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto& attachment_info_data = shared_attachment_wrapping_object->shared_ptr_->GetAttachmentInfo();
        CopyFromAttachmentInfoToAttachmentInfoInterop(attachment_info_data, *attachment_info);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

/// Copy the raw data from the specified attachment object to the specified memory buffer. The value of the 'size' parameter
/// on input is the size of the buffer pointed to by 'data'. On output, the value of 'size' is the actual size of the data. At most
/// the initial value of 'size' bytes are copied to the buffer. If the initial value of 'size' is zero (0) or 'data' is null, then 
/// no data is copied. 
/// \param          attachment_object   The attachment object.
/// \param [in,out] size                On input, the size of the memory block pointed to by 'data', on output the actual size of the available data.
/// \param [out]    data                Pointer where the data is to be copied to. At most the initial content of 'size' bytes are copied.
///
/// \returns    An error-code indicating success or failure of the operation.
LibCZIApiErrorCode libCZI_AttachmentGetRawData(AttachmentObjectHandle attachment_object, std::uint64_t* size, void* data)
{
    if (attachment_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    if (size == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_attachment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IAttachment>*>(attachment_object);
    if (!shared_attachment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    size_t size_of_destination = *size;
    try
    {
        size_t actual_size;
        const auto raw_data = shared_attachment_wrapping_object->shared_ptr_->GetRawData(&actual_size);
        *size = actual_size;
        if (data != nullptr)
        {
            memcpy(data, raw_data.get(), min(size_of_destination, actual_size));
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseAttachment(AttachmentObjectHandle attachment_object)
{
    if (attachment_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_attachment_wrapping_object = reinterpret_cast<SharedPtrWrapper<IAttachment>*>(attachment_object);
    if (!shared_attachment_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_attachment_wrapping_object->Invalidate();
    delete shared_attachment_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_CreateOutputStreamForFileUTF8(const char* filename, bool overwrite, OutputStreamObjectHandle* output_stream_object)
{
    if (filename == nullptr || output_stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *output_stream_object = kInvalidObjectHandle;
    try
    {
        const auto stream = libCZI::CreateOutputStreamForFileUtf8(filename, overwrite);
        if (!stream)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_output_stream_wrapping_object = new SharedPtrWrapper<IOutputStream>{ stream };
        *output_stream_object = reinterpret_cast<OutputStreamObjectHandle>(shared_output_stream_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CreateOutputStreamForFileWide(const wchar_t* filename, bool overwrite, OutputStreamObjectHandle* output_stream_object)
{
    if (filename == nullptr || output_stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *output_stream_object = kInvalidObjectHandle;
    try
    {
        const auto stream = libCZI::CreateOutputStreamForFile(filename, overwrite);
        if (!stream)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_output_stream_wrapping_object = new SharedPtrWrapper<IOutputStream>{ stream };
        *output_stream_object = reinterpret_cast<OutputStreamObjectHandle>(shared_output_stream_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseOutputStream(OutputStreamObjectHandle output_stream_object)
{
    if (output_stream_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_output_stream_wrapping_object = reinterpret_cast<SharedPtrWrapper<IOutputStream>*>(output_stream_object);
    if (!shared_output_stream_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_output_stream_wrapping_object->Invalidate();
    delete shared_output_stream_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_CreateOutputStreamFromExternal(const ExternalOutputStreamStructInterop* external_output_stream_struct, OutputStreamObjectHandle* output_stream_object)
{
    if (external_output_stream_struct == nullptr || output_stream_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *output_stream_object = kInvalidObjectHandle;
    auto stream = make_shared<OutputStreamWrapper>(*external_output_stream_struct);
    if (!stream)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }

    auto shared_output_stream_wrapping_object = new SharedPtrWrapper<IOutputStream>{ stream };
    *output_stream_object = reinterpret_cast<OutputStreamObjectHandle>(shared_output_stream_wrapping_object);
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_CreateWriter(CziWriterObjectHandle* writer_object, const char* options)
{
    if (writer_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    *writer_object = kInvalidObjectHandle;

    CZIWriterOptions writer_options;
    const bool writer_options_valid = ParameterHelpers::TryParseCZIWriterOptions(options, writer_options);
    try
    {
        const auto writer = writer_options_valid ? libCZI::CreateCZIWriter(&writer_options) : libCZI::CreateCZIWriter();
        if (!writer)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto shared_writer_wrapping_object = new SharedPtrWrapper<ICziWriter>{ writer };
        *writer_object = reinterpret_cast<CziWriterObjectHandle>(shared_writer_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseWriter(CziWriterObjectHandle writer_object)
{
    if (writer_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_writer_wrapping_object->Invalidate();
    delete shared_writer_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

LibCZIApiErrorCode libCZI_WriterClose(CziWriterObjectHandle writer_object)
{
    if (writer_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        shared_writer_wrapping_object->shared_ptr_->Close();
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_WriterCreate(CziWriterObjectHandle writer_object, OutputStreamObjectHandle output_stream_object, const char* parameters)
{
    if (writer_object == kInvalidObjectHandle || output_stream_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    auto shared_output_stream_wrapping_object = reinterpret_cast<SharedPtrWrapper<IOutputStream>*>(output_stream_object);
    if (!shared_output_stream_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    std::shared_ptr<libCZI::ICziWriterInfo> czi_writer_info;
    const bool czi_writer_info_valid = ParameterHelpers::TryParseCZIWriterInfo(parameters, czi_writer_info);
    try
    {
        if (czi_writer_info_valid)
        {
            shared_writer_wrapping_object->shared_ptr_->Create(shared_output_stream_wrapping_object->shared_ptr_, czi_writer_info);
        }
        else
        {
            shared_writer_wrapping_object->shared_ptr_->Create(shared_output_stream_wrapping_object->shared_ptr_, nullptr);
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

namespace
{
    void FillOutAddSubBlockInfoBase(const AddSubBlockInfoInterop* add_sub_block_info, AddSubBlockInfoBase& add_sub_block_info_base)
    {
        add_sub_block_info_base.coordinate = ParameterHelpers::ConvertCoordinateInteropToDimCoordinate(add_sub_block_info->coordinate);
        add_sub_block_info_base.mIndexValid = add_sub_block_info->m_index_valid != 0;
        if (add_sub_block_info_base.mIndexValid)
        {
            add_sub_block_info_base.mIndex = add_sub_block_info->m_index;
        }

        add_sub_block_info_base.x = add_sub_block_info->x;
        add_sub_block_info_base.y = add_sub_block_info->y;
        add_sub_block_info_base.logicalWidth = add_sub_block_info->logical_width;
        add_sub_block_info_base.logicalHeight = add_sub_block_info->logical_height;
        add_sub_block_info_base.physicalWidth = add_sub_block_info->physical_width;
        add_sub_block_info_base.physicalHeight = add_sub_block_info->physical_height;
        add_sub_block_info_base.PixelType = static_cast<PixelType>(add_sub_block_info->pixel_type); // TODO(JBL): check validity
        add_sub_block_info_base.SetCompressionMode(CompressionMode::UnCompressed);
    }


    void WriterAddSubBlockUncompressed(const shared_ptr<ICziWriter>& writer, const AddSubBlockInfoInterop* add_sub_block_info)
    {
        AddSubBlockInfoStridedBitmap add_sub_block_info_strided_bitmap;
        add_sub_block_info_strided_bitmap.Clear();

        FillOutAddSubBlockInfoBase(add_sub_block_info, add_sub_block_info_strided_bitmap);

        add_sub_block_info_strided_bitmap.strideBitmap = add_sub_block_info->stride;
        add_sub_block_info_strided_bitmap.ptrBitmap = add_sub_block_info->data;

        add_sub_block_info_strided_bitmap.ptrSbBlkMetadata = add_sub_block_info->metadata;
        add_sub_block_info_strided_bitmap.sbBlkMetadataSize = add_sub_block_info->size_metadata;

        add_sub_block_info_strided_bitmap.ptrSbBlkAttachment = add_sub_block_info->attachment;
        add_sub_block_info_strided_bitmap.sbBlkAttachmentSize = add_sub_block_info->size_attachment;

        writer->SyncAddSubBlock(add_sub_block_info_strided_bitmap);
    }

    void WriterAddSubBlockCompressed(const shared_ptr<ICziWriter>& writer, const AddSubBlockInfoInterop* add_sub_block_info)
    {
        AddSubBlockInfoMemPtr add_sub_block_info_mem_ptr;
        add_sub_block_info_mem_ptr.Clear();

        FillOutAddSubBlockInfoBase(add_sub_block_info, add_sub_block_info_mem_ptr);

        add_sub_block_info_mem_ptr.ptrData = add_sub_block_info->data;
        add_sub_block_info_mem_ptr.dataSize = add_sub_block_info->size_data;
        add_sub_block_info_mem_ptr.ptrSbBlkMetadata = add_sub_block_info->metadata;
        add_sub_block_info_mem_ptr.sbBlkMetadataSize = add_sub_block_info->size_metadata;
        add_sub_block_info_mem_ptr.ptrSbBlkAttachment = add_sub_block_info->attachment;
        add_sub_block_info_mem_ptr.sbBlkAttachmentSize = add_sub_block_info->size_attachment;

        writer->SyncAddSubBlock(add_sub_block_info_mem_ptr);
    }
}

LibCZIApiErrorCode libCZI_WriterAddSubBlock(CziWriterObjectHandle writer_object, const AddSubBlockInfoInterop* add_sub_block_info_interop)
{
    if (writer_object == kInvalidObjectHandle || add_sub_block_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto compression_mode = libCZI::Utils::CompressionModeFromRawCompressionIdentifier(add_sub_block_info_interop->compression_mode_raw);
        if (compression_mode == CompressionMode::UnCompressed)
        {
            WriterAddSubBlockUncompressed(shared_writer_wrapping_object->shared_ptr_, add_sub_block_info_interop);
        }
        else
        {
            WriterAddSubBlockCompressed(shared_writer_wrapping_object->shared_ptr_, add_sub_block_info_interop);
        }

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_WriterAddAttachment(CziWriterObjectHandle writer_object, const AddAttachmentInfoInterop* add_attachment_info_interop)
{
    if (writer_object == kInvalidObjectHandle || add_attachment_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    AddAttachmentInfo add_attachment_info;
    add_attachment_info.Clear();
    memcpy(&add_attachment_info.contentGuid, add_attachment_info_interop->guid, sizeof(add_attachment_info.contentGuid));
    static_assert(sizeof(add_attachment_info.contentFileType) == sizeof(add_attachment_info_interop->contentFileType), "sizes are expected to be equal");
    memcpy(add_attachment_info.contentFileType, add_attachment_info_interop->contentFileType, sizeof(add_attachment_info.contentFileType));
    static_assert(sizeof(add_attachment_info.name) == sizeof(add_attachment_info_interop->name), "sizes are expected to be equal");
    memcpy(add_attachment_info.name, add_attachment_info_interop->name, sizeof(add_attachment_info.name));

    add_attachment_info.dataSize = add_attachment_info_interop->size_attachment_data;
    add_attachment_info.ptrData = add_attachment_info_interop->attachment_data;

    try
    {
        shared_writer_wrapping_object->shared_ptr_->SyncAddAttachment(add_attachment_info);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_WriterWriteMetadata(CziWriterObjectHandle writer_object, const WriteMetadataInfoInterop* write_metadata_info_interop)
{
    if (writer_object == kInvalidObjectHandle || write_metadata_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_writer_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICziWriter>*>(writer_object);
    if (!shared_writer_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    WriteMetadataInfo write_metadata_info;
    write_metadata_info.Clear();
    write_metadata_info.szMetadataSize = write_metadata_info_interop->size_metadata;
    write_metadata_info.szMetadata = (const char*)write_metadata_info_interop->metadata;

    try
    {
        shared_writer_wrapping_object->shared_ptr_->SyncWriteMetadata(write_metadata_info);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_CreateSingleChannelTileAccessor(CziReaderObjectHandle reader_object, SingleChannelScalingTileAccessorObjectHandle* accessor_object)
{
    if (reader_object == kInvalidObjectHandle || accessor_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_czi_reader_wrapping_object = reinterpret_cast<SharedPtrWrapper<ICZIReader>*>(reader_object);
    if (!shared_czi_reader_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto accessor = shared_czi_reader_wrapping_object->shared_ptr_->CreateSingleChannelScalingTileAccessor();
        if (!accessor)
        {
            return LibCZIApi_ErrorCode_UnspecifiedError;
        }

        auto accessor_wrapping_object = new SharedPtrWrapper<ISingleChannelScalingTileAccessor>{ accessor };
        *accessor_object = reinterpret_cast<SingleChannelScalingTileAccessorObjectHandle>(accessor_wrapping_object);
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_SingleChannelTileAccessorCalcSize(SingleChannelScalingTileAccessorObjectHandle accessor_object, const IntRectInterop* roi, float zoom, IntSizeInterop* size)
{
    if (accessor_object == kInvalidObjectHandle || roi == nullptr || size == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_accessor_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISingleChannelScalingTileAccessor>*>(accessor_object);
    if (!shared_accessor_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    IntRect libczi_roi{ roi->x, roi->y, roi->w, roi->h };
    try
    {
        const auto size_data = shared_accessor_wrapping_object->shared_ptr_->CalcSize(libczi_roi, zoom);
        size->w = size_data.w;
        size->h = size_data.h;
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_SingleChannelTileAccessorGet(SingleChannelScalingTileAccessorObjectHandle accessor_object, const CoordinateInterop* coordinate, const IntRectInterop* roi, float zoom, const AccessorOptionsInterop* options, BitmapObjectHandle* bitmap_object)
{
    if (accessor_object == kInvalidObjectHandle || coordinate == nullptr || roi == nullptr || bitmap_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_accessor_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISingleChannelScalingTileAccessor>*>(accessor_object);
    if (!shared_accessor_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    IntRect libczi_roi{ roi->x, roi->y, roi->w, roi->h };
    const auto libczi_coordinate = ParameterHelpers::ConvertCoordinateInteropToDimCoordinate(*coordinate);
    const auto libczi_options = ParameterHelpers::ConvertSingleChannelScalingTileAccessorOptionsInteropToLibCZI(options);

    try
    {
        auto result_bitmap = shared_accessor_wrapping_object->shared_ptr_->Get(libczi_roi, &libczi_coordinate, zoom, &libczi_options);

        auto shared_bitmap_wrapping_object = new SharedPtrWrapper<IBitmapData>{ result_bitmap };
        *bitmap_object = reinterpret_cast<BitmapObjectHandle>(shared_bitmap_wrapping_object);

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::bad_alloc&)
    {
        return LibCZIApi_ErrorCode_OutOfMemory;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseCreateSingleChannelTileAccessor(SingleChannelScalingTileAccessorObjectHandle accessor_object)
{
    if (accessor_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_accessor_wrapping_object = reinterpret_cast<SharedPtrWrapper<ISingleChannelScalingTileAccessor>*>(accessor_object);
    if (!shared_accessor_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_accessor_wrapping_object->Invalidate();
    delete shared_accessor_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_CompositorDoMultiChannelComposition(std::int32_t channelCount, const BitmapObjectHandle* source_bitmaps, const CompositionChannelInfoInterop* channel_info, BitmapObjectHandle* bitmap_object)
{
    if (channelCount <= 0)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    if (source_bitmaps == nullptr || channel_info == nullptr || bitmap_object == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    unique_ptr<IBitmapData* []> source_bitmaps_data{ new IBitmapData * [channelCount] };
    unique_ptr<Compositors::ChannelInfo[]> channel_info_data{ new Compositors::ChannelInfo[channelCount] };

    for (int i = 0; i < channelCount; ++i)
    {
        auto shared_bitmap_wrapping_object = reinterpret_cast<SharedPtrWrapper<IBitmapData>*>(source_bitmaps[i]);
        if (!shared_bitmap_wrapping_object->IsValid())
        {
            return LibCZIApi_ErrorCode_InvalidHandle;
        }

        source_bitmaps_data[i] = shared_bitmap_wrapping_object->shared_ptr_.get();
        channel_info_data[i] = ParameterHelpers::ConvertCompositionChannelInfoInteropToChannelInfo(channel_info + i);
    }

    try
    {
        auto composed_bitmap = Compositors::ComposeMultiChannel_Bgr24(channelCount, source_bitmaps_data.get(), channel_info_data.get());
        auto shared_bitmap_wrapping_object = new SharedPtrWrapper<IBitmapData>{ composed_bitmap };
        *bitmap_object = reinterpret_cast<BitmapObjectHandle>(shared_bitmap_wrapping_object);
        return LibCZIApi_ErrorCode_OK;

    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_CompositorFillOutCompositionChannelInfoInterop(DisplaySettingsHandle display_settings_handle, int channel_index, bool sixteen_or_eight_bits_lut, CompositionChannelInfoInterop* composition_channel_info_interop)
{
    if (display_settings_handle == kInvalidObjectHandle || composition_channel_info_interop == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_display_settings_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::IDisplaySettings>*>(display_settings_handle);
    if (!shared_display_settings_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        ParameterHelpers::FillOutCompositionChannelInfoFromDisplaySettings(
            shared_display_settings_wrapping_object->shared_ptr_.get(),
            channel_index,
            sixteen_or_eight_bits_lut,
            *composition_channel_info_interop);

        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_DisplaySettingsGetChannelDisplaySettings(DisplaySettingsHandle display_settings_handle, int channel_id, ChannelDisplaySettingsHandle* channel_display_setting)
{
    if (display_settings_handle == kInvalidObjectHandle || channel_display_setting == nullptr)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_display_settings_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::IDisplaySettings>*>(display_settings_handle);
    if (!shared_display_settings_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    try
    {
        const auto channel_display_settings = shared_display_settings_wrapping_object->shared_ptr_->GetChannelDisplaySettings(channel_id);
        if (!channel_display_settings)
        {
            return LibCZIApi_ErrorCode_IndexOutOfRange;
        }

        *channel_display_setting = reinterpret_cast<ChannelDisplaySettingsHandle>(new SharedPtrWrapper<libCZI::IChannelDisplaySetting>{ channel_display_settings });
        return LibCZIApi_ErrorCode_OK;
    }
    catch (const std::exception&)
    {
        return LibCZIApi_ErrorCode_UnspecifiedError;
    }
}

LibCZIApiErrorCode libCZI_ReleaseDisplaySettings(DisplaySettingsHandle writer_object)
{
    if (writer_object == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_display_settings_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::IDisplaySettings>*>(writer_object);
    if (!shared_display_settings_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_display_settings_wrapping_object->Invalidate();
    delete shared_display_settings_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}

//****************************************************************************************************

LibCZIApiErrorCode libCZI_ReleaseChannelDisplaySettings(ChannelDisplaySettingsHandle channel_display_settings_handle)
{
    if (channel_display_settings_handle == kInvalidObjectHandle)
    {
        return LibCZIApi_ErrorCode_InvalidArgument;
    }

    auto shared_channel_display_settings_wrapping_object = reinterpret_cast<SharedPtrWrapper<libCZI::IChannelDisplaySetting>*>(channel_display_settings_handle);
    if (!shared_channel_display_settings_wrapping_object->IsValid())
    {
        return LibCZIApi_ErrorCode_InvalidHandle;
    }

    shared_channel_display_settings_wrapping_object->Invalidate();
    delete shared_channel_display_settings_wrapping_object;
    return LibCZIApi_ErrorCode_OK;
}
