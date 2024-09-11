// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE

#include <memory>
#include <string>

#include "../libCZI.h"
#include <azure/storage/blobs.hpp>

/// Implementation of stream object based on Azure-SDK C++ (https://github.com/Azure/azure-sdk-for-cpp).
/// This azure-input-stream is intended to manage authentication with the functionality provided by the
/// Azure-SDK. Please see https://learn.microsoft.com/en-us/azure/storage/blobs/quickstart-blobs-c-plus-plus?tabs=connection-string%2Croles-azure-portal#authenticate-to-azure-and-authorize-access-to-blob-data
/// for the concepts followed in this implementation.
/// The idea is:
/// * For this stream object, the uri-string provided by the user contains the necessary information to identify the blob  
///    to operate on in an Azure Blob Storage account.
/// * For this uri-string, we define a syntax (detailed below) that gives the information in a key-value form.  
/// * In addition, the property-bag is used to provide additional information controlling the operation.    
/// 
/// The syntax for the uri-string is: <key1>=<value1>;<key2>=<value2>.
/// The following rules apply:
/// * Key-value pairs are separated by a semicolon ';'.  
/// * A equal sign '=' separates the key from the value.  
/// * Spaces are significant, they are part of the key or value. So, the key "key" is different from the key " key".  
/// * A semicolon or an equal sign can be part of the key or value if it is escaped by a backslash '\'.  
/// * Empty keys or values are not allowed.  
///
/// There are multiple ways to authenticate with Azure Blob Storage. This stream object supports the following methods:
/// +-------------------------------+
/// | DefaultAzureCredential        | This method uses the Azure SDK's DefaultAzureCredential to authenticate with Azure Blob Storage.  
/// | EnvironmentCredential         | This method uses the Azure SDK's EnvironmentCredential to authenticate with Azure Blob Storage.
/// | AzureCliCredential            | 
/// | ManagedIdentityCredential     |   
class AzureBlobInputStream : public libCZI::IStream
{
private:
    static const wchar_t* kUriKey_ContainerName;
    static const wchar_t* kUriKey_BlobName;
    static const wchar_t* kUriKey_Account;
    static const wchar_t* kUriKey_AccountUrl;
    static const wchar_t* kUriKey_ConnectionString;

    std::unique_ptr<Azure::Storage::Blobs::BlockBlobClient> block_blob_client_;

    enum class AuthenticationMode
    {
        /// Use the Azure SDK's DefaultAzureCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_default_azure_credential.html.
        /// Note that the documentation states:
        /// This credential is intended to be used at the early stages of development, to allow the developer some time to work with the other aspects 
        /// of the SDK, and later to replace this credential with the exact credential that is the best fit for the application. It is not intended 
        /// to be used in a production environment.
        DefaultAzureCredential,

        /// Use the Azure SDK's EnvironmentCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_environment_credential.html.
        EnvironmentCredential,

        /// Use the Azure SDK's AzureCliCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_azure_cli_credential.html.
        AzureCliCredential,

        //ManagedIdentityCredential,
        //WorkloadIdentityCredential,
        
        ConnectionString,
    };
public:
    AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    AzureBlobInputStream(const std::wstring& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    ~AzureBlobInputStream() override;

    static std::string GetBuildInformation();
    static libCZI::StreamsFactory::Property GetClassProperty(const char* property_name);
private:
    static AuthenticationMode DetermineAuthenticationMode(const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    static std::string DetermineServiceUrl(const std::map<std::wstring, std::wstring>& tokenized_file_name);
    void CreateWithDefaultAzureCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithEnvironmentCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithCreateAzureCliCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag, const std::function<std::shared_ptr<Azure::Core::Credentials::TokenCredential>()>& create_credentials_functor);
    void CreateWithConnectionString(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateDefaultAzureCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateEnvironmentCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateAzureCliCredential();
    // https ://learn.microsoft.com/en-us/azure/storage/blobs/quickstart-blobs-c-plus-plus?tabs=managed-identity%2Croles-azure-portal#authenticate-to-azure-and-authorize-access-to-blob-data
};

#endif
