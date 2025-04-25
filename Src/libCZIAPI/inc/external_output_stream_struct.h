// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This structure contains information about externally provided functions for writing data to an output stream,
/// and it is used to construct a stream-object to be used with libCZI.
/// Note on lifetime: The function pointers must remain valid until the function 'close_function' is called. The lifetime
/// may extend beyond calling the 'libCZI_ReleaseOutputStream' function for the corresponding stream-object.
struct ExternalOutputStreamStructInterop
{
    /// A user parameter which is passed to the callback function.
    std::uintptr_t opaque_handle1;

    /// A user parameter which is passed to the callback function.
    std::uintptr_t opaque_handle2;

    /// Function pointer used to write data into the stream.
    /// This function might be called from an arbitrary thread, and it may be called concurrently from multiple threads.
    /// A 0 as return value indicates successful operation. A non-zero value indicates a non-recoverable error.
    /// In case of an error, the error_info parameter must be filled with the error information.
    ///
    /// \param          opaque_handle1    The value of the opaque_handle1 field of the ExternalOutputStreamStructInterop.
    /// \param          opaque_handle2    The value of the opaque_handle2 field of the ExternalOutputStreamStructInterop.
    /// \param          offset            The offset in the stream where to start writing to.
    /// \param [in]     pv                Pointer to the buffer with the data to be written.
    /// \param          size              The size of the buffer (and the number of bytes to be written to the stream).
    /// \param [out]    out_bytes_written If non-null, the number of bytes that actually could be written is to be put here.
    /// \param [out]    error_info      If non-null, in case of an error (i.e. return value <>0), this parameter may be used to report additional error information.
    std::int32_t(*write_function)(
        std::uintptr_t opaque_handle1,
        std::uintptr_t opaque_handle2,
        std::uint64_t offset,
        const void* pv,
        std::uint64_t size,
        std::uint64_t* out_bytes_written,
        ExternalStreamErrorInfoInterop* error_info);

    /// Function pointer used to close the stream. This function is called only once, and up until this function is called,
    /// the write_function pointer must remain valid and operational. No assumptions should be made about when this
    /// function is called, so the implementation must be prepared to handle this function being called at any time
    /// (but not concurrently with calls to the read_function).
    /// 
    /// \param  opaque_handle1  The value of the opaque_handle1 field of the ExternalOutputStreamStructInterop.
    /// \param  opaque_handle2  The value of the opaque_handle2 field of the ExternalOutputStreamStructInterop.
    void(*close_function)(std::uintptr_t opaque_handle1, std::uintptr_t opaque_handle2);
};

#pragma pack(pop)
