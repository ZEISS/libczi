stream objects                 {#stream_objects_}
==============

# stream objects

All input/output operations in libCZI are done through stream objects. Stream objects are used by the CZIReader to access the data in a CZI-file.
The stream object is an abstraction of a random-access stream.   
libCZI defines three different stream objects - read-only streams, write-only streams and read-write streams. The respective 
interfaces are: IStream, IOutputStream and IInputOutputStream.
libCZI provides implementations for reading from a file and for writing to a file in the file-system.  
In addition, there is an experimental implementation for reading from an http(s)-server. This implementation is based on [libcurl](https://curl.se/libcurl/) and allows 
reading from a CZI-file which is located on a web-server.

For creating a stream object for reading, a class factory is provided (in the file libCZI_StreamsLib.h).

## Azure-SDK reader

This reader implementation is based on the [Azure-SDK C++ library](https://github.com/Azure/azure-sdk-for-cpp). It allows 
reading from a CZI-file which is located on an Azure Blob Storage.
This azure-input-stream is intended to manage authentication with the functionality provided by the
Azure-SDK. Please see https://learn.microsoft.com/en-us/azure/storage/blobs/quickstart-blobs-c-plus-plus?tabs=connection-string%2Croles-azure-portal#authenticate-to-azure-and-authorize-access-to-blob-data
for the concepts followed in this implementation.
The idea is:
* For this stream object, the uri-string provided by the user contains the necessary information to identify the blob  
   to operate on in an Azure Blob Storage account.
* For this uri-string, we define a syntax (detailed below) that gives the information in a key-value form.  
* In addition, the property-bag is used to provide additional information controlling the operation.    

The syntax for the uri-string is: `<key1>=<value1>;<key2>=<value2>`.  
The following rules apply:
* Key-value pairs are separated by a semicolon ';'.
* A equal sign '=' separates the key from the value.
* Spaces are significant, they are part of the key or value. So, the key "key" is different from the key " key".  
* A semicolon or an equal sign can be part of the key or value if it is escaped by a backslash '\\'.  
* Empty keys or values are not allowed.  

The reader has multiple modes of operation - mainly differing in how the authentication is done:

  mode of operation            |  description
--------------------------------|------------------------------------------------------------
| DefaultAzureCredential        | This method uses the Azure SDK's DefaultAzureCredential to authenticate with Azure Blob Storage.  
| EnvironmentCredential         | This method uses the Azure SDK's EnvironmentCredential to authenticate with Azure Blob Storage.
| AzureCliCredential            | 
| ManagedIdentityCredential     |   
| ConnectionStringCredential    | Use a connection string (which includes the storage account access keys) to authorize requests to Azure Blob Storage.