#include "../libCZI/libCZI.h"

using namespace libCZI;
using namespace std;

class CLibCZISite : public libCZI::ISite
{
    libCZI::ISite* pSite;
    public:
    explicit CLibCZISite() 
    {
#if defined(WIN32ENV)
        if (options.GetUseWICJxrDecoder())
        {
            this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithWICDecoder);
        }
        else
        {
            this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithJxrDecoder);
        }
#else
        this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::Default);
#endif
    }

    bool IsEnabled(int logLevel) override
    {
        return true;
    }

    void Log(int level, const char* szMsg) override
    {
        printf("%s\n", szMsg);
    }

    std::shared_ptr<libCZI::IDecoder> GetDecoder(libCZI::ImageDecoderType type, const char* arguments) override
    {
        return this->pSite->GetDecoder(type, arguments);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t extraRows, std::uint32_t extraColumns) override
    {
        return this->pSite->CreateBitmap(pixeltype, width, height, stride, extraRows, extraColumns);
    }
};


int main(int argc, char* argv[])
{
    CLibCZISite site;
    libCZI::SetSiteObject(&site);

    //auto stream = StreamsFactory::CreateDefaultStreamForFile(R"(D:\OneDrive\jbohl\OneDrive\Z\libCZI-Jpgxr-performance-issue\20200903_RS013_AJB02_000.czi)");

    auto stream = StreamsFactory::CreateDefaultStreamForFile(R"(n:\uncompressed_20200903_RS013_AJB02_000.czi)");

    auto reader = libCZI::CreateCZIReader();
    reader->Open(stream);
    const auto statistics = reader->GetStatistics();

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    int size_x = 2000;
    int size_y = 2000;

    for (int y = 0; y < statistics.boundingBoxLayer0Only.h / size_y; ++y)
    {
        for (int x = 0; x < statistics.boundingBoxLayer0Only.w / size_x; ++x)
        {
            CDimCoordinate coordinate
            {
                { DimensionIndex::C, 0 },
            };

            auto bitmap = accessor->Get(
                IntRect
                {
                    statistics.boundingBoxLayer0Only.x + x * size_x,
                    statistics.boundingBoxLayer0Only.y + y * size_y,
                    size_x,
                    size_y
                },
                &coordinate,
                1,
                nullptr);
        }
    }
    // Load the image
    //    CZI::Image image;
    //       image.Load("test.jxr");
       // Get the image size
       //    const int width = image.GetWidth();
       //       const int height = image.GetHeight();
          // Get the pixel data
          //    const CZI::Pixel* pixels = image.GetPixels();
             // Print the first pixel
             //    printf("First pixel: %d %d %d %d\n",
             //           pixels[0].r,
             //                  pixels[0].g,
             //                         pixels[0].b,
             //                                pixels[0].a);
                // Print the last pixel
                //    printf("Last pixel: %d %d %d %d\n",
                //           pixels[width * height - 1].r,
                //                  pixels[width * height - 1].g,
                //                         pixels[width * height - 1].b,
                //                                pixels[width * height - 1].a);
    return 0;
}
