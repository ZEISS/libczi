// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "ImportExport.h"
#include "libCZI_exceptions.h"
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace libCZI
{
    /// Values that represent dimension indexes.
    enum class DimensionIndex : std::uint8_t
    {
        invalid = 0,    ///< Invalid dimension index.

        MinDim = 1,     ///< This enum must be have the value of the lowest (valid) dimension index.

        Z = 1,          ///< The Z-dimension.
        C = 2,          ///< The C-dimension ("channel").
        T = 3,          ///< The T-dimension ("time").
        R = 4,          ///< The R-dimension ("rotation").
        S = 5,          ///< The S-dimension ("scene").
        I = 6,          ///< The I-dimension ("illumination").
        H = 7,          ///< The H-dimension ("phase").
        V = 8,          ///< The V-dimension ("view").
        B = 9,          ///< The B-dimension ("block") - its use is deprecated.

        MaxDim = 9      ///< This enum must be have the value of the highest (valid) dimension index.
    };

    /// Interface used to represent a coordinate (in the space of the dimensions identified by `DimensionIndex`).
    class LIBCZI_API IDimCoordinate
    {
    public:

        /// Attempts to get position index in the specified dimension.
        ///
        /// \param dim                 The dimension.
        /// \param [out] coordinate    If non-null and the dimension is valid (in this coordinate), it will receive the value of the coordinate for the specified dimension.
        ///
        /// \return True if it succeeds (i. e. the specified dimension is given in this coordinate), false otherwise.
        virtual bool TryGetPosition(DimensionIndex dim, int* coordinate) const = 0;

        virtual ~IDimCoordinate() = default;

        /// Query if the specified dimension is given ('is valid') in this coordinate.
        ///
        /// \param dim The dimension.
        ///
        /// \return True if the dimension is valid for this coordinate, false if not.
        inline bool IsValid(DimensionIndex dim) const
        {
            return this->TryGetPosition(dim, nullptr);
        }

        /// Gets the number of valid dimensions.
        ///
        /// \return The number of valid dimensions.
        inline int GetNumberOfValidDimensions() const
        {
            int cnt = 0;
            for (auto i = static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MinDim); i <= static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MaxDim); ++i)
            {
                if (this->IsValid(static_cast<libCZI::DimensionIndex>(i)))
                {
                    ++cnt;
                }
            }

            return cnt;
        }
    };

    /// Interface used to represent an interval (for several dimensions).
    class LIBCZI_API IDimBounds
    {
    public:

        /// Attempts to get the interval for the specified dimension.
        /// \param dim                 The dimension.
        /// \param [in,out] startIndex If non-null, it will receive the start index.
        /// \param [in,out] size       If non-null, it will receive the size.
        /// \return True if the dimension is valid and the data was succeessfully retrieved, false if it fails.
        virtual bool TryGetInterval(DimensionIndex dim, int* startIndex, int* size) const = 0;

        virtual ~IDimBounds() = default;

        /// Query if the specified dimension is valid.
        /// \param dim The dimension.
        /// \return True if valid, false otherwise.
        inline bool IsValid(DimensionIndex dim) const
        {
            return this->TryGetInterval(dim, nullptr, nullptr);
        }
    };

    /// Interface used to represent a set of indices. The object can be queried whether the specified index
    /// is contained in the set or not.
    class LIBCZI_API IIndexSet
    {
    public:

        /// Query if the specified 'index' is contained in the set or not.
        ///
        /// \param index The index to test.
        ///
        /// \return true if contained it is contained in the set, false if not.
        virtual bool IsContained(int index) const = 0;

        virtual ~IIndexSet() = default;
    };

    /// A structure combining a dimension and a value.
    struct DimensionAndValue
    {
        libCZI::DimensionIndex dimension;   ///< The dimension.
        int value;                          ///< The value (for this dimension).
    };

    /// A structure combining a dimension and an interval (defined by a start value and the size).
    struct DimensionAndStartSize
    {
        libCZI::DimensionIndex dimension;   ///< The dimension.
        int start;                          ///< The start value.
        int size;                           ///< The size.
    };

    /// Base class containing some commonly used methods.
    class LIBCZI_API CDimBase
    {
    protected:
        /// Utility to get a bit index for the specified dimension.
        /// \param dim The dimension.
        /// \return The bit index for dimension.
        static std::underlying_type<libCZI::DimensionIndex>::type GetBitIndexForDimension(libCZI::DimensionIndex dim)
        {
            // TODO: add checks and asserts
            auto i = ((std::underlying_type<libCZI::DimensionIndex>::type)dim) - 1;
            return i;
        }
    };

    /// Implementation of a class representing a coordinate (and implementing the `IDimCoordinate`-interface).
    class LIBCZI_API CDimCoordinate : public CDimBase, public libCZI::IDimCoordinate
    {
    private:
        std::uint32_t validDims;
        int values[static_cast<int>(libCZI::DimensionIndex::MaxDim)];
    public:
        /// Default constructor which constructs an empty coordinate (no valid dimensions).
        CDimCoordinate() : validDims(0) {}

        /// Constructor which constructs a coordinate object from the specified initializer list.
        /// It can be used like this:
        /// \code{.cpp}
        /// auto coord = CDimCoordinate{ { DimensionIndex::Z,8 } , { DimensionIndex::T,0 }, { DimensionIndex::C,1 } };
        /// \endcode
        /// \remark
        /// If the same dimension appears multiple times in the initializer list, then the last occurence wins.
        /// \param list The list of dimensions-and-values.
        CDimCoordinate(std::initializer_list<DimensionAndValue> list) : CDimCoordinate()
        {
            for (const auto& d : list)
            {
                this->Set(d.dimension, d.value);
            }
        }

        /// Copy-constructor which creates a copy of the specifed coordinate.
        ///
        /// \param other The coordinate for which to create a copy. It may be null in which case an empty coordinate is created.
        explicit CDimCoordinate(const libCZI::IDimCoordinate* other) : CDimCoordinate()
        {
            if (other != nullptr)
            {
                for (auto i = (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MinDim); i <= (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MaxDim); ++i)
                {
                    int coord;
                    if (other->TryGetPosition((libCZI::DimensionIndex)i, &coord))
                    {
                        this->Set((libCZI::DimensionIndex)i, coord);
                    }
                }
            }
        }

        /// Sets the value for the specified dimension. The specified dimension will be marked 'valid'.
        ///
        /// \param dimension The dimension to set.
        /// \param value     The value to set.
        void Set(libCZI::DimensionIndex dimension, int value)
        {
            int index = CDimCoordinate::GetBitIndexForDimension(dimension);
            this->values[index] = value;
            this->validDims |= (1 << index);
        }

        /// Clears the validity of the specified dimension.
        ///
        /// \param dimension The dimension to mark as 'not valid for this coordinate'.
        void Clear(libCZI::DimensionIndex dimension)
        {
            int index = CDimCoordinate::GetBitIndexForDimension(dimension);
            this->validDims &= ~(1 << index);
        }

        /// Clears the validity of all dimensions.
        void Clear()
        {
            this->validDims = 0;
        }

        /// Enumerate the valid dimensions contained in this coordinate. The specified functor will
        /// be called for each valid dimension (and providing the functor with the dimension index and
        /// the coordinate). If the functor returns false, the enumeration is cancelled.
        ///
        /// \param func The functor to call for each valid dimension. If returning false, the enumeration is cancelled.
        void EnumValidDimensions(const std::function<bool(libCZI::DimensionIndex dim, int value)>& func) const
        {
            for (auto i = (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MinDim); i <= (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MaxDim); ++i)
            {
                int bitIndex = CDimCoordinate::GetBitIndexForDimension((libCZI::DimensionIndex)i);
                if ((this->validDims & (1 << bitIndex)) != 0)
                {
                    if (func((libCZI::DimensionIndex)i, this->values[bitIndex]) != true)
                        break;
                }
            }
        }

        /// Determine the number the valid dimensions contained in this coordinate. 
        int GetValidDimensionsCount() const
        {
            int count = 0;
            for (auto i = (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MinDim); i <= (std::underlying_type<libCZI::DimensionIndex>::type)(libCZI::DimensionIndex::MaxDim); ++i)
            {
                int bitIndex = CDimCoordinate::GetBitIndexForDimension((libCZI::DimensionIndex)i);
                if ((this->validDims & (1 << bitIndex)) != 0)
                {
                    ++count;
                }
            }

            return count;
        }

        /// Parse the specified string and construct a CDimCoordinate-object from the information. The
        /// syntax for the string is - a character identifying the dimension (one of 'Z', 'C', 'T', 'R',
        /// 'S', 'I', 'H', 'V', 'B') followed by an integer (possibly with a + or -). There can be more
        /// than one dimension given, in which case the parts can be separated by a comma, a semicolon or
        /// spaces. The regular expression is: 
        /// \code{.unparsed}
        /// ([:blank:]*[Z|C|T|R|S|I|H|V|B][\\+|-]?[[:digit:]]+[,|;| ]*)
        /// \endcode
        /// It is illegal to give the same dimension multiple times.
        ///
        /// \exception LibCZIStringParseException Thrown when the specified string cannot be parsed because
        /// of a syntactical error.
        ///
        /// \param str The string to parse.
        ///
        /// \return A CDimCoordinate object constructed from the string.
        static CDimCoordinate Parse(const char* str);
    public: // IDimCoordinate
        bool TryGetPosition(libCZI::DimensionIndex dim, int* coordinate) const override
        {
            const int index = CDimCoordinate::GetBitIndexForDimension(dim);
            if ((this->validDims & (1 << index)) != 0)
            {
                if (coordinate != nullptr)
                {
                    *coordinate = this->values[index];
                }

                return true;
            }

            return false;
        }
    };

    /// Implementation of a class representing an interval (and implementing the `libCZI::IDimBounds`-interface).
    class LIBCZI_API CDimBounds : public CDimBase, public libCZI::IDimBounds
    {
    private:
        std::uint32_t validDims;
        int start[static_cast<int>(libCZI::DimensionIndex::MaxDim)];
        int size[static_cast<int>(libCZI::DimensionIndex::MaxDim)];
    public:
        /// Default constructor - the object will contain no valid dimension.
        CDimBounds() : validDims(0) {}

        /// Constructor which copies the content of the specified IDimBounds-object.
        ///
        /// \param other The IDimBounds-object to copy information from.
        explicit CDimBounds(const IDimBounds* other) : validDims(0)
        {
            if (other != nullptr)
            {
                for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
                {
                    int start, size;
                    if (other->TryGetInterval(static_cast<DimensionIndex>(i), &start, &size))
                    {
                        this->Set(static_cast<DimensionIndex>(i), start, size);
                    }
                }
            }
        }

        /// Construct a libCZI::CDimBounds object from an initializer-list.
        /// \param list The list of "dimension, start and size".
        CDimBounds(std::initializer_list<DimensionAndStartSize> list) : CDimBounds()
        {
            for (auto d : list)
            {
                this->Set(d.dimension, d.start, d.size);
            }
        }

        /// Sets (for the specified dimension) the start and the size.
        /// \param dimension The dimension.
        /// \param start     The start.
        /// \param size      The size.
        void Set(libCZI::DimensionIndex dimension, int start, int size)
        {
            const int index = CDimBounds::GetBitIndexForDimension(dimension);
            this->start[index] = start;
            this->size[index] = size;
            this->validDims |= (1 << index);
        }

        /// Enumerate valid dimensions.
        /// \param func The functor which will be called for all valid dimensions.
        void EnumValidDimensions(const std::function<bool(libCZI::DimensionIndex dim, int start, int size)>& func) const
        {
            for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
            {
                const int bitIndex = CDimBase::GetBitIndexForDimension(static_cast<libCZI::DimensionIndex>(i));
                if ((this->validDims & (1 << bitIndex)) != 0)
                {
                    if (func(static_cast<libCZI::DimensionIndex>(i), this->start[bitIndex], this->size[bitIndex]) != true)
                    {
                        break;
                    }
                }
            }
        }

        /// Clears the validity of the specified dimension.
        /// \param dimension The dimension.
        void Clear(libCZI::DimensionIndex dimension)
        {
            int index = CDimBounds::GetBitIndexForDimension(dimension);
            this->validDims &= ~(1 << index);
        }

        /// Clears this object to its blank/initial state. All dimensions will be set to invalid.
        void Clear()
        {
            this->validDims = 0;
        }

        /// Query if this object is empty - no valid dimensions contained.
        ///
        /// \return True if empty, false if not.
        bool IsEmpty() const
        {
            return this->validDims == 0;
        }

        /// Parse the specified string and construct a CDimBounds-object from the information. The syntax
        /// for the string is - a character identifying the dimension (one of 'Z', 'C', 'T', 'R', 'S',
        /// 'I', 'H', 'V', 'B') followed by an integer (possibly with a + or -) specifying the start-index,
        /// followed by a colon ':', then followed by an integer specifying the size. There can be more
        /// than one dimension given, in which case the above sequence is repeated for a different
        /// dimension. It is illegal to give the same dimension multiple times. Examples: "T0:10",
        /// "T0:5Z0:10", "C0:2T0:10Z0:5".
        ///
        /// \exception LibCZIStringParseException Thrown when the specified string cannot be parsed because
        /// of a syntactical error.
        ///
        /// \param str The string to parse.
        ///
        /// \return A CDimBounds object constructed from the string.
        static CDimBounds Parse(const char* str);

    public: // IDimBounds
        /// Attempts to get interval and size for the specified dimension.
        /// \param dim              The dimemension.
        /// \param [out] startIndex If non-null, it will receive the start index.
        /// \param [out] size       If non-null, it will receive the size.
        /// \return True if it succeeds, false if it fails.
        bool TryGetInterval(libCZI::DimensionIndex dim, int* startIndex, int* size) const override
        {
            const int index = CDimBounds::GetBitIndexForDimension(dim);
            if ((this->validDims & (1 << index)) != 0)
            {
                if (startIndex != nullptr)
                {
                    *startIndex = this->start[index];
                }

                if (size != nullptr)
                {
                    *size = this->size[index];
                }

                return true;
            }

            return false;
        }
    };
}
