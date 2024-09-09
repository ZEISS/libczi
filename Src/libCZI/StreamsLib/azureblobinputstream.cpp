#include "azureblobinputstream.h"

#include <azure/storage/blobs.hpp>
#include <azure/identity/default_azure_credential.hpp>
#include <iostream>
#include "../utilities.h"

using namespace std;

AzureBlobInputStream::AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
    : AzureBlobInputStream(Utilities::convertUtf8ToWchar_t(url.c_str()), property_bag)
{
}

AzureBlobInputStream::AzureBlobInputStream(const std::wstring& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    const auto key_value_uri = Utilities::TokenizeAzureUriString(url);

    const auto authentication_mode = DetermineAuthenticationMode(property_bag);

    switch (authentication_mode)
    {
    case AuthenticationMode::DefaultAzureCredential:
        this->CreateWithDefaultAzureCredential(key_value_uri, property_bag);
        break;
    default:
        throw std::runtime_error("Unsupported authentication mode");
    }
    /*const std::string connectionString = "XXX";
    const std::string containerName = "$web";
    const std::string blobName = "libczi/DCV_30MB.czi";

    auto containerClient = Azure::Storage::Blobs::BlobContainerClient::CreateFromConnectionString(connectionString, containerName);

    for (auto blobPage = containerClient.ListBlobs(); blobPage.HasPage(); blobPage.MoveToNextPage()) {
        for (auto& blob : blobPage.Blobs) {
            // Below is what you want to do with each blob
            std::cout << "blob: " << blob.Name << std::endl;
        }
    }*/


#if 0
    // Initialize an instance of DefaultAzureCredential
    auto defaultAzureCredential = std::make_shared<Azure::Identity::DefaultAzureCredential>();
    cout << "DefaultAzureCredential" << defaultAzureCredential->GetCredentialName() << endl;

    //auto accountURL = "https://<storage-account-name>.blob.core.windows.net";
    auto accountURL = "https://libczirwtestdata.blob.core.windows.net/";
    Azure::Storage::Blobs::BlobServiceClient blobServiceClient(accountURL, defaultAzureCredential);

    cout << "URL:" << blobServiceClient.GetUrl() << endl;


    // auto x = blobServiceClient.ListBlobContainers();
     //cout << "List of blob containers in the account:" << x.Prefix << endl;

     // Specify the container and blob you want to access
    std::string container_name = "$web";
    std::string blob_name = "libczi/DCV_30MB.czi";

    // Get a reference to the container and blob
    auto containerClient = blobServiceClient.GetBlobContainerClient(container_name);

    /*
    auto list = containerClient.ListBlobs();
    // Loop through the blobs and print information
    for (const auto& blobItem : list.Blobs) {
        std::cout << "Blob Name: " << blobItem.Name << std::endl;
        std::cout << "Blob Size: " << blobItem.BlobSize << " bytes" << std::endl;


        std::cout << "---------------------" << std::endl;
    }*/

    auto blobClient = containerClient.GetBlockBlobClient(blob_name);

    this->blockBlobClient_ = std::make_unique<Azure::Storage::Blobs::BlockBlobClient>(blobClient);

    //try
    //{
    //    std::cout << "*#******************************" << std::endl;

    //    // Define the range you want to download (for example, bytes 0 to 99)
    //    Azure::Storage::Blobs::DownloadBlobToOptions options;
    //    options.Range = Azure::Core::Http::HttpRange{ 0,100 };

    //    // Prepare a buffer to hold the downloaded data
    //    std::vector<uint8_t> buffer(100);
    //    buffer.resize(100);

    //    // Download the specified range into the buffer
    //    auto downloadResponse = blobClient.DownloadTo(buffer.data(), buffer.size(), options);

    //    std::cout << "DONE" << std::endl;

    //    // Output the downloaded range content
    //    std::cout << "Downloaded range (0-99): ";
    //    for (auto byte : buffer) {
    //        std::cout << static_cast<char>(byte);  // Assuming the blob content is text or convertible to char
    //    }
    //    std::cout << std::endl;
    //}
    //catch (const Azure::Core::RequestFailedException& e) {
    //    // Handle any errors that occur during the download
    //    std::cerr << "Failed to download range: " << e.Message << std::endl;
    //}
    //catch (const std::exception& e) {
    //    // Handle any errors that occur during the download
    //    std::cerr << "exception caught: " << e.what()<< std::endl;
    //}
#endif
}

void AzureBlobInputStream::CreateWithDefaultAzureCredential(const std::map<std::wstring, std::wstring>& tokenized_file_name, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
    // check whether the required arguments are present in the tokenized_file_name-property-bag
    //
    // 1. containername and blobname are required in any case
    // 2. then, either account or accounturl must be present. If accounturl and account are present, account is ignored.
    //
    // Test-URI: account=libczirwtestdata;containername=$web;blobname=libczi/DCV_30MB.czi
    auto iterator = tokenized_file_name.find(L"containername");
    if (iterator == tokenized_file_name.end())
    {
        throw std::runtime_error("The specified uri-string must specify a value for 'containername'.");
    }

    const string container_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    iterator = tokenized_file_name.find(L"blobname");
    if (iterator == tokenized_file_name.end())
    {
        throw std::runtime_error("The specified uri-string must specify a value for 'blobname'.");
    }

    const string blob_name = Utilities::convertWchar_tToUtf8(iterator->second.c_str());

    string service_url = AzureBlobInputStream::DetermineServiceUrl(tokenized_file_name);

    // Initialize an instance of DefaultAzureCredential
    auto defaultAzureCredential = std::make_shared<Azure::Identity::DefaultAzureCredential>();
    //cout << "DefaultAzureCredential" << defaultAzureCredential->GetCredentialName() << endl;

    // note: make_unique is not available in C++11
    this->serviceClient_ = unique_ptr<Azure::Storage::Blobs::BlobServiceClient>(new Azure::Storage::Blobs::BlobServiceClient(service_url, defaultAzureCredential));
    //cout << "URL:" << this->serviceClient_->GetUrl() << endl;

    // Get a reference to the container and blob
    auto containerClient = this->serviceClient_->GetBlobContainerClient(container_name);
    auto blobClient = containerClient.GetBlockBlobClient(blob_name);
    this->blockBlobClient_ = unique_ptr<Azure::Storage::Blobs::BlockBlobClient>(new Azure::Storage::Blobs::BlockBlobClient(blobClient));
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
    auto download_response = this->blockBlobClient_->DownloadTo(static_cast<uint8_t*>(pv), static_cast<size_t>(size), options);
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
    return AuthenticationMode::DefaultAzureCredential;
}

/*static*/std::string AzureBlobInputStream::DetermineServiceUrl(const std::map<std::wstring, std::wstring>& tokenized_file_name)
{
    if (tokenized_file_name.find(L"accounturl") != tokenized_file_name.end())
    {
        return Utilities::convertWchar_tToUtf8(tokenized_file_name.at(L"accounturl").c_str());
    }

    if (tokenized_file_name.find(L"account") != tokenized_file_name.end())
    {
        ostringstream account_url;
        account_url << "https://" << Utilities::convertWchar_tToUtf8(tokenized_file_name.at(L"account").c_str()) << ".blob.core.windows.net";
        return account_url.str();
    }

    throw std::runtime_error("The specified uri-string must specify a value for 'account' or 'accounturl'.");
}
