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
/// * DefaultAzureCredential
/// * EnvironmentCredential
/// * AzureCliCredential
/// * ManagedIdentityCredential  
/// * WorkloadIdentityCredential  
/// * ConnectionString  
/// 
/// For the uri-string, the following keys are defined:
/// +------------------+---------------------------------------------------
/// | account          | The storage-account name. It will be used to create the account-URL as
/// |                  | https://<acount>.blob.core.windows.net".
/// |                  | This key is relevant for all authentication modes except
/// |                  | "ConnectionString".
/// +------------------+--------------------------------------------------------------------------
/// | accounturl       | The complete base-URL for the storage account. If this is given, then the
/// |                  | key 'account' is ignored (and this URL is used instead).
/// |                  | This key is relevant for all authentication modes except 
/// |                  | "ConnectionString".
/// +------------------+--------------------------------------------------------------------------
/// | containername    | The container name.
/// +------------------+--------------------------------------------------------------------------
/// | blobname         | The name of the blob.
/// +------------------+--------------------------------------------------------------------------
/// | connectionstring | The connection string to access the blob store. 
/// |                  | This key is relevant only for authentication mode "ConnectionString".
/// +------------------+--------------------------------------------------------------------------
/// 
/// In the property-bag, the following keys are used:
/// 
/// | Property                      | ID  | Type   | Description
/// +-------------------------------+-----+--------+---------------------------------------------------
/// | kAzureBlob_AuthenticationMode | 200 | string | Choose the authentication mode. Possible values are:
/// |                               |     |        | DefaultAzureCredential, EnvironmentCredential, AzureCliCredential,
/// |                               |     |        | ManagedIdentityCredential, WorkloadIdentityCredential,
/// |                               |     |        | ConnectionString.
/// |                               |     |        | The default is : DefaultAzureCredential.
class AzureBlobInputStream : public libCZI::IStream
{
private:
    static const wchar_t* kUriKey_ContainerName;
    static const wchar_t* kUriKey_BlobName;
    static const wchar_t* kUriKey_Account;
    static const wchar_t* kUriKey_AccountUrl;
    static const wchar_t* kUriKey_ConnectionString;

    std::unique_ptr<Azure::Storage::Blobs::BlockBlobClient> block_blob_client_;

    /// Values that represent authentication modes.
    enum class AuthenticationMode
    {
        /// Use the Azure SDK's DefaultAzureCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_default_azure_credential.html.
        /// Note that the documentation states:
        /// This credential is intended to be used at the early stages of development, to allow the developer some time to work with the other aspects 
        /// of the SDK, and later to replace this credential with the exact credential that is the best fit for the application. It is not intended 
        /// to be used in a production environment.
        /// This will try the following authentication methods (in this order) and stop when one succeeds:
        /// EnvironmentCredential, WorkloadIdentityCredential, AzureCliCredential, ManagedIdentityCredential.
        DefaultAzureCredential,

        /// Use the Azure SDK's EnvironmentCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_environment_credential.html.
        /// This will read account information specified via environment variables and use it to authenticate - c.f. https://github.com/Azure/azure-sdk-for-cpp/blob/main/sdk/identity/azure-identity/README.md#environment-variables.
        EnvironmentCredential,

        /// Use the Azure SDK's AzureCliCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_azure_cli_credential.html.
        AzureCliCredential,

        /// Use the Azure SDK's ManagedIdentityCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_managed_identity_credential.html.
        /// Attempts authentication using a managed identity that has been assigned to the deployment environment. This authentication type works in Azure VMs, App Service and Azure Functions applications, as well as the Azure Cloud Shell. More information about configuring 
        /// managed identities can be found here: https://learn.microsoft.com/entra/identity/managed-identities-azure-resources/overview.
        ManagedIdentityCredential,

        /// Use the Azure SDK's WorkloadIdentityCredential to authenticate with Azure Blob Storage. C.f. https://azuresdkdocs.blob.core.windows.net/$web/cpp/azure-identity/1.9.0/class_azure_1_1_identity_1_1_workload_identity_credential.html.
        /// This authenticates using a Kubernetes service account token.
        WorkloadIdentityCredential,

        /// Use authentication via a connection string. A connection string includes the storage account access key and uses it to authorize requests. Always be careful to never expose the 
        /// keys in an unsecure location.
        ConnectionString,
    };
public:
    AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    AzureBlobInputStream(const std::wstring& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    static std::string GetBuildInformation();
private:
    static AuthenticationMode DetermineAuthenticationMode(const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    static std::string DetermineServiceUrl(const std::map<std::wstring, std::wstring>& tokenized_file_name);
    void CreateWithDefaultAzureCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithEnvironmentCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithCreateAzureCliCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithWorkloadIdentityCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithManagedIdentityCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);
    void CreateWithCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag, const std::function<std::shared_ptr<Azure::Core::Credentials::TokenCredential>()>& create_credentials_functor);
    void CreateWithConnectionString(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateDefaultAzureCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateEnvironmentCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateAzureCliCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateWorkloadIdentityCredential();
    static std::shared_ptr<Azure::Core::Credentials::TokenCredential> CreateManagedIdentityCredential();
};

#endif
