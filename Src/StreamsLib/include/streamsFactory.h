#pragma once

#include "include_libczi.h"
#include <cstdint>
#include <string>
#include <stdexcept>
#include <map>
#include <memory>

namespace libCZIStreamsLib
{
    struct Property
    {
    public:
        /// Values that represent the type represented by this variant.
        enum class Type
        {
            Invalid,    ///< An enum constant representing the 'invalid' type (would throw an invalid-argument-exception).
            Int32,      ///< An enum constant representing the 'signed 32-bit integer' type.
            Float,      ///< An enum constant representing the 'float' type.
            Double,     ///< An enum constant representing the 'double' type.
            Boolean,    ///< An enum constant representing the 'boolean' type.
            String      ///< An enum constant representing the 'string' type.
        };

        /// Default constructor - setting the variant to 'invalid'.
        Property() : type(Type::Invalid)
        {
        }

        /// Constructor for initializing the 'int32' type.
        ///
        /// \param  v   The value to set the variant to.
        explicit Property(std::int32_t v)
        {
            this->SetInt32(v);
        }

        /// Constructor for initializing the 'double' type.
        ///
        /// \param  v   The value to set the variant to.
        explicit Property(double v)
        {
            this->SetDouble(v);
        }

        /// Constructor for initializing the 'float' type.
        ///
        /// \param  v   The value to set the variant to.
        explicit Property(float v)
        {
            this->SetFloat(v);
        }

        /// Constructor for initializing the 'bool' type.
        ///
        /// \param  v   The value to set the variant to.
        explicit Property(bool v)
        {
            this->SetBool(v);
        }

        /// Constructor for initializing the 'string' type.
        ///
        /// \param  v   The value to set the variant to.
        explicit Property(const std::string& v)
        {
            this->SetString(v);
        }

        /// Constructor for initializing the 'string' type.
        ///
        /// \param  string   The value to set the variant to.
        explicit Property(const char* string)
        {
            this->SetString(string);
        }

        /// Sets the type of the variant to "Int32" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetInt32(std::int32_t v)
        {
            this->type = Type::Int32;
            this->int32Value = v;
        }

        /// Sets the type of the variant to "Double" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetDouble(double v)
        {
            this->type = Type::Double;
            this->doubleValue = v;
        }

        /// Sets the type of the variant to "Float" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetFloat(float v)
        {
            this->type = Type::Float;
            this->floatValue = v;
        }

        /// Sets the type of the variant to "Boolean" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetBool(bool v)
        {
            this->type = Type::Boolean;
            this->boolValue = v;
        }

        /// Sets the type of the variant to "String" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetString(const std::string& v)
        {
            this->type = Type::String;
            this->stringValue = v;
        }

        /// Returns integer value if ValueType is int, otherwise throws a RuntimeError.
        std::int32_t GetAsInt32OrThrow() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Int32);
            return this->int32Value;
        }

        /// Returns double value if ValueType is double, otherwise throws a RuntimeError.
        double GetAsDoubleOrThrow() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Double);
            return this->doubleValue;
        }

        /// Returns float value if ValueType is 'Float', otherwise throws a RuntimeError.
        float GetAsFloatOrThrow() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Float);
            return this->floatValue;
        }

        /// Returns boolean value if ValueType is boolean, otherwise throws a RuntimeError.
        bool GetAsBoolOrThrow() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Boolean);
            return this->boolValue;
        }

        /// Returns string value if ValueType is string, otherwise throws a RuntimeError.
        std::string GetAsStringOrThrow() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::String);
            return this->stringValue;
        }

        /// Returns ValueType.
        Type GetType() const
        {
            return this->type;
        }

    private:
        Type type;   ///< The type which is represented by the variant.

        union
        {
            std::int32_t int32Value;
            float floatValue;
            double doubleValue;
            bool boolValue;
        };

        std::string stringValue;

        void ThrowIfTypeIsUnequalTo(Type typeToCheck) const
        {
            if (this->type != typeToCheck)
            {
                throw std::runtime_error("Unexpected type encountered.");
            }
        }
    };

    class StreamProperties
    {
    public:
        const int kTest = 0;
    };

    struct CreateStreamInfo
    {
        std::string class_name;
        
        std::string filename;

        std::map<int, Property> property_bag;
    };


    std::shared_ptr<libCZI::IStream> CreateStream(const CreateStreamInfo& stream_info);

    struct StreamClassInfo
    {
        std::string class_name;
        std::string short_description;
    };

    bool GetStreamInfoForClass(int index, StreamClassInfo& stream_info);
    int GetStreamInfoCount();
}