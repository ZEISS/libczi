// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

/// Defines an alias representing the error codes for the libCZIApi-module.
/// In general, values greater than zero indicate an error condition; and values less
/// or equal to zero indicate proper operation.
typedef std::int32_t LibCZIApiErrorCode;

/// The operation completed successfully.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_OK = 0;

/// An invalid argument was supplied to the function.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_InvalidArgument = 1;

/// An invalid handle was supplied to the function (i.e. a handle which is either a bogus value or a handle which has already been destroyed).
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_InvalidHandle = 2;

/// The operation failed due to an out-of-memory condition.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_OutOfMemory = 3;

/// A supplied index was out of range.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_IndexOutOfRange = 4;

/// A semantic error in using Lock/Unlock methods (e.g. of the bitmap object) was detected. Reasons could be
/// an unbalanced number of Lock/Unlock calls, or the object was destroyed with a lock still held.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_LockUnlockSemanticViolated = 20;

/// An unspecified error occurred.
static constexpr LibCZIApiErrorCode LibCZIApi_ErrorCode_UnspecifiedError = 50;
