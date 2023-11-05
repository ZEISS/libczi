#include "../libCZI/libCZI.h"
#include "save_bitmap.h"
#include <Windows.h>

using namespace libCZI;
using namespace std;

class CLibCZISite : public libCZI::ISite
{
    libCZI::ISite* pSite;
public:
    explicit CLibCZISite()
    {
        //this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithWICDecoder);
        this->pSite = libCZI::GetDefaultSiteObject(libCZI::SiteObjectType::WithJxrDecoder);
    }

    bool IsEnabled(int logLevel) override
    {
       // return false;
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
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CLibCZISite site;
    libCZI::SetSiteObject(&site);

    auto stream = StreamsFactory::CreateDefaultStreamForFile(R"(D:\OneDrive\jbohl\OneDrive\Z\libCZI-Jpgxr-performance-issue\20200903_RS013_AJB02_000.czi)");

    //auto stream = StreamsFactory::CreateDefaultStreamForFile(R"(n:\uncompressed_20200903_RS013_AJB02_000.czi)");

    auto reader = libCZI::CreateCZIReader();
    reader->Open(stream);
    const auto statistics = reader->GetStatistics();

    auto accessor = reader->CreateSingleChannelScalingTileAccessor();

    int size_x = 2000;
    int size_y = 2000;

    auto save_bitmap = CSaveBitmapFactory::CreateDefaultSaveBitmapObj();

    //for (int y = 0; y < statistics.boundingBoxLayer0Only.h / size_y; ++y)
    {
        //for (int x = 0; x < statistics.boundingBoxLayer0Only.w / size_x; ++x)
        for (int i=0; i < 1; ++i)
        {
            CDimCoordinate coordinate
            {
                { DimensionIndex::C, 0 },
            };

            ISingleChannelScalingTileAccessor::Options options;
            options.backGroundColor = RgbFloatColor{ 0, 0, 0 };
            options.sceneFilter = libCZI::Utils::IndexSetFromString(L"0");
            options.useCoverageOptimization = true;

            auto bitmap = accessor->Get(
                IntRect
                {
                    -126748,// statistics.boundingBoxLayer0Only.x + x * size_x,
                    46095,//statistics.boundingBoxLayer0Only.y + y * size_y,
                    size_x,
                    size_y
                },
                &coordinate,
                1,
                &options);

            /*
            wstringstream wss;
            wss << "N:\\testout\\x=" << x << "_y=" << y << ".png";
            save_bitmap->Save(wss.str().c_str(), SaveDataFormat::PNG, bitmap.get());
            */
        }
    }

    return 0;
}
