// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "executeCreateCzi.h"
#include "IBitmapGen.h"
#include "inc_libCZI.h"

using namespace libCZI;
using namespace std;

class CExecuteCreateCzi
{
public:
    static bool execute(const CCmdLineOptions& options);
private:
    static void PrintInformationAboutJob(const std::wstring& outputfilename, const CCmdLineOptions& options);
    static void DoIt(const CCmdLineOptions& options, std::shared_ptr<libCZI::IOutputStream> outStream);
    static bool CalcTilePosition(const CreateTileInfo& tileInfo, const std::tuple<std::uint32_t, std::uint32_t>& size, uint32_t cnt, int& x, int& y, int& m);
    static void AddSubBlock(const libCZI::CDimCoordinate& coord, const CCmdLineOptions& options, IBitmapGen* bitmapGen, libCZI::ICziWriter* writer, std::function<bool(uint32_t cnt, int& x, int& y, int& m)> getPosAndM);
    static wstring GetCreateCziInformalTaskDescription(const CCmdLineOptions& options, const wchar_t* linebreak);
};

/*static*/void CExecuteCreateCzi::AddSubBlock(const CDimCoordinate& coord, const CCmdLineOptions& options, IBitmapGen* bitmapGen, ICziWriter* writer, function<bool(uint32_t cnt, int& x, int& y, int& m)> getPosAndM)
{
    BitmapGenInfo genInfo;
    genInfo.Clear();
    genInfo.coord = &coord;

    for (uint32_t cnt = 0;; ++cnt)
    {
        int x, y, m;
        if (!getPosAndM(cnt, x, y, m))
        {
            break;
        }

        genInfo.mValid = true;
        genInfo.mIndex = m;
        genInfo.tilePixelPosition = make_pair(x, y);

        auto bm = bitmapGen->Create(options.GetPixelGeneratorPixeltype(), get<0>(options.GetCreateBitmapSize()), get<1>(options.GetCreateBitmapSize()), genInfo);

        const void* sbBlkMetadata;
        uint32_t sbBlkMetadataSize;

        std::string sbMdXml;

        if (options.GetHasSubBlockKeyValueMetadata())
        {
            auto kv = options.GetSubBlockKeyValueMetadata();
            auto kvIter = kv.cbegin();

            // copy the key-value data into the sub-block-metadata
            auto sbBlkMd = Utils::CreateSubBlockMetadata(
                [&](int no, std::tuple<std::string, std::string>& nodeNameAndValue)->bool
                {
                    if (kvIter == kv.cend())
                    {
                        return false;
                    }

            nodeNameAndValue = make_tuple((*kvIter).first, (*kvIter).second);
            ++kvIter;
            return true;
                });

            sbMdXml = sbBlkMd->GetXml();
        }

        if (sbMdXml.empty())
        {
            sbBlkMetadata = nullptr;
            sbBlkMetadataSize = 0;
        }
        else
        {
            sbBlkMetadata = sbMdXml.c_str();;
            sbBlkMetadataSize = static_cast<uint32_t>(sbMdXml.size());
        }

        {
            ScopedBitmapLockerSP locker(bm);

            if (options.GetCompressionMode() == CompressionMode::Invalid || options.GetCompressionMode() == CompressionMode::UnCompressed)
            {
                AddSubBlockInfoStridedBitmap addInfo;
                addInfo.Clear();
                addInfo.coordinate = coord;
                addInfo.mIndexValid = true;
                addInfo.mIndex = m;
                addInfo.x = x;
                addInfo.y = y;
                addInfo.logicalWidth = bm->GetWidth();
                addInfo.logicalHeight = bm->GetHeight();
                addInfo.physicalWidth = bm->GetWidth();
                addInfo.physicalHeight = bm->GetHeight();
                addInfo.PixelType = bm->GetPixelType();
                addInfo.ptrBitmap = locker.ptrDataRoi;
                addInfo.strideBitmap = locker.stride;
                addInfo.ptrSbBlkMetadata = sbBlkMetadata;
                addInfo.sbBlkMetadataSize = sbBlkMetadataSize;
                writer->SyncAddSubBlock(addInfo);
            }
            else if (options.GetCompressionMode() == CompressionMode::Zstd1)
            {
                auto memblk = ZstdCompress::CompressZStd1Alloc(bm->GetWidth(), bm->GetHeight(), locker.stride, bm->GetPixelType(), locker.ptrDataRoi, options.GetCompressionParameters().get());

                AddSubBlockInfoMemPtr addInfo;
                addInfo.Clear();
                addInfo.coordinate = coord;
                addInfo.mIndexValid = true;
                addInfo.mIndex = m;
                addInfo.x = x;
                addInfo.y = y;
                addInfo.logicalWidth = bm->GetWidth();
                addInfo.logicalHeight = bm->GetHeight();
                addInfo.physicalWidth = bm->GetWidth();
                addInfo.physicalHeight = bm->GetHeight();
                addInfo.PixelType = bm->GetPixelType();
                addInfo.ptrData = memblk->GetPtr();
                addInfo.dataSize = memblk->GetSizeOfData();
                addInfo.ptrSbBlkMetadata = sbBlkMetadata;
                addInfo.sbBlkMetadataSize = sbBlkMetadataSize;
                addInfo.SetCompressionMode(CompressionMode::Zstd1);
                writer->SyncAddSubBlock(addInfo);
            }
            else if (options.GetCompressionMode() == CompressionMode::Zstd0)
            {
                auto memblk = ZstdCompress::CompressZStd0Alloc(bm->GetWidth(), bm->GetHeight(), locker.stride, bm->GetPixelType(), locker.ptrDataRoi, options.GetCompressionParameters().get());

                AddSubBlockInfoMemPtr addInfo;
                addInfo.Clear();
                addInfo.coordinate = coord;
                addInfo.mIndexValid = true;
                addInfo.mIndex = m;
                addInfo.x = x;
                addInfo.y = y;
                addInfo.logicalWidth = bm->GetWidth();
                addInfo.logicalHeight = bm->GetHeight();
                addInfo.physicalWidth = bm->GetWidth();
                addInfo.physicalHeight = bm->GetHeight();
                addInfo.PixelType = bm->GetPixelType();
                addInfo.ptrData = memblk->GetPtr();
                addInfo.dataSize = memblk->GetSizeOfData();
                addInfo.ptrSbBlkMetadata = sbBlkMetadata;
                addInfo.sbBlkMetadataSize = sbBlkMetadataSize;
                addInfo.SetCompressionMode(CompressionMode::Zstd0);
                writer->SyncAddSubBlock(addInfo);
            }
        }
    }
}

/*static*/bool CExecuteCreateCzi::execute(const CCmdLineOptions& options)
{
    BitmapGenFactory::InitializeFactory();

    std::wstring outputfilename = options.MakeOutputFilename(L"", L"czi");

    CExecuteCreateCzi::PrintInformationAboutJob(outputfilename, options);

    // Create an "output-stream-object"
    auto outStream = libCZI::CreateOutputStreamForFile(outputfilename.c_str(), true);

    CExecuteCreateCzi::DoIt(options, outStream);

    return true;
}

/*static*/void CExecuteCreateCzi::PrintInformationAboutJob(const std::wstring& outputfilename, const CCmdLineOptions& options)
{
    std::wstringstream ss;
    ss << L"Creating output-file \"" << outputfilename << L"\"." << endl;
    ss << endl;
    ss << CExecuteCreateCzi::GetCreateCziInformalTaskDescription(options, L"\n") << endl;

    options.GetLog()->WriteLineStdOut(ss.str().c_str());
}

/*static*/wstring CExecuteCreateCzi::GetCreateCziInformalTaskDescription(const CCmdLineOptions& options, const wchar_t* linebreak)
{
    wstringstream ss;
    ss << L"Bounds:    " << convertUtf8ToUCS2(Utils::DimBoundsToString(&options.GetCreateBounds())) << linebreak;
    const auto& tileSize = options.GetCreateBitmapSize();
    ss << L"Tile-size: " << get<0>(tileSize) << L" x " << get<1>(tileSize) << linebreak;
    const auto tileInfo = options.GetCreateTileInfo();
    ss << L"Mosaic:    ";
    if (tileInfo.IsValid())
    {
        ss << tileInfo.rows << L" row" << (tileInfo.rows > 1 ? L"s" : L"") << L" by " << tileInfo.columns << L" column" << (tileInfo.columns > 1 ? L"s" : L"") << L" with " << (int)(tileInfo.overlap * 100) << L"% overlap";;
    }
    else
    {
        ss << L"no";
    }

    return ss.str();
}

/*static*/void CExecuteCreateCzi::DoIt(const CCmdLineOptions& options, std::shared_ptr<IOutputStream> outStream)
{
    // create the "CZI-writer"-object
    auto writer = libCZI::CreateCZIWriter();

    // initialize the "CZI-writer-object" with the "output-stream-object"
    // notes: (1) not sure if we should/have to provide a "bounds" at initialization
    //        (2) the bounds provided here _could_ be used to create a suitable sized subblk-directory at the
    //             beginning of the file AND for checking the validity of the subblocks added later on
    //        (3) ...both things are not really necessary from a technical point of view, however... consistency-
    //             checking I'd consider an important feature

    auto spWriterInfo = make_shared<CCziWriterInfo>(options.GetIsFileGuidValid() ? options.GetFileGuid() : libCZI::GUID{ 0,0,0,{ 0,0,0,0,0,0,0,0 } });
    //spWriterInfo->SetReservedSizeForMetadataSegment(true, 10 * 1024);
    //spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 400);
    writer->Create(outStream, spWriterInfo);

    // there should always be a C-dimension (very strongly recommended)
    auto bounds{ options.GetCreateBounds() };
    if (!bounds.IsValid(DimensionIndex::C))
    {
        bounds.Set(DimensionIndex::C, 0, 1);
    }

    // utility-object used to create an "articial bitmap"
    CBitmapGenParameters bitmapGenParameters;
    bitmapGenParameters.SetFontFilename(convertToUtf8(options.GetFontNameOrFile()));
    bitmapGenParameters.SetFontHeight(options.GetFontHeight());

    auto bitmapGeneratorClassName = options.GetBitmapGeneratorClassName();
    auto get = BitmapGenFactory::CreateBitmapGenerator(bitmapGeneratorClassName.empty() ? "default" : bitmapGeneratorClassName.c_str(), &bitmapGenParameters);

    // loop through all coordinates within the bounds
    Utils::EnumAllCoordinates(bounds,
        [&](uint64_t cnt, const libCZI::CDimCoordinate& coord)->bool
        {
            auto str = Utils::DimCoordinateToString(&coord);

    std::stringstream ss;
    ss << "Writing subblock #" << cnt << " coordinate: " << str << " ";
    options.GetLog()->WriteStdOut(ss.str().c_str());

    const auto tileInfo = options.GetCreateTileInfo();

    // loop through all tiles
    AddSubBlock(coord, options, get.get(), writer.get(),
        [&](uint32_t cntTiles, int& x, int& y, int& m)->bool
        {
            // if no tile-info is given, just write one tile
            if (!tileInfo.IsValid())
            {
                if (cntTiles == 0)
                {
                    x = y = m = 0;
                    options.GetLog()->WriteLineStdOut("(no M).");
                    return true;
                }

                return false;
            }

    bool b = CExecuteCreateCzi::CalcTilePosition(tileInfo, options.GetCreateBitmapSize(), cntTiles, x, y, m);
    if (b)
    {
        stringstream ss;
        if (cntTiles == 0)
        {
            ss << "M=" << m;
        }
        else
        {
            ss << ", " << m;
        };

        options.GetLog()->WriteStdOut(ss.str().c_str());
    }
    else
    {
        options.GetLog()->WriteLineStdOut(".");
    }

    return b;
        });

    return true;
        });

    get.reset();    // not needed any more

    // get "partially filled out" metadata - the metadata contains information which was derived from the 
    //  subblocks added, in particular we "pre-fill" the Size-information, and the Pixeltype-information
    PrepareMetadataInfo prepareInfo;
    prepareInfo.funcGenerateIdAndNameForChannel = [](int channelIndex)->tuple<string, tuple<bool, string>>
    {
        stringstream ssId, ssName;
        ssId << "Channel:" << channelIndex;
        ssName << "Channel #" << channelIndex;
        return make_tuple(ssId.str(), make_tuple(true, ssName.str()));
    };

    auto mdBldr = writer->GetPreparedMetadata(prepareInfo);

    // now we could add additional information
    GeneralDocumentInfo docInfo;
    docInfo.SetName(options.GetOutputFilename());
    docInfo.SetTitle(L"CZICmd generated");
    docInfo.SetComment(GetCreateCziInformalTaskDescription(options, L"; "));
    MetadataUtils::WriteGeneralDocumentInfo(mdBldr.get(), docInfo);

    mdBldr->GetRootNode()->GetOrCreateChildNode("Metadata/Information/Application/Name")->SetValue("libCZIrw-Test");
    mdBldr->GetRootNode()->GetOrCreateChildNode("Metadata/Information/Application/Version")->SetValue("0.01");

    // the resulting metadata-information is written to the CZI here
    auto xml = mdBldr->GetXml(true);
    WriteMetadataInfo writerMdInfo = { 0 };
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();
    writer->SyncWriteMetadata(writerMdInfo);

    // TEST
    ////AddAttachmentInfo attchmntInfo;
    ////attchmntInfo.ptrData = attachmentData;
    ////attchmntInfo.dataSize = sizeof(attachmentData);
    ////attchmntInfo.SetName("Thumbnail");
    ////attchmntInfo.SetContentFileType("JPG");
    ////attchmntInfo.contentGuid = { 0xB13BC88F,0x37A5,0x444,{ 0x94,0x93 , 0x28,0xC0,0x7C,0x3B,0xCE,0x85 } };
    ////writer->SyncAddAttachment(attchmntInfo);

    // this finishes the write-operation, the subblock-directory-segment is written out to disk and the CZI
    //  is finalized.
    writer->Close();
}

/*static*/bool CExecuteCreateCzi::CalcTilePosition(const CreateTileInfo& tileInfo, const std::tuple<std::uint32_t, std::uint32_t>& size, uint32_t cnt, int& x, int& y, int& m)
{
    uint32_t r = cnt / tileInfo.columns;
    uint32_t c = cnt - r * tileInfo.columns;
    if (r >= tileInfo.rows || c >= tileInfo.columns)
    {
        return false;
    }

    m = cnt;
    x = c * get<0>(size) - static_cast<uint32_t>(lrint(c * get<0>(size) * tileInfo.overlap));
    y = r * get<1>(size) - static_cast<uint32_t>(lrint(r * get<1>(size) * tileInfo.overlap));
    return true;
}

bool executeCreateCzi(const CCmdLineOptions& options)
{
    return CExecuteCreateCzi::execute(options);
}
