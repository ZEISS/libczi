// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "ImportExport.h"
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>

namespace libCZI
{
    class IStream;

    /// A factory object for creating streams objects. 
    /// libCZI is operating on abstractions (IStream for an input stream, IOutputStream for an output stream and IInputOutputStream
    /// for and input-output-stream) for accessing the CZI-data. In this class factory we gather implementations provided by libCZI
    /// and provide functionality to enumerate available classes.
    /// At this point, we can find two variants here - for operating on a file in a file-system and for operating on an http- or
    /// https-stream.
    /// The http-stream class is based on cURL (https://curl.se/libcurl/), and it needs to be configured in when building libCZI.
    class LIBCZI_API StreamsFactory
    {
    public:
        /// This declares a variant type (to be used with the property bag in the streams factory).
        struct LIBCZI_API Property
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

        /// Here the keys for the property-bag with options for creating a stream-object are gathered.
        class StreamProperties
        {
        public:
            /// Values that identify properties in the property-bag for constructing a stream-object.
            enum
            {
                kCurlHttp_Proxy = 100, ///< For CurlHttpInputStream, type string: gives the proxy to use, c.f. https://curl.se/libcurl/c/CURLOPT_PROXY.html for more information.

                kCurlHttp_UserAgent = 101, ///< For CurlHttpInputStream, type string: gives the user agent to use, c.f. https://curl.se/libcurl/c/CURLOPT_USERAGENT.html for more information.

                kCurlHttp_Timeout = 102, ///< For CurlHttpInputStream, type int32: gives the timeout in seconds, c.f. https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html for more information.

                kCurlHttp_ConnectTimeout = 103, ///< For CurlHttpInputStream, type int32: gives the timeout in seconds for the connection phase, c.f. https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html for more information.

                kCurlHttp_Xoauth2Bearer = 104, ///< For CurlHttpInputStream, type string: gives an OAuth2.0 access token, c.f. https://curl.se/libcurl/c/CURLOPT_XOAUTH2_BEARER.html for more information.

                kCurlHttp_Cookie = 105, ///< For CurlHttpInputStream, type string: gives a cookie, c.f. https://curl.se/libcurl/c/CURLOPT_COOKIE.html for more information.

                kCurlHttp_SslVerifyPeer = 106, ///< For CurlHttpInputStream, type bool: a boolean indicating whether the SSL-certificate of the remote host is to be verified, c.f. https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html for more information.

                kCurlHttp_SslVerifyHost = 107, ///< For CurlHttpInputStream, type bool: a boolean indicating whether the SSL-certificate's name is to be verified against host, c.f. https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html for more information.

                kCurlHttp_FollowLocation = 108, ///< For CurlHttpInputStream, type bool: a boolean indicating whether redirects are to be followed, c.f. https://curl.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html for more information.

                kCurlHttp_MaxRedirs = 109, ///< For CurlHttpInputStream, type int32: gives the maximum number of redirects to follow, c.f. https://curl.se/libcurl/c/CURLOPT_MAXREDIRS.html for more information.

                kCurlHttp_CaInfo = 110, ///< For CurlHttpInputStream, type string: gives the directory to check for CA certificate bundle , c.f. https://curl.se/libcurl/c/CURLOPT_CAINFO.html for more information.

                kCurlHttp_CaInfoBlob = 111, ///< For CurlHttpInputStream, type string: give PEM encoded content holding one or more certificates to verify the HTTPS server with, c.f. https://curl.se/libcurl/c/CURLOPT_CAINFO_BLOB.html for more information.
            };
        };

        /// The parameters for creating an instance of a stream object.
        struct LIBCZI_API CreateStreamInfo
        {
            std::string class_name; ///< Name of the class (this uniquely identifies the class).

            std::map<int, Property> property_bag; ///< A property-bag with options for creating the stream-object.
        };

        /// Perform one-time initialization of the streams objects.
        /// Some stream objects may require some one-time initialization for being operational (this is e.g. the case with
        /// the libcurl-based ones). It is considered good practice to call this function before using any of the other methods.
        /// Calling it multiple times is not a problem (and subsequent calls after the first are ignored).
        static void Initialize();

        /// Creates and initializes a new instance of the specified stream class. If the specified
        /// class is not known, then this function will return nullptr. In case of an error when
        /// initializing the stream, an exception will be thrown.
        ///
        /// \param  stream_info     Information describing the stream.
        /// \param  file_identifier The filename (or, more generally, an URI of some sort) identifying the file to be opened in UTF8-encoding.
        ///
        /// \returns    The newly created and initialized stream.
        static std::shared_ptr<libCZI::IStream> CreateStream(const CreateStreamInfo& stream_info, const std::string& file_identifier);

        /// Creates and initializes a new instance of the specified stream class. If the specified
        /// class is not known, then this function will return nullptr. In case of an error when
        /// initializing the stream, an exception will be thrown.
        ///
        /// \param  stream_info     Information describing the stream.
        /// \param  file_identifier The filename (or, more generally, an URI of some sort) identifying the file to be opened.
        ///
        /// \returns    The newly created and initialized stream.
        static std::shared_ptr<libCZI::IStream> CreateStream(const CreateStreamInfo& stream_info, const std::wstring& file_identifier);

        /// This structure gathers information about a stream class.
        struct LIBCZI_API StreamClassInfo
        {
            std::string class_name;                         ///< Name of the class (this uniquely identifies the class).
            std::string short_description;                  ///< A short and informal description of the class.

            /// A function which returns a string with build information for the class (e.g. version information). Note 
            /// that this field may be null, in which case no information is available.
            std::function<std::string()> get_build_info;

            /// A function which returns a class-specific property about the class. This is e.g. intended for
            /// providing information about build-time options for a specific class. Currently, it is used for
            /// the libcurl-based stream-class to provide information about the build-time configured paths for
            /// the CA certificates.
            /// Note that this field may be null, in which case no information is available.
            std::function<Property(const char* property_name)> get_property;
        };

        /// Gets information about a stream class available in the factory. The function returns false if the index is out of range.
        /// The function is idempotent, the information returned from it will not change during the lifetime of the application.
        ///
        /// \param          index       Zero-based index of the class for which information is to be retrieved.
        /// \param [out]    stream_info Information describing the stream class.
        ///
        /// \returns    True if it succeeds; false otherwise.
        static bool GetStreamInfoForClass(int index, StreamClassInfo& stream_info);

        /// Gets the number of stream classes available in the factory.
        ///
        /// \returns    The number of available stream classes.
        static int GetStreamClassesCount();

        /// Creates an instance of the default streams-objects for reading from the file-system.
        ///
        /// \param  filename Filename of the file to open (in UTF-8 encoding).
        ///
        /// \returns A new instance of a streams-objects for reading the specified file from the file-system.
        static std::shared_ptr<libCZI::IStream> CreateDefaultStreamForFile(const char* filename);

        /// Creates an instance of the default streams-objects for reading from the file-system.
        ///
        /// \param  filename Filename of the file to open.
        ///
        /// \returns A new instance of a streams-objects for reading the specified file from the file-system.
        static std::shared_ptr<libCZI::IStream> CreateDefaultStreamForFile(const wchar_t* filename);

        /// A static string for the property_name for the get_property-function of the StreamClassInfo identifying the
        /// build-time configured file holding one or more certificates to verify the peer with. C.f. https://curl.se/libcurl/c/curl_version_info.html, this
        /// property gives the value of the "cainfo"-field. If it is null, then an invalid property is returned. 
        static const char* kStreamClassInfoProperty_CurlHttp_CaInfo;

        /// A static string for the property_name for the get_property-function of the StreamClassInfo identifying the
        /// build-time configured directory holding CA certificates. C.f. https://curl.se/libcurl/c/curl_version_info.html, this
        /// property gives the value of the "capath"-field. If it is null, then an invalid property is returned.
        static const char* kStreamClassInfoProperty_CurlHttp_CaPath;
    };
}
