#include "azureblobinputstream.h"

#include <azure/storage/blobs.hpp>
#include <azure/identity/default_azure_credential.hpp>
#include "../utilities.h"

using namespace std;

/*static*/const wchar_t* AzureBlobInputStream::kUriKey_ContainerName = L"containername";
/*static*/const wchar_t* AzureBlobInputStream::kUriKey_BlobName = L"blobname";
/*static*/const wchar_t* AzureBlobInputStream::kUriKey_Account = L"account";
/*static*/const wchar_t* AzureBlobInputStream::kUriKey_AccountUrl = L"accounturl";
/*static*/const wchar_t* AzureBlobInputStream::kUriKey_ConnectionString = L"connectionstring";

AzureBlobInputStream::AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
    : AzureBlobInputStream(Utilities::convertUtf8ToWchar_t(url.c_str()), property_bag)
{
}

AzureBlobInputStream::AzureBlobInputStream(const std::wstring& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    const auto key_value_uri = Utilities::TokenizeAzureUriString(url);

    const auto authentication_mode = AzureBlobInputStream::DetermineAuthenticationMode(property_bag);

    switch (authentication_mode)
    {
    case AuthenticationMode::DefaultAzureCredential:
        this->CreateWithDefaultAzureCredential(key_value_uri, property_bag);
        break;
    case AuthenticationMode::ConnectionString:
        this->CreateWithConnectionString(key_value_uri, property_bag);
        break;
    default:
        throw std::runtime_error("Unsupported authentication mode");
    }
}

void AzureBlobInputStream::CreateWithDefaultAzureCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    // check whether the required arguments are present in the tokenized_file_name-property-bag
    //
    // 1. containername and blobname are required in any case
    // 2. then, either account or accounturl must be present. If accounturl and account are present, account is ignored.
    //
    // Test-URI: account=libczirwtestdata;containername=$web;blobname=libczi/DCV_30MB.czi
    auto iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_ContainerName);
    if (iterator == tokenized_file_name.end())
    {
        ostringstream string_stream;
        string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_ContainerName) << "'.";
        throw std::runtime_error(string_stream.str());
    }

    const string container_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_BlobName);
    if (iterator == tokenized_file_name.end())
    {
        ostringstream string_stream;
        string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_BlobName) << "'.";
        throw std::runtime_error(string_stream.str());
    }

    const string blob_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    string service_url = AzureBlobInputStream::DetermineServiceUrl(tokenized_file_name);

    // Initialize an instance of DefaultAzureCredential
    auto defaultAzureCredential = make_shared<Azure::Identity::DefaultAzureCredential>();

    Azure::Storage::Blobs::BlobServiceClient blob_service_client(service_url, defaultAzureCredential);

    // Get a reference to the container and blob
    const auto blob_container_client = blob_service_client.GetBlobContainerClient(container_name);
    auto blobClient = blob_container_client.GetBlockBlobClient(blob_name);

    // note: make_unique is not available in C++11
    this->block_blob_client_ = unique_ptr<Azure::Storage::Blobs::BlockBlobClient>(new Azure::Storage::Blobs::BlockBlobClient(blobClient));
}

void AzureBlobInputStream::CreateWithConnectionString(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    auto iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_ConnectionString);
    if (iterator == tokenized_file_name.end())
    {
        ostringstream string_stream;
        string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_ConnectionString) << "'.";
        throw std::runtime_error(string_stream.str());
    }

    const string connection_string = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_ContainerName);
    if (iterator == tokenized_file_name.end())
    {
        ostringstream string_stream;
        string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_ContainerName) << "'.";
        throw std::runtime_error(string_stream.str());
    }

    const string container_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_BlobName);
    if (iterator == tokenized_file_name.end())
    {
        ostringstream string_stream;
        string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_BlobName) << "'.";
        throw std::runtime_error(string_stream.str());
    }

    const string blob_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    const auto blob_service_client = Azure::Storage::Blobs::BlobServiceClient::CreateFromConnectionString(connection_string);

    // Get a reference to the container and blob
    const auto blob_container_client = blob_service_client.GetBlobContainerClient(container_name);
    auto blobClient = blob_container_client.GetBlockBlobClient(blob_name);

    // note: make_unique is not available in C++11
    this->block_blob_client_ = unique_ptr<Azure::Storage::Blobs::BlockBlobClient>(new Azure::Storage::Blobs::BlockBlobClient(blobClient));
}

void AzureBlobInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    // On Azure-SDK level, the size and offset are signed 64-bit integers - we check whether the requested size is within the limits of the signed 64-bit integer
    //  and otherwise throw an exception. The casts below are therefore safe.
    if (size > numeric_limits<int64_t>::max() || size > numeric_limits<size_t>::max())
    {
        throw std::runtime_error("size is too large");
    }

    if (offset > numeric_limits<int64_t>::max())
    {
        throw std::runtime_error("offset is too large");
    }

    Azure::Storage::Blobs::DownloadBlobToOptions options;
    options.Range = Azure::Core::Http::HttpRange{ static_cast<int64_t>(offset), static_cast<int64_t>(size) };
    auto download_response = this->block_blob_client_->DownloadTo(static_cast<uint8_t*>(pv), static_cast<size_t>(size), options);
    const Azure::Core::Http::HttpStatusCode code = download_response.RawResponse->GetStatusCode();
    if (code == Azure::Core::Http::HttpStatusCode::Ok || code == Azure::Core::Http::HttpStatusCode::PartialContent)
    {
        // the reported position should match the requested offset
        if (download_response.Value.ContentRange.Offset != static_cast<int64_t>(offset))
        {
            throw std::runtime_error("The reported position does not match the requested offset");
        }

        // and, we expect that the Length is valid (and less than the requested size)
        if (!download_response.Value.ContentRange.Length.HasValue())
        {
            throw std::runtime_error("No Length given in the downloadResponse.");
        }

        if (download_response.Value.ContentRange.Length.Value() > static_cast<int64_t>(size))
        {
            throw std::runtime_error("The reported length is larger than the requested size");
        }

        if (ptrBytesRead != nullptr)
        {
            *ptrBytesRead = download_response.Value.ContentRange.Length.Value();
        }
    }
}

AzureBlobInputStream::~AzureBlobInputStream()
{
}

/*static*/std::string AzureBlobInputStream::GetBuildInformation()
{
    return { LIBCZI_AZURESDK_VERSION_INFO };
}

/*static*/libCZI::StreamsFactory::Property AzureBlobInputStream::GetClassProperty(const char* property_name)
{
    return libCZI::StreamsFactory::Property();
}


/*static*/AzureBlobInputStream::AuthenticationMode AzureBlobInputStream::DetermineAuthenticationMode(const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    const auto iterator = property_bag.find(libCZI::StreamsFactory::StreamProperties::kAzureBlob_AuthenticationMode);
    if (iterator != property_bag.end())
    {
        const auto& value = iterator->second.GetAsStringOrThrow();
        if (value == "DefaultAzureCredential")
        {
            return AuthenticationMode::DefaultAzureCredential;
        }

        if (value == "ConnectionString")
        {
            return AuthenticationMode::ConnectionString;
        }

        throw std::runtime_error("Unsupported authentication mode");
    }

    return AuthenticationMode::DefaultAzureCredential;
}

/*static*/std::string AzureBlobInputStream::DetermineServiceUrl(const std::map<std::wstring, std::wstring>& tokenized_file_name)
{
    auto iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_AccountUrl);
    if (iterator != tokenized_file_name.end())
    {
        return Utilities::convertWchar_tToUtf8(iterator->second.c_str());
    }

    iterator = tokenized_file_name.find(AzureBlobInputStream::kUriKey_Account);
    if (iterator != tokenized_file_name.end())
    {
        ostringstream account_url;
        account_url << "https://" << Utilities::convertWchar_tToUtf8(iterator->second.c_str()) << ".blob.core.windows.net";
        return account_url.str();
    }

    ostringstream string_stream;
    string_stream << "The specified uri-string must specify a value for '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_Account) << "' or '" << Utilities::convertWchar_tToUtf8(AzureBlobInputStream::kUriKey_AccountUrl) << "'.";
    throw std::runtime_error(string_stream.str());
}
