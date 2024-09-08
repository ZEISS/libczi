#include "azureblobinputstream.h"

#include <azure/storage/blobs.hpp>
#include <azure/identity/default_azure_credential.hpp>
#include <iostream>

using namespace std;

AzureBlobInputStream::AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag)
{
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


    
    // Initialize an instance of DefaultAzureCredential
    auto defaultAzureCredential = std::make_shared<Azure::Identity::DefaultAzureCredential>();
    cout << "DefaultAzureCredential"<<defaultAzureCredential->GetCredentialName() << endl;

    //auto accountURL = "https://<storage-account-name>.blob.core.windows.net";
    auto accountURL = "https://libczirwtestdata.blob.core.windows.net/";
    Azure::Storage::Blobs::BlobServiceClient blobServiceClient(accountURL, defaultAzureCredential);

    cout << "URL:" << blobServiceClient.GetUrl()<< endl;


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
    
}

void AzureBlobInputStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
    Azure::Storage::Blobs::DownloadBlobToOptions options;
    options.Range = Azure::Core::Http::HttpRange{(int64_t)offset, (int64_t)size };
    auto downloadResponse = this->blockBlobClient_->DownloadTo((uint8_t*)pv, (size_t)size, options);
    if (ptrBytesRead != nullptr)
    {
        *ptrBytesRead = size;
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
