// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <libCZI.h>

constexpr uint32_t kMagicInvalid = 0;
constexpr uint32_t kMagicICziReader = 0x48EE87D2u;
constexpr uint32_t kMagicISubBlock = 0x27AD4774u;
constexpr uint32_t kMagicIStream = 0xF2BB50D4u;
constexpr uint32_t kMagicIBitmapData = 0x9155FC95u;
constexpr uint32_t kMagicIMetadataSegment = 0x7A3D4A3Au;
constexpr uint32_t kMagicIAttachment = 0x3A4A3D7Au;
constexpr uint32_t kMagicIOutputStream = 0x8020FAA3u;
constexpr uint32_t kMagicICziWriter = 0xF8384829u;
constexpr uint32_t kMagicISingleChannelScalingTileAccessor = 0xE34C94D7u;
constexpr uint32_t kMagicICziMultiDimensionDocumentInfo = 0x3A4A3D7Au;
constexpr uint32_t kMagicIDisplaySettings = 0xE67C5A4Cu;
constexpr uint32_t kMagicIChannelDisplaySetting = 0x88265932u;

// In this file we define a generic template class that can be used to wrap a shared pointer to an object.
// This is used to provide a handle to an object, and we use a magic value to check if the handle is still valid.

/// This class is used to represent a shared pointer, or this is what the handles we are providing are pointing to. This class
/// contains a magic value, which is used to check if the handle is still valid. When the handle is created, the magic value
/// is set to a specific value. When the handle is destroyed, the magic value is set to 0. If the magic value is not the
/// expected value, the handle is invalid.
/// We use template specialization to set the magic value for each type of object, and we need to use some template magic
/// to make this work. We use partial template specialization to set the magic value for each type of object. 
///
/// \typeparam  ClassT      The type of the object to be stored here.
/// \typeparam  MagicValueN The magic value to be used for this type of object.
template <typename ClassT, std::uint32_t MagicValueN>
struct SharedPtrWrapperBase
{
    explicit SharedPtrWrapperBase(std::shared_ptr<ClassT> shared_ptr) : magic_(MagicValueN), shared_ptr_(std::move(shared_ptr)) {}

    ~SharedPtrWrapperBase()
    {
        this->Invalidate();
    }

    /// Query if this object is valid. This checks if the magic value is the expected value. If this is not the case,
    /// this means that either the value has been invalided, or that the pointer is bogus.
    ///
    /// \returns    True if valid, false if not.
    [[nodiscard]] bool IsValid() const { return this->magic_ == MagicValueN; }

    /// Invalidates the magic value. This is used when the handle is destroyed.
    void Invalidate() { this->magic_ = kMagicInvalid; }

    std::uint32_t magic_;
    std::shared_ptr<ClassT> shared_ptr_;
};

/// This class is used to represent a pointer (to an object), or this is what the handles we are providing is pointing to. This class
/// contains a magic value, which is used to check if the handle is still valid. When the handle is created, the magic value
/// is set to a specific value. When the handle is destroyed, the magic value is set to 0. If the magic value is not the
/// expected value, the handle is invalid.
/// We use template specialization to set the magic value for each type of object, and we need to use some template magic
/// to make this work. We use partial template specialization to set the magic value for each type of object. 
/// Note that the pointer is deleted when the handle is destroyed, so there is a transfer of ownership of the pointer/object.
///
/// \typeparam  ClassT      The type of the object to be stored here.
/// \typeparam  MagicValueN The magic value to be used for this type of object.
template <typename ClassT, std::uint32_t MagicValueN>
struct PtrWrapperBase
{
    explicit PtrWrapperBase(ClassT* ptr) : magic_(MagicValueN), ptr_(ptr) {}

    ~PtrWrapperBase()
    {
        this->Invalidate();
        delete this->ptr_;
        this->ptr_ = nullptr;
    }

    /// Query if this object is valid. This checks if the magic value is the expected value. If this is not the case,
    /// this means that either the value has been invalided, or that the pointer is bogus.
    ///
    /// \returns    True if valid, false if not.
    [[nodiscard]] bool IsValid() const { return this->magic_ == MagicValueN; }

    /// Invalidates the magic value. This is used when the handle is destroyed.
    void Invalidate() { this->magic_ = kMagicInvalid; }

    std::uint32_t magic_;
    ClassT* ptr_;
};

/// SharedPtrWrapper is a generic template class that inherits from SharedPtrWrapperBase.
/// This uses an invalid magic value of 0 for all types, and we use partial template specialization to set the magic value
/// for each type of object we want to support.
/// Note that the constructor is private, so that we can only create partial specialization of this class.
///
/// \typeparam  t   Generic type parameter.
template <typename t>
struct SharedPtrWrapper :public SharedPtrWrapperBase<t, 0>
{
private:
    explicit SharedPtrWrapper(std::shared_ptr<t> shared_ptr) : SharedPtrWrapperBase<t, 0>(std::move(shared_ptr)) {}
};

/// PtrWrapper is a generic template class that inherits from PtrWrapperBase.
/// This uses an invalid magic value of 0 for all types, and we use partial template specialization to set the magic value
/// for each type of object we want to support.
/// Note that the constructor is private, so that we can only create partial specialization of this class.
///
/// \typeparam  t   Generic type parameter.
template <typename t>
struct PtrWrapper :public PtrWrapperBase<t, 0>
{
private:
    explicit PtrWrapper(t* ptr) : PtrWrapperBase<t, 0>(ptr) {}
};

/// Partial template specialization for ICZIReader objects.
template <>
struct SharedPtrWrapper<libCZI::ICZIReader> : SharedPtrWrapperBase<libCZI::ICZIReader, kMagicICziReader>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::ICZIReader> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for ISubBlock objects.
template <>
struct SharedPtrWrapper<libCZI::ISubBlock> : SharedPtrWrapperBase<libCZI::ISubBlock, kMagicISubBlock>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::ISubBlock> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for IStream objects (i.e. input stream objects).
template <>
struct SharedPtrWrapper<libCZI::IStream> : SharedPtrWrapperBase<libCZI::IStream, kMagicIStream>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IStream> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for IBitmapData objects.
template <>
struct SharedPtrWrapper<libCZI::IBitmapData> : SharedPtrWrapperBase<libCZI::IBitmapData, kMagicIBitmapData>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IBitmapData> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for IMetadataSegment objects.
template <>
struct SharedPtrWrapper<libCZI::IMetadataSegment> : SharedPtrWrapperBase<libCZI::IMetadataSegment, kMagicIMetadataSegment>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IMetadataSegment> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for IAttachment objects.
template <>
struct SharedPtrWrapper<libCZI::IAttachment> : SharedPtrWrapperBase<libCZI::IAttachment, kMagicIAttachment>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IAttachment> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for IOutputStream objects (i.e. output stream objects).
template <>
struct SharedPtrWrapper<libCZI::IOutputStream> : SharedPtrWrapperBase<libCZI::IOutputStream, kMagicIOutputStream>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IOutputStream> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for CziWriter objects
template <>
struct SharedPtrWrapper<libCZI::ICziWriter> : SharedPtrWrapperBase<libCZI::ICziWriter, kMagicICziWriter>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::ICziWriter> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {}
};

/// Partial template specialization for ISingleChannelScalingTileAccessor objects
template <>
struct SharedPtrWrapper<libCZI::ISingleChannelScalingTileAccessor> : SharedPtrWrapperBase<libCZI::ISingleChannelScalingTileAccessor, kMagicISingleChannelScalingTileAccessor>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::ISingleChannelScalingTileAccessor> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {
    }
};

/// Partial template specialization for ICziMultiDimensionDocumentInfo objects
template <>
struct SharedPtrWrapper<libCZI::ICziMultiDimensionDocumentInfo> : SharedPtrWrapperBase<libCZI::ICziMultiDimensionDocumentInfo, kMagicICziMultiDimensionDocumentInfo>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::ICziMultiDimensionDocumentInfo> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {
    }
};

/// Partial template specialization for IDisplaySettings objects.
template <>
struct SharedPtrWrapper<libCZI::IDisplaySettings> : SharedPtrWrapperBase<libCZI::IDisplaySettings, kMagicIDisplaySettings>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IDisplaySettings> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {
    }
};

/// Partial template specialization for IChannelDisplaySetting objects.
template <>
struct SharedPtrWrapper<libCZI::IChannelDisplaySetting> : SharedPtrWrapperBase<libCZI::IChannelDisplaySetting, kMagicIChannelDisplaySetting>
{
    explicit SharedPtrWrapper(std::shared_ptr<libCZI::IChannelDisplaySetting> shared_ptr) :
        SharedPtrWrapperBase(std::move(shared_ptr)) {
    }
};
