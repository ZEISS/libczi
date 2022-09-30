// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "SegmentWalker.h"
#include "utils.h"
#include "testImage.h"
#include "../libCZI/decoder_zstd.h"
#include "../libCZI/CziWriter.h"
#include "../libCZI/CZIReader.h"

using namespace libCZI;
using namespace std;

//! .
//! .
/**
 * \brief   Compresses image either with ZStd0 algorithm or ZStd1, depending on passed compression mode parameter.
 *          The passed 'params' parameter can be null, then the compression uses default compression level.
 * \param   img     The buffer with bitmap data.
 * \param   param   The pointer to the parameters used to set compression level and low-high bytes packing.
 * \param   mode    The compression mode to identify whether it should compress with Zstd1 or Zstd0 algorithm.
 *                  It should not use any other mode. If other mode is used, it compresses using Zstd0.
 * \param   block [out] On output this contains information of image block to write in the stream.
 */
static void _compressImage(const std::shared_ptr<libCZI::IBitmapData>& img, const ICompressParameters* params, CompressionMode mode, AddSubBlockInfoMemPtr& block)
{
    EXPECT_TRUE((mode == CompressionMode::Zstd0) || (mode == CompressionMode::Zstd1)) << "Unsupported compression mode. Will use default Zstd0";

    const uint32_t width = img->GetWidth();
    const uint32_t height = img->GetHeight();
    const PixelType pixelType = img->GetPixelType();
    const size_t maxSize = mode == CompressionMode::Zstd1
        ? ZstdCompress::CalculateMaxCompressedSizeZStd1(width, height, pixelType)
        : ZstdCompress::CalculateMaxCompressedSizeZStd0(width, height, pixelType);

    uint8_t* buffer = new uint8_t[maxSize];

    size_t imgSize = maxSize;
    ScopedBitmapLockerSP imgLock{ img };
    bool result = mode == CompressionMode::Zstd1
        ? ZstdCompress::CompressZStd1(width, height, imgLock.stride, pixelType, imgLock.ptrDataRoi, buffer, imgSize, params)
        : ZstdCompress::CompressZStd0(width, height, imgLock.stride, pixelType, imgLock.ptrDataRoi, buffer, imgSize, params);

    EXPECT_TRUE(result) << "Failed to compress bitmap image";
    EXPECT_TRUE(maxSize >= imgSize) << "Unexpected compress image size";

    block.Clear();

    block.coordinate = CDimCoordinate::Parse("C0");
    block.mIndexValid = true;
    block.mIndex = 0;
    block.x = 0;
    block.y = 0;
    block.logicalWidth = width;
    block.logicalHeight = height;
    block.physicalWidth = width;
    block.physicalHeight = height;
    block.PixelType = pixelType;

    block.ptrData = buffer;
    block.dataSize = imgSize;
    block.SetCompressionMode(mode == CompressionMode::Zstd1 ? CompressionMode::Zstd1 : CompressionMode::Zstd0);
}

//! Compresses image with ZStd0 algorithm. The passed 'params' parameter can be nullptr if it should compress
//! with default compression parameters.
static void _compressImageZStd0(const std::shared_ptr<libCZI::IBitmapData>& img, const ICompressParameters* params, AddSubBlockInfoMemPtr& block)
{
    _compressImage(img, params, CompressionMode::Zstd0, block);
}

//! Compresses image with ZStd1 algorithm. The passed 'params' parameter can be nullptr if it should compress
//! with default compression parameters.
static void _compressImageZStd1(const std::shared_ptr<libCZI::IBitmapData>& img, const ICompressParameters* params, AddSubBlockInfoMemPtr& block)
{
    _compressImage(img, params, CompressionMode::Zstd1, block);
}

/**
 * \brief   Tests writing image compressed with ZStd0 algorithm.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   params  The compression parameters. If nullptr the default parameters are used.
 */
static void _testWriteCompressedImageZStd0(uint32_t width, uint32_t height, PixelType pixelType, const ICompressParameters* params)
{
    constexpr GUID guid = { 0x14e7389b, 0x57bb, 0x4eb4, { 0xa3, 0x8a, 0xda, 0xb4, 0xd1, 0x2, 0xce, 0x2e } };
    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, width, height);

    CCziWriter writer;
    writer.Create(make_shared<CMemOutputStream>(0), make_shared<CCziWriterInfo >(guid));

    AddSubBlockInfoMemPtr block;
    _compressImageZStd0(img, params, block);

    writer.ICziWriter::SyncAddSubBlock(block);

    uint8_t* compressed = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(block.ptrData));
    delete[] compressed;

    writer.Close();
}

/**
 * \brief   Makes test of writing ZStd0 compressed image to use default compression parameters.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 */
static void _testWriteCompressedImageZStd0Basic(uint32_t width, uint32_t height, PixelType pixelType)
{
    _testWriteCompressedImageZStd0(width, height, pixelType, nullptr);
}

/**
 * \brief   Makes test of writing ZStd0 compressed image to specified compression level.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 */
static void _testWriteCompressedImageZStd0Level(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);

    _testWriteCompressedImageZStd0(width, height, pixelType, &params);
}

/**
 * \brief   Tests writing image compressed with ZStd1 algorithm.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   params  The compression parameters. If nullptr the default parameters are used.
 */
static void _testWriteCompressedImageZStd1(uint32_t width, uint32_t height, PixelType pixelType, const ICompressParameters* params)
{
    constexpr GUID guid = { 0x14e7389b, 0x57bb, 0x4eb4, { 0xa3, 0x8a, 0xda, 0xb4, 0xd1, 0x2, 0xce, 0x2e } };
    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, width, height);

    CCziWriter writer;
    writer.Create(make_shared<CMemOutputStream>(0), make_shared<CCziWriterInfo >(guid));

    AddSubBlockInfoMemPtr block;
    _compressImageZStd1(img, params, block);

    writer.ICziWriter::SyncAddSubBlock(block);

    uint8_t* compressed = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(block.ptrData));
    delete[] compressed;

    writer.Close();
}

/**
 * \brief   Makes test of writing ZStd1 compressed image to use default compression parameters.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 */
static void _testWriteCompressedImageZStd1Basic(uint32_t width, uint32_t height, PixelType pixelType)
{
    _testWriteCompressedImageZStd1(width, height, pixelType, nullptr);
}

/**
 * \brief   Makes test of writing ZStd1 compressed image to specified compression level.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 */
static void _testWriteCompressedImageZStd1Level(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);

    _testWriteCompressedImageZStd1(width, height, pixelType, &params);
}

/**
 * \brief   Makes test of writing ZStd1 compressed image to specified compression level
 *          and low-high byte packing enabling.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 * \param   lowPacking  If true, the low-high byte packing is enabled.
 */
static void _testWriteCompressedImageZStd1LowPacking(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level, bool lowPacking)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);
    params.map[keyLowPack] = CompressParameter(lowPacking);

    _testWriteCompressedImageZStd1(width, height, pixelType, &params);
}

/**
 * \brief   Tests writing and reading image compressed with ZStd0 algorithm.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   params  The compression parameters. If nullptr the default parameters are used.
 */
static void _testWriteReadCompressedImageZStd0(uint32_t width, uint32_t height, PixelType pixelType, const ICompressParameters* params)
{
    constexpr GUID guid{ 0x26893eb7, 0x598a, 0x4f5d, { 0x8e, 0x9, 0x87, 0x4e, 0xa1, 0xd2, 0x76, 0xe9 } };
    const CDimCoordinate coordinate{ { DimensionIndex::Z,0 } ,{ DimensionIndex::C, 0 } };

    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, width, height);

    uint8_t* compressed = nullptr;
    size_t sizeCompressed = 0;

    std::shared_ptr<void> buffer;
    size_t sizeBuffer = 0;

    do
    {
        // write
        auto stream = make_shared<CMemOutputStream>(0);
        CCziWriter writer;
        writer.Create(stream, make_shared<CCziWriterInfo >(guid));

        AddSubBlockInfoMemPtr block;
        _compressImageZStd0(img, params, block);
        block.coordinate = coordinate;

        compressed = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(block.ptrData));
        sizeCompressed = block.dataSize;

        static_cast<libCZI::ICziWriter&>(writer).SyncAddSubBlock(block);

        auto metadataBuilder = writer.GetPreparedMetadata(PrepareMetadataInfo());
        string xml = metadataBuilder->GetXml(true);
        WriteMetadataInfo writerMdInfo = { 0 };
        writerMdInfo.szMetadata = xml.c_str();
        writerMdInfo.szMetadataSize = xml.size();

        writer.SyncWriteMetadata(writerMdInfo);
        writer.Close();

        buffer = stream->GetCopy(&sizeBuffer);
        EXPECT_TRUE(buffer != nullptr);
        EXPECT_TRUE(sizeBuffer != 0);

    } while (false);

    do
    {
        auto stream = CreateStreamFromMemory(buffer, sizeBuffer);
        CCZIReader reader;
        reader.Open(stream);

        std::shared_ptr<ISubBlock> sbBlkRead = reader.ReadSubBlock(0);
        size_t sizeBlock = 0;
        std::shared_ptr<const void> imgBlock = sbBlkRead->GetRawData(ISubBlock::MemBlkType::Data, &sizeBlock);

        EXPECT_TRUE(sizeBlock != 0);
        EXPECT_TRUE(imgBlock != nullptr);
        EXPECT_EQ(sizeBlock, sizeCompressed);
        EXPECT_TRUE(memcmp(compressed, imgBlock.get(), sizeCompressed) == 0) << "Unexpected image block";

        std::shared_ptr<CZstd0Decoder> dec = CZstd0Decoder::Create();
        std::shared_ptr<libCZI::IBitmapData> decImg = dec->Decode(imgBlock.get(), sizeBlock, pixelType, width, height);
        ScopedBitmapLockerSP lockDecode{ decImg };
        ScopedBitmapLockerSP lockOrigin{ img };

        EXPECT_TRUE(decImg != nullptr) << "Failed to create decoded image";
        EXPECT_TRUE(decImg->GetHeight() == height) << "The decoded image has wrong height";
        EXPECT_TRUE(decImg->GetWidth() == width) << "The decoded image has wrong width";
        EXPECT_TRUE(decImg->GetPixelType() == pixelType) << "The decoded image has wrong pixel type";

        const uint8_t* origin = reinterpret_cast<const uint8_t*>(lockOrigin.ptrDataRoi);
        const uint8_t* decode = reinterpret_cast<const uint8_t*>(lockDecode.ptrDataRoi);
        const uint32_t strideOrigin = lockOrigin.stride;
        const uint32_t strideDecode = lockDecode.stride;
        const uint32_t line = width * Utils::GetBytesPerPixel(pixelType);

        for (uint32_t i = 0; i < height; ++i)
        {
            EXPECT_TRUE(memcmp(origin, decode, line) == 0);
            origin += strideOrigin;
            decode += strideDecode;
        }

        reader.Close();

    } while (false);

    if (compressed != nullptr)
        delete[] compressed;
}

/**
 * \brief   Makes test of writing and reading ZStd0 compressed image to use default compression parameters.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 */
static void _testWriteReadCompressedImageZStd0Basic(uint32_t width, uint32_t height, PixelType pixelType)
{
    _testWriteReadCompressedImageZStd0(width, height, pixelType, nullptr);
}

/**
 * \brief   Makes test of writing and reading ZStd0 compressed image to specified compression level.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 */
static void _testWriteReadCompressedImageZStd0Level(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);

    _testWriteReadCompressedImageZStd0(width, height, pixelType, &params);
}

/**
 * \brief   Tests writing and reading image compressed with ZStd1 algorithm.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   params  The compression parameters. If nullptr the default parameters are used.
 */
static void _testWriteReadCompressedImageZStd1(uint32_t width, uint32_t height, PixelType pixelType, const ICompressParameters* params)
{
    constexpr GUID guid{ 0x26893eb7, 0x598a, 0x4f5d, { 0x8e, 0x9, 0x87, 0x4e, 0xa1, 0xd2, 0x76, 0xe9 } };
    const CDimCoordinate coordinate{ { DimensionIndex::Z,0 } ,{ DimensionIndex::C, 0 } };

    std::shared_ptr<libCZI::IBitmapData> img = CreateRandomBitmap(pixelType, width, height);

    uint8_t* compressed = nullptr;
    size_t sizeCompressed = 0;

    std::shared_ptr<void> buffer;
    size_t sizeBuffer = 0;

    do
    {
        // write
        auto stream = make_shared<CMemOutputStream>(0);
        CCziWriter writer;
        writer.Create(stream, make_shared<CCziWriterInfo >(guid));

        AddSubBlockInfoMemPtr block;
        _compressImageZStd1(img, params, block);
        block.coordinate = coordinate;

        compressed = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(block.ptrData));
        sizeCompressed = block.dataSize;

        static_cast<libCZI::ICziWriter&>(writer).SyncAddSubBlock(block);

        auto metadataBuilder = writer.GetPreparedMetadata(PrepareMetadataInfo());
        string xml = metadataBuilder->GetXml(true);
        WriteMetadataInfo writerMdInfo = { 0 };
        writerMdInfo.szMetadata = xml.c_str();
        writerMdInfo.szMetadataSize = xml.size();

        writer.SyncWriteMetadata(writerMdInfo);
        writer.Close();

        buffer = stream->GetCopy(&sizeBuffer);
        EXPECT_TRUE(buffer != nullptr);
        EXPECT_TRUE(sizeBuffer != 0);

    } while (false);

    do
    {
        auto stream = CreateStreamFromMemory(buffer, sizeBuffer);
        CCZIReader reader;
        reader.Open(stream);

        std::shared_ptr<ISubBlock> sbBlkRead = reader.ReadSubBlock(0);
        size_t sizeBlock = 0;
        std::shared_ptr<const void> imgBlock = sbBlkRead->GetRawData(ISubBlock::MemBlkType::Data, &sizeBlock);

        EXPECT_TRUE(sizeBlock != 0);
        EXPECT_TRUE(imgBlock != nullptr);
        EXPECT_EQ(sizeBlock, sizeCompressed);
        EXPECT_TRUE(memcmp(compressed, imgBlock.get(), sizeCompressed) == 0) << "Unexpected image block";

        std::shared_ptr<CZstd1Decoder> dec = CZstd1Decoder::Create();
        std::shared_ptr<libCZI::IBitmapData> decImg = dec->Decode(imgBlock.get(), sizeBlock, pixelType, width, height);

        EXPECT_TRUE(decImg != nullptr) << "Failed to create decoded image";
        EXPECT_TRUE(decImg->GetHeight() == height) << "The decoded image has wrong height";
        EXPECT_TRUE(decImg->GetWidth() == width) << "The decoded image has wrong width";
        EXPECT_TRUE(decImg->GetPixelType() == pixelType) << "The decoded image has wrong pixel type";

        EXPECT_TRUE(AreBitmapDataEqual(img, decImg)) << "The bitmaps are not equal";

        reader.Close();

    } while (false);

    if (compressed != nullptr)
        delete[] compressed;
}

/**
 * \brief   Makes test of writing and reading ZStd1 compressed image to use default compression parameters.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 */
static void _testWriteReadCompressedImageZStd1Basic(uint32_t width, uint32_t height, PixelType pixelType)
{
    _testWriteReadCompressedImageZStd1(width, height, pixelType, nullptr);
}

/**
 * \brief   Makes test of writing and reading ZStd1 compressed image to specified compression level.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 */
static void _testWriteReadCompressedImageZStd1Level(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);

    _testWriteReadCompressedImageZStd1(width, height, pixelType, &params);
}

/**
 * \brief   Makes test of writing and reading ZStd1 compressed image to specified compression level
 *          and low-high byte packing enabling.
 * \param   width   The width of image in pixels.
 * \param   height  The height of image in pixels.
 * \param   pixelType   The type of pixel. Supports Gray8, Gray16, Bgr24 and Bgr48.
 * \param   level   The compression level.
 * \param   lowPacking  If true, the low-high byte packing is enabled.
 */
static void _testWriteReadCompressedImageZStd1LowPacking(uint32_t width, uint32_t height, PixelType pixelType, uint32_t level, bool lowPacking)
{
    constexpr int32_t keyLevel = static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL);
    constexpr int32_t keyLowPack = static_cast<int32_t>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING);

    libCZI::CompressParametersOnMap params;
    params.map[keyLevel] = CompressParameter(level);
    params.map[keyLowPack] = CompressParameter(lowPacking);
    params.map[keyLevel] = CompressParameter(level);

    _testWriteReadCompressedImageZStd1(width, height, pixelType, &params);
}

//////////////////////////////////////////////////////////////////////////
/// Unit tests
//////////////////////////////////////////////////////////////////////////

TEST(CziWriter, WriteBitmapGray8)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();

    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;

    writer->SyncAddSubBlock(addSbBlkInfo);
    writer->Close();

    uint8_t hash[16];
    Utils::CalcMd5SumHash(outStream->GetDataC(), outStream->GetDataSize(), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x3f,0xf0,0x0c,0x5c,0x91,0x8b,0x8a,0xbb,0xd8,0x15,0xa0,0x06,0xb3,0x35,0xdc,0x05 };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";

    WriteOutTestCzi("CziWriter", "Writer1", outStream->GetDataC(), outStream->GetDataSize());
}

TEST(CziWriter, Writer2)
{
    // check that duplicate entries are rejected

    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);
    writer->Create(outStream, nullptr);
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0T0");
    addSbBlkInfo.mIndexValid = 0;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    bool expectedExceptionCaught = false;

    try
    {
        writer->SyncAddSubBlock(addSbBlkInfo);
    }
    catch (LibCZIWriteException& e)
    {
        if (e.GetErrorType() == LibCZIWriteException::ErrorType::AddCoordinateAlreadyExisting)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "Incorrect behavior";
}

TEST(CziWriter, Writer3)
{
    // check that duplicate entries are rejected

    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);
    writer->Create(outStream, nullptr);
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    static const uint8_t attachmentData[10] = { 1,2,3,4,5,6,7,8,9,10 };

    AddAttachmentInfo attchmntInfo;
    attchmntInfo.ptrData = attachmentData;
    attchmntInfo.dataSize = sizeof(attachmentData);
    attchmntInfo.SetName("Thumbnail");
    attchmntInfo.SetContentFileType("JPG");
    attchmntInfo.contentGuid = { 0xB13BC88F,0x37A5,0x444,{ 0x94,0x93 , 0x28,0xC0,0x7C,0x3B,0xCE,0x85 } };
    writer->SyncAddAttachment(attchmntInfo);

    bool expectedExceptionCaught = false;

    try
    {
        writer->SyncAddAttachment(attchmntInfo);
    }
    catch (LibCZIWriteException& e)
    {
        if (e.GetErrorType() == LibCZIWriteException::ErrorType::AddAttachmentAlreadyExisting)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "Incorrect behavior";
}

TEST(CziWriter, WriteAndReadBitmapBgr24)
{
    auto outputStream = make_shared<CMemOutputStream>(0);
    auto cziWriter = CreateCZIWriter();
    cziWriter->Create(outputStream, nullptr);

    auto bitmap = CreateTestBitmap(PixelType::Bgr24, 16, 16);

    CDimCoordinate coord{ { DimensionIndex::Z,0 } ,{ DimensionIndex::C, 0 } };

    {
        ScopedBitmapLockerSP lockBm{ bitmap };
        AddSubBlockInfoStridedBitmap addInfo;
        addInfo.coordinate = coord;
        addInfo.mIndexValid = true;
        addInfo.mIndex = 0;
        addInfo.x = 0;
        addInfo.y = 0;
        addInfo.logicalWidth = bitmap->GetWidth();
        addInfo.logicalHeight = bitmap->GetHeight();
        addInfo.physicalWidth = bitmap->GetWidth();
        addInfo.physicalHeight = bitmap->GetHeight();
        addInfo.PixelType = PixelType::Bgr24;
        addInfo.ptrBitmap = lockBm.ptrDataRoi;
        addInfo.strideBitmap = lockBm.stride;

        cziWriter->SyncAddSubBlock(addInfo);
    }

    auto metadataBuilder = cziWriter->GetPreparedMetadata(PrepareMetadataInfo());
    string xml = metadataBuilder->GetXml(true);
    WriteMetadataInfo writerMdInfo = { 0 };
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();

    cziWriter->SyncWriteMetadata(writerMdInfo);

    cziWriter->Close();

    cziWriter.reset();		// not needed anymore

    size_t cziData_Size;
    auto cziData = outputStream->GetCopy(&cziData_Size);
    outputStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);

    auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto sbBlkRead = reader->ReadSubBlock(0);

    // note: here we read "raw data" from the subblock (i.e. not a "bitmap object"). Per CZI-specification, the bitmap is
    //       stored with "minimal stride/no padding"
    size_t sbBlkDataRead_Size;
    auto sbBlkDataRead = sbBlkRead->GetRawData(ISubBlock::MemBlkType::Data, &sbBlkDataRead_Size);

    {
        size_t sizeOfLine = bitmap->GetWidth() * static_cast<size_t>(Utils::GetBytesPerPixel(bitmap->GetPixelType()));
        EXPECT_EQ(sbBlkDataRead_Size, bitmap->GetHeight() * sizeOfLine) << "Incorrect result";

        ScopedBitmapLockerSP lockBm{ bitmap };
        for (uint32_t y = 0; y < bitmap->GetHeight(); ++y)
        {
            const uint8_t* ptrBitmap1 = static_cast<const uint8_t*>(sbBlkDataRead.get()) + y * sizeOfLine;
            const uint8_t* ptrBitmap2 = static_cast<const uint8_t*>(lockBm.ptrDataRoi) + y * lockBm.stride;
            EXPECT_TRUE(memcmp(ptrBitmap1,ptrBitmap2,sizeOfLine)==0) << "Incorrect result";
        }
    }

    sbBlkRead.reset();	// not needed anymore

    auto mdSegment = reader->ReadMetadataSegment();

    auto metadata = mdSegment->CreateMetaFromMetadataSegment();
    auto xmlRead = metadata->GetXml();

    // TODO: we should add some options to "GetXml" so that we get the data without this header, shouldn't we?
    const char* expectedResult =
        "<?xml version=\"1.0\"?>\n"
        "<ImageDocument>\n"
        " <Metadata>\n"
        "  <Information>\n"
        "   <Image>\n"
        "    <SizeX>16</SizeX>\n"
        "    <SizeY>16</SizeY>\n"
        "    <SizeZ>1</SizeZ>\n"
        "    <SizeC>1</SizeC>\n"
        "    <SizeM>1</SizeM>\n"
        "    <Dimensions>\n"
        "     <Channels>\n"
        "      <Channel Id=\"Channel:0\">\n"
        "       <PixelType>Bgr24</PixelType>\n"
        "      </Channel>\n"
        "     </Channels>\n"
        "    </Dimensions>\n"
        "    <PixelType>Bgr24</PixelType>\n"
        "   </Image>\n"
        "  </Information>\n"
        " </Metadata>\n"
        "</ImageDocument>\n";

    EXPECT_TRUE(strcmp(xmlRead.c_str(), expectedResult) == 0) << "Incorrect result";
}

TEST(CziWriter, Writer5)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    AddSubBlockInfo addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = 0;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = 100;
    addSbBlkInfo.logicalHeight = 10;
    addSbBlkInfo.physicalWidth = 100;
    addSbBlkInfo.physicalHeight = 10;
    addSbBlkInfo.PixelType = PixelType::Gray8;
    addSbBlkInfo.sizeData = 1000;

    std::uint8_t data = 0;
    int cnt = 0;
    addSbBlkInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
    {
        ++data;
        ptr = &data;
        size = 1;
        ++cnt;
        return true;
    };

    writer->SyncAddSubBlock(addSbBlkInfo);

    // we expect that the getData-functor was called 1000 times
    EXPECT_EQ(cnt, 1000) << "Incorrect result";

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);

    auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto sbBlkRead = reader->ReadSubBlock(0);

    size_t sbBlkDataRead_size;
    auto sbBlkDataRead = sbBlkRead->GetRawData(ISubBlock::MemBlkType::Data, &sbBlkDataRead_size);

    EXPECT_EQ(sbBlkDataRead_size, 1000) << "Incorrect result";

    const uint8_t* ptrSbBlkData = static_cast<const uint8_t*>(sbBlkDataRead.get());
    bool isCorrect = true;
    for (int i = 0; i < 1000; ++i)
    {
        if (ptrSbBlkData[i] != (uint8_t)(i + 1))
        {
            isCorrect = false;
            break;
        }
    }

    EXPECT_TRUE(isCorrect) << "Incorrect result";
}

TEST(CziWriter, Writer6)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    AddSubBlockInfo addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = false;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = 100;
    addSbBlkInfo.logicalHeight = 10;
    addSbBlkInfo.physicalWidth = 100;
    addSbBlkInfo.physicalHeight = 10;
    addSbBlkInfo.PixelType = PixelType::Gray8;
    addSbBlkInfo.sizeData = 1000;

    std::uint8_t data = 0;
    int cnt = 0;
    addSbBlkInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
    {
        if (callCnt > 500)
        {
            return false;
        }

        ++data;
        ptr = &data;
        size = 1;
        ++cnt;
        return true;
    };

    writer->SyncAddSubBlock(addSbBlkInfo);

    // we expect that the getData-functor was called 501 times
    EXPECT_EQ(cnt, 501) << "Incorrect result";

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);

    auto reader = CreateCZIReader();
    reader->Open(inputStream);

    auto sbBlkRead = reader->ReadSubBlock(0);

    size_t sbBlkDataRead_size;
    auto sbBlkDataRead = sbBlkRead->GetRawData(ISubBlock::MemBlkType::Data, &sbBlkDataRead_size);

    EXPECT_EQ(sbBlkDataRead_size, 1000) << "Incorrect result";

    const uint8_t* ptrSbBlkData = static_cast<const uint8_t*>(sbBlkDataRead.get());
    bool isCorrect = true;
    for (int i = 0; i < 1000; ++i)
    {
        if (ptrSbBlkData[i] != i > 500 ? 0 : (uint8_t)(i + 1))
        {
            isCorrect = false;
            break;
        }
    }

    EXPECT_TRUE(isCorrect) << "Incorrect result";
}

TEST(CziWriter, Writer7)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } });	// set a bounds for Z and C

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z0C0");
    addSbBlkInfo.mIndexValid = false;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    // coordinate Z0C0 is within bounds, so we expect this to succeed
    writer->SyncAddSubBlock(addSbBlkInfo);

    bool expectedExceptionCaught = false;
    // the coordinate Z10C0 is out-of-bounds, so we expect an exception
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z10C0");
    try
    {
        writer->SyncAddSubBlock(addSbBlkInfo);
    }
    catch (LibCZIWriteException& excp)
    {
        if (excp.GetErrorType() == LibCZIWriteException::ErrorType::SubBlockCoordinateOutOfBounds)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "did not behave as expected";
}

TEST(CziWriter, Writer8)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } });	// set a bounds for Z and C

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z0C0");
    addSbBlkInfo.mIndexValid = false;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    // coordinate Z0C0 is within bounds, so we expect this to succeed
    writer->SyncAddSubBlock(addSbBlkInfo);

    bool expectedExceptionCaught = false;

    // now we try to add a subblock with an "insufficient" coordinate - we do not specify a "C"-coordinate
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z1");
    try
    {
        writer->SyncAddSubBlock(addSbBlkInfo);
    }
    catch (LibCZIWriteException& excp)
    {
        if (excp.GetErrorType() == LibCZIWriteException::ErrorType::SubBlockCoordinateInsufficient)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "did not behave as expected";
}

TEST(CziWriter, Writer9)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
        0, 5);	// set a bounds M : 0<=m<=5

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z0C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    // coordinate Z0C0 is within bounds, so we expect this to succeed
    writer->SyncAddSubBlock(addSbBlkInfo);

    bool expectedExceptionCaught = false;

    // now we try to add a subblock with an "insufficient" coordinate - we do not specify a "C"-coordinate
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z1C0");
    addSbBlkInfo.mIndex = 6;
    try
    {
        writer->SyncAddSubBlock(addSbBlkInfo);
    }
    catch (LibCZIWriteException& excp)
    {
        if (excp.GetErrorType() == LibCZIWriteException::ErrorType::SubBlockCoordinateOutOfBounds)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "did not behave as expected";
}

TEST(CziWriter, Writer10)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
        0, 5);	// set a bounds M : 0<=m<=5

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z0C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    // coordinate Z0C0 is within bounds, so we expect this to succeed
    writer->SyncAddSubBlock(addSbBlkInfo);

    bool expectedExceptionCaught = false;

    // now we try to add a subblock with a "surplus" coordinate - we also specify a T-dimension
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z1C0T0");
    addSbBlkInfo.mIndex = 1;
    try
    {
        writer->SyncAddSubBlock(addSbBlkInfo);
    }
    catch (LibCZIWriteException& excp)
    {
        if (excp.GetErrorType() == LibCZIWriteException::ErrorType::AddCoordinateContainsUnexpectedDimension)
        {
            expectedExceptionCaught = true;
        }
    }

    EXPECT_TRUE(expectedExceptionCaught) << "did not behave as expected";
}

TEST(CziWriter, Writer11)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
        0, 5);	// set a bounds M : 0<=m<=5

                // reserve space for the subblockdirectory large enough to hold a many subblocks as we specified above
    spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 0);

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;

    for (int z = 0; z < 10; ++z)
    {
        for (int c = 0; c < 1; ++c)
        {
            for (int m = 0; m < 5; ++m)
            {
                addSbBlkInfo.Clear();
                addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
                addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
                addSbBlkInfo.mIndexValid = true;
                addSbBlkInfo.mIndex = m;
                addSbBlkInfo.x = 0;
                addSbBlkInfo.y = 0;
                addSbBlkInfo.logicalWidth = bitmap->GetWidth();
                addSbBlkInfo.logicalHeight = bitmap->GetHeight();
                addSbBlkInfo.physicalWidth = bitmap->GetWidth();
                addSbBlkInfo.physicalHeight = bitmap->GetHeight();
                addSbBlkInfo.PixelType = bitmap->GetPixelType();
                addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
                addSbBlkInfo.strideBitmap = lockBm.stride;

                writer->SyncAddSubBlock(addSbBlkInfo);
            }
        }
    }

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);

    bool success = true;
    int subBlkCnt = 0;
    CSegmentWalker::Walk(
        inputStream.get(),
        [&](int cnt, const std::string& id, std::int64_t allocatedSize, std::int64_t usedSize)->bool
        {
            // we expect the CZI-fileheader-segment, then the subblockdirectory-segment, and then exactly 50 subblocks
            if (cnt == 0)
            {
                if (id != "ZISRAWFILE")
                {
                    success = false; return false;
                }

                return true;
            }
            else if (cnt == 1)
            {
                if (id != "ZISRAWDIRECTORY")
                {
                    success = false; return false;
                }

                return true;
            }
            else if (cnt < 2 + 10 * 5)
            {
                if (id != "ZISRAWSUBBLOCK")
                {
                    success = false; return false;
                }

                subBlkCnt++;
                return true;
            }

            success = false;
            return false;
        });

    EXPECT_TRUE(success) << "did not behave as expected";
    EXPECT_TRUE(subBlkCnt == 50) << "did not behave as expected";
}

TEST(CziWriter, Writer12)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
        0, 5);	// set a bounds M : 0<=m<=5

                // reserve size in the subblockdirectory-segment for 10 subblocks, which is too small to hold all 50 subblocks,
                //  so we expect the subblockdirectory-segment at the end of the file
    spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 2);

    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;

    for (int z = 0; z < 10; ++z)
    {
        for (int c = 0; c < 1; ++c)
        {
            for (int m = 0; m < 5; ++m)
            {
                addSbBlkInfo.Clear();
                addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
                addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
                addSbBlkInfo.mIndexValid = true;
                addSbBlkInfo.mIndex = m;
                addSbBlkInfo.x = 0;
                addSbBlkInfo.y = 0;
                addSbBlkInfo.logicalWidth = bitmap->GetWidth();
                addSbBlkInfo.logicalHeight = bitmap->GetHeight();
                addSbBlkInfo.physicalWidth = bitmap->GetWidth();
                addSbBlkInfo.physicalHeight = bitmap->GetHeight();
                addSbBlkInfo.PixelType = bitmap->GetPixelType();
                addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
                addSbBlkInfo.strideBitmap = lockBm.stride;

                writer->SyncAddSubBlock(addSbBlkInfo);
            }
        }
    }

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);

    bool success = true;
    bool allReceived = false;
    int subBlkCnt = 0;
    CSegmentWalker::Walk(
        inputStream.get(),
        [&](int cnt, const std::string& id, std::int64_t allocatedSize, std::int64_t usedSize)->bool
        {
            // we expect the CZI-fileheader-segment, then the subblockdirectory-segment, and then exactly 50 subblocks
            if (cnt == 0)
            {
                if (id != "ZISRAWFILE")
                {
                    success = false; return false;
                }

                return true;
            }
            else if (cnt == 1)
            {
                if (id != "DELETED")
                {
                    success = false; return false;
                }

                return true;
            }
            else if (cnt < 2 + 10 * 5)
            {
                if (id != "ZISRAWSUBBLOCK")
                {
                    success = false; return false;
                }

                subBlkCnt++;
                return true;
            }
            else if (cnt == 2 + 10 * 5)
            {
                if (id != "ZISRAWDIRECTORY")
                {
                    success = false;  return false;
                }

                allReceived = true;
                return true;
            }

            success = false;
            return false;
        });

    EXPECT_TRUE(success && allReceived) << "did not behave as expected";
    EXPECT_EQ(subBlkCnt, 50) << "did not behave as expected";
}

TEST(CziWriter, Writer13)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    size_t sizeCompressedBitmap;
    int widthCompressedBitmap, heightCompressedBitmap;
    auto compressedBitmap = CTestImage::GetJpgXrCompressedImage(&sizeCompressedBitmap, &widthCompressedBitmap, &heightCompressedBitmap);

    AddSubBlockInfoMemPtr addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = widthCompressedBitmap;
    addSbBlkInfo.logicalHeight = heightCompressedBitmap;
    addSbBlkInfo.physicalWidth = widthCompressedBitmap;
    addSbBlkInfo.physicalHeight = heightCompressedBitmap;
    addSbBlkInfo.PixelType = PixelType::Bgr24;
    addSbBlkInfo.SetCompressionMode(CompressionMode::JpgXr);

    addSbBlkInfo.ptrData = compressedBitmap;
    addSbBlkInfo.dataSize = (uint32_t)sizeCompressedBitmap;

    writer->SyncAddSubBlock(addSbBlkInfo);

    auto metadataBuilder = writer->GetPreparedMetadata(PrepareMetadataInfo());
    string xml = metadataBuilder->GetXml(true);
    WriteMetadataInfo writerMdInfo = { 0 };
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();

    writer->SyncWriteMetadata(writerMdInfo);

    writer->Close();

    writer.reset();

    WriteOutTestCzi("CziWriter", "Writer13", outStream->GetDataC(), outStream->GetDataSize());

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    outStream.reset();	// not needed anymore

    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    auto sbBlk = spReader->ReadSubBlock(0);

    auto sbBlkInfo = sbBlk->GetSubBlockInfo();
    EXPECT_TRUE(sbBlkInfo.GetCompressionMode() == CompressionMode::JpgXr) << "Incorrect result";

    auto bitmap = sbBlk->CreateBitmap();
    EXPECT_TRUE(bitmap->GetWidth() == widthCompressedBitmap && bitmap->GetHeight() == heightCompressedBitmap) << "Incorrect result";
    EXPECT_TRUE(bitmap->GetPixelType() == PixelType::Bgr24) << "Incorrect result";
}

TEST(CziWriter, Writer14)
{
    // create a CZI including a pyramid-tile

    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 1024, 1024);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;

    // write 4 tiles, at position (0,0), (1024,0), (0,1024), (1024,1024)
    for (int m = 0; m < 4; ++m)
    {
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = m;
        addSbBlkInfo.x = (m % 2) * bitmap->GetWidth();
        addSbBlkInfo.y = (m / 2) * bitmap->GetHeight();
        addSbBlkInfo.logicalWidth = bitmap->GetWidth();
        addSbBlkInfo.logicalHeight = bitmap->GetHeight();
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lockBm.stride;

        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    // and one pyramid-tile (with zoom=1/2)
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = false;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth() * 2;
    addSbBlkInfo.logicalHeight = bitmap->GetHeight() * 2;
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;

    writer->SyncAddSubBlock(addSbBlkInfo);

    writer->Close();
    writer.reset();

    WriteOutTestCzi("CziWriter", "Writer14", outStream->GetDataC(), outStream->GetDataSize());

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);
    auto pyramidStatistics = spReader->GetPyramidStatistics();

    int cnt = 0;
    for (const auto& i : pyramidStatistics.scenePyramidStatistics)
    {
        cnt++;
        EXPECT_EQ(cnt, 1) << "was expecting only one item";
        EXPECT_EQ(i.first, (numeric_limits<int>::max)()) << "not the expected result (we were expecting an invalid scene-index)";
        EXPECT_EQ(i.second.size(), 2) << "was expecting two pyramid-layer-info-items";
        EXPECT_EQ(i.second[0].count, 4) << "was expecting 4 tiles on layer0";
        EXPECT_EQ(i.second[0].layerInfo.IsLayer0(), true) << "was expecting layer0";
        EXPECT_EQ(i.second[1].count, 1) << "was expecting 1 tile on layer1";
        EXPECT_EQ(i.second[1].layerInfo.minificationFactor, 2) << "was expecting a minification-factor of 2";
        EXPECT_EQ(i.second[1].layerInfo.pyramidLayerNo, 1) << "was expecting to see layer1";
    }

    EXPECT_EQ(cnt, 1) << "was expecting exactly one item";
}

TEST(CziWriter, Writer15)
{
    // create a CZI including a pyramid-tile

    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 1024, 1024);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;

    // write 4 tiles, at position (0,0), (1024,0), (2048,0), (3072,0), (0,1024), (1024,1024)... (3072,3072)
    for (int m = 0; m < 4 * 4; ++m)
    {
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = m;
        addSbBlkInfo.x = (m % 4) * bitmap->GetWidth();
        addSbBlkInfo.y = (m / 4) * bitmap->GetHeight();
        addSbBlkInfo.logicalWidth = bitmap->GetWidth();
        addSbBlkInfo.logicalHeight = bitmap->GetHeight();
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lockBm.stride;

        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    // write 4 tiles (with zoom=1/2), at position (0,0), (2048,0), (0,2048), (2048,2048)
    for (int m = 0; m < 4; ++m)
    {
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
        addSbBlkInfo.mIndexValid = false;
        addSbBlkInfo.mIndex = 0;
        addSbBlkInfo.x = (m % 2) * bitmap->GetWidth() * 2;
        addSbBlkInfo.y = (m / 2) * bitmap->GetHeight() * 2;
        addSbBlkInfo.logicalWidth = bitmap->GetWidth() * 2;
        addSbBlkInfo.logicalHeight = bitmap->GetHeight() * 2;
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lockBm.stride;

        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    // and one pyramid-tile (with zoom=1/4)
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = false;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth() * 4;
    addSbBlkInfo.logicalHeight = bitmap->GetHeight() * 4;
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;

    writer->SyncAddSubBlock(addSbBlkInfo);

    writer->Close();
    writer.reset();

    WriteOutTestCzi("CziWriter", "Writer15", outStream->GetDataC(), outStream->GetDataSize());

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);
    auto pyramidStatistics = spReader->GetPyramidStatistics();

    int cnt = 0;
    for (const auto& i : pyramidStatistics.scenePyramidStatistics)
    {
        cnt++;
        EXPECT_EQ(cnt, 1) << "was expecting only one item";
        EXPECT_EQ(i.first, (numeric_limits<int>::max)()) << "not the expected result (we were expecting an invalid scene-index)";
        EXPECT_EQ(i.second.size(), 3) << "was expecting three pyramid-layer-info-items";
        EXPECT_EQ(i.second[0].count, 16) << "was expecting 4 tiles on layer0";
        EXPECT_EQ(i.second[0].layerInfo.IsLayer0(), true) << "was expecting layer0";
        EXPECT_EQ(i.second[1].count, 4) << "was expecting 4 tiles on layer1";
        EXPECT_EQ(i.second[1].layerInfo.minificationFactor, 2) << "was expecting a minification-factor of 2";
        EXPECT_EQ(i.second[1].layerInfo.pyramidLayerNo, 1) << "was expecting to see layer1";
        EXPECT_EQ(i.second[2].count, 1) << "was expecting 1 tile on layer2";
        EXPECT_EQ(i.second[2].layerInfo.minificationFactor, 2) << "was expecting a minification-factor of 2";
        EXPECT_EQ(i.second[2].layerInfo.pyramidLayerNo, 2) << "was expecting to see layer2";
    }

    EXPECT_EQ(cnt, 1) << "was expecting exactly one item";
}

TEST(CziWriter, WriterReturnFalseFromCallback)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >();
    writer->Create(outStream, spWriterInfo);

    AddSubBlockInfo addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = 100;
    addSbBlkInfo.logicalHeight = 10;
    addSbBlkInfo.physicalWidth = 100;
    addSbBlkInfo.physicalHeight = 10;
    addSbBlkInfo.PixelType = PixelType::Gray8;
    addSbBlkInfo.sizeData = 1000;
    addSbBlkInfo.getData = std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)>(
        [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
        {
            return false;
        }
    );

    writer->SyncAddSubBlock(addSbBlkInfo);
    writer->Close();
    writer.reset();

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);
    auto statistics = spReader->GetStatistics();

    EXPECT_EQ(statistics.subBlockCount, 1) << "Expected one subblock";
    EXPECT_TRUE(statistics.boundingBox.w == 100 && statistics.boundingBox.h == 10 &&
        statistics.boundingBox.x == 0 && statistics.boundingBox.y == 0) << "Expected a bounding-box of (0,0,100,10)";

    auto sbBlk = spReader->ReadSubBlock(0);

    auto sbBlkInfo = sbBlk->GetSubBlockInfo();
    EXPECT_TRUE(sbBlkInfo.GetCompressionMode() == CompressionMode::UnCompressed) << "Incorrect result";
    EXPECT_TRUE(sbBlkInfo.physicalSize.w == 100 && sbBlkInfo.physicalSize.h == 10) << "Expected a subblock-size of (100,10)";

    size_t size;
    auto data = sbBlk->GetRawData(ISubBlock::Data, &size);
    EXPECT_EQ(size, 100 * 10) << "Size was expected to be 1000 bytes";
    bool allZero = true;
    for (const uint8_t* ptr = static_cast<const uint8_t*>(data.get()); ptr < static_cast<const uint8_t*>(data.get()) + size; ++ptr)
    {
        if (*ptr != 0)
        {
            allZero = false;
            break;
        }
    }

    EXPECT_EQ(allZero, true) << "Data was expected to be all zero";
}

TEST(CziWriter, WriterReturnOneByteAndThenFalseFromCallback)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >();
    writer->Create(outStream, spWriterInfo);

    const static uint8_t oneByteOfData = 0xff;

    AddSubBlockInfo addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = 100;
    addSbBlkInfo.logicalHeight = 10;
    addSbBlkInfo.physicalWidth = 100;
    addSbBlkInfo.physicalHeight = 10;
    addSbBlkInfo.PixelType = PixelType::Gray8;
    addSbBlkInfo.sizeData = 1000;
    addSbBlkInfo.getData = std::function<bool(int callCnt, size_t offset, const void*& ptr, size_t& size)>(
        [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
        {
            if (callCnt == 0)
            {
                ptr = &oneByteOfData;
                size = 1;
                return true;
            }

            return false;
        }
    );

    writer->SyncAddSubBlock(addSbBlkInfo);
    writer->Close();
    writer.reset();

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);
    auto statistics = spReader->GetStatistics();

    EXPECT_EQ(statistics.subBlockCount, 1) << "Expected one subblock";
    EXPECT_TRUE(statistics.boundingBox.w == 100 && statistics.boundingBox.h == 10 &&
        statistics.boundingBox.x == 0 && statistics.boundingBox.y == 0) << "Expected a bounding-box of (0,0,100,10)";

    auto sbBlk = spReader->ReadSubBlock(0);

    auto sbBlkInfo = sbBlk->GetSubBlockInfo();
    EXPECT_TRUE(sbBlkInfo.GetCompressionMode() == CompressionMode::UnCompressed) << "Incorrect result";
    EXPECT_TRUE(sbBlkInfo.physicalSize.w == 100 && sbBlkInfo.physicalSize.h == 10) << "Expected a subblock-size of (100,10)";

    size_t size;
    auto data = sbBlk->GetRawData(ISubBlock::Data, &size);
    EXPECT_EQ(size, 100 * 10) << "Size was expected to be 1000 bytes";
    bool allCorrect = true;
    for (const uint8_t* ptr = static_cast<const uint8_t*>(data.get()); ptr < static_cast<const uint8_t*>(data.get()) + size; ++ptr)
    {
        if (ptr == static_cast<const uint8_t*>(data.get()))
        {
            if (*ptr != 0xff)
            {
                allCorrect = false;
                break;
            }
        }
        else if (*ptr != 0)
        {
            allCorrect = false;
            break;
        }
    }

    EXPECT_EQ(allCorrect, true) << "Data was expected to be all zero";
}

TEST(CziWriter, WriterExtraLargeSubblockSegmentTest)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);
    CBitmapOperations::Fill(bitmap.get(), { 1, 1, 1 });

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z1C2T3R4S5I6H7V8B9");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();

    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z10C11T12R13S14I15H16V17B18");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 1;
    CBitmapOperations::Fill(bitmap.get(), { 0.5f, 0.5f, 0.5f });
    writer->SyncAddSubBlock(addSbBlkInfo);

    writer->Close();
    writer.reset();

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);
    auto statistics = spReader->GetStatistics();
    EXPECT_EQ(statistics.IsMIndexValid(), true);
    int startIdx, idxSize;
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::Z, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 1); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::C, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 2); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::T, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 3); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::R, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 4); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::S, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 5); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::I, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 6); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::H, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 7); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::V, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 8); EXPECT_EQ(idxSize, 10);
    EXPECT_TRUE(statistics.dimBounds.TryGetInterval(DimensionIndex::B, &startIdx, &idxSize));
    EXPECT_EQ(startIdx, 9); EXPECT_EQ(idxSize, 10);
    auto sbBlk = spReader->ReadSubBlock(0);
    auto sbBlkInfo = sbBlk->GetSubBlockInfo();
    int coord;
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::Z, &coord));
    EXPECT_EQ(coord, 1);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::C, &coord));
    EXPECT_EQ(coord, 2);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::T, &coord));
    EXPECT_EQ(coord, 3);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::R, &coord));
    EXPECT_EQ(coord, 4);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::S, &coord));
    EXPECT_EQ(coord, 5);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::I, &coord));
    EXPECT_EQ(coord, 6);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::H, &coord));
    EXPECT_EQ(coord, 7);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::V, &coord));
    EXPECT_EQ(coord, 8);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::B, &coord));
    EXPECT_EQ(coord, 9);
    EXPECT_EQ(sbBlkInfo.mIndex, 0);
    const uint8_t* ptr; size_t size;
    sbBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    EXPECT_EQ(size, 4 * 4);
    for (size_t i = 0; i < 4 * 4; ++i)
    {
        EXPECT_EQ(ptr[i], 255);
    }

    sbBlk = spReader->ReadSubBlock(1);
    sbBlkInfo = sbBlk->GetSubBlockInfo();
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::Z, &coord));
    EXPECT_EQ(coord, 10);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::C, &coord));
    EXPECT_EQ(coord, 11);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::T, &coord));
    EXPECT_EQ(coord, 12);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::R, &coord));
    EXPECT_EQ(coord, 13);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::S, &coord));
    EXPECT_EQ(coord, 14);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::I, &coord));
    EXPECT_EQ(coord, 15);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::H, &coord));
    EXPECT_EQ(coord, 16);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::V, &coord));
    EXPECT_EQ(coord, 17);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::B, &coord));
    EXPECT_EQ(coord, 18);
    EXPECT_EQ(sbBlkInfo.mIndex, 1);
    sbBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    EXPECT_EQ(size, 4 * 4);
    for (size_t i = 0; i < 4 * 4; ++i)
    {
        EXPECT_TRUE(ptr[i] == 255 / 2 || ptr[i] == (255 + 1) / 2);
    }
}

TEST(CziWriter, WriterMinimalSubblock)
{
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
    writer->Create(outStream, spWriterInfo);

    auto bitmap = CreateTestBitmap(PixelType::Gray8, 1, 1);
    CBitmapOperations::Fill(bitmap.get(), { 1, 1, 1 });

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("Z1C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();

    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);
    writer->Close();
    writer.reset();

    size_t cziData_size;
    auto cziData = outStream->GetCopy(&cziData_size);
    auto inputStream = CreateStreamFromMemory(cziData, cziData_size);

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    auto sbBlk = spReader->ReadSubBlock(0);
    auto sbBlkInfo = sbBlk->GetSubBlockInfo();
    int coord;
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::Z, &coord));
    EXPECT_EQ(coord, 1);
    EXPECT_TRUE(sbBlkInfo.coordinate.TryGetPosition(DimensionIndex::C, &coord));
    EXPECT_EQ(coord, 0);
    const uint8_t* ptr; size_t size;
    sbBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    EXPECT_EQ(size, 1 * 1);
    EXPECT_EQ(ptr[0], 255);
}

//! Write ZSt0 compressed Gray8 image with no parameters
TEST(CziWriter, WriteCompressedZStd0ImageGray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteCompressedImageZStd0Basic(64, 64, pixelType);
}

//! Write ZSt0 compressed Gray16 image with no parameters
TEST(CziWriter, WriteCompressedZStd0ImageGray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteCompressedImageZStd0Basic(63, 63, pixelType);
}

//! Write ZSt0 compressed Brg24 image with no parameters
TEST(CziWriter, WriteCompressedZStd0ImageBgr24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd0Basic(62, 62, pixelType);
}

//! Write ZSt0 compressed Brg48 image with no parameters
TEST(CziWriter, WriteCompressedZStd0ImageBgr48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteCompressedImageZStd0Basic(61, 61, pixelType);
}

//! Write ZSt0 compressed Gray8 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd0ImageGray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteCompressedImageZStd0Level(64, 64, pixelType, 2);
}

//! Write ZSt0 compressed Gray16 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd0ImageGray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteCompressedImageZStd0Level(63, 63, pixelType, 2);
}

//! Write ZSt0 compressed Brg24 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd0ImageBrg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd0Level(62, 62, pixelType, 2);
}

//! Write ZSt0 compressed Brg48 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd0ImageBrg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteCompressedImageZStd0Level(61, 61, pixelType, 2);
}

//! Write ZSt1 compressed Gray8 image with no parameters
TEST(CziWriter, WriteCompressedZStd1ImageGray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteCompressedImageZStd1Basic(64, 64, pixelType);
}

//! Write ZSt1 compressed Gray16 image with no parameters
TEST(CziWriter, WriteCompressedZStd1ImageGray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteCompressedImageZStd1Basic(63, 63, pixelType);
}

//! Write ZSt1 compressed Brg24 image with no parameters
TEST(CziWriter, WriteCompressedZStd1ImageBgr24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd1Basic(62, 62, pixelType);
}

//! Write ZSt1 compressed Brg48 image with no parameters
TEST(CziWriter, WriteCompressedZStd1ImageBgr48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteCompressedImageZStd1Basic(61, 61, pixelType);
}

//! Write ZSt1 compressed Gray8 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd1ImageGray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteCompressedImageZStd1Level(64, 64, pixelType, 2);
}

//! Write ZSt1 compressed Gray16 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd1ImageGray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteCompressedImageZStd1Level(63, 63, pixelType, 2);
}

//! Write ZSt1 compressed Brg24 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd1ImageBrg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd1Level(62, 62, pixelType, 2);
}

//! Write ZSt1 compressed Brg48 image with compression level 2 parameter
TEST(CziWriter, WriteCompressedZStd1ImageBrg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteCompressedImageZStd1Level(61, 61, pixelType, 2);
}

//! Write ZSt1 compressed Gray8 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteCompressedZStd1ImageGray8LowPacking)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteCompressedImageZStd1LowPacking(64, 64, pixelType, 2, true);
}

//! Write ZSt1 compressed Gray16 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteCompressedZStd1ImageGray16LowPacking)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteCompressedImageZStd1LowPacking(63, 63, pixelType, 2, true);
}

//! Write ZSt1 compressed Bgr24 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteCompressedZStd1ImageBrg24LowPacking)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd1LowPacking(62, 62, pixelType, 2, true);
}

//! Write ZSt1 compressed Bgr48 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteCompressedZStd1ImageBrg48LowPacking)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteCompressedImageZStd1LowPacking(61, 61, pixelType, 2, true);
}

//! Write and read ZSt0 compressed Gray8 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd0ImageGray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteReadCompressedImageZStd0Basic(64, 64, pixelType);
}

//! Write and read ZSt0 compressed Gray16 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd0ImageGray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteReadCompressedImageZStd0Basic(63, 63, pixelType);
}

//! Write and read ZSt0 compressed Brg24 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd0ImageBgr24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteCompressedImageZStd0Basic(62, 62, pixelType);
}

//! Write and read ZSt0 compressed Brg48 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd0ImageBgr48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteReadCompressedImageZStd0Basic(61, 61, pixelType);
}

//! Write and read ZSt0 compressed Gray8 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd0ImageGray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteReadCompressedImageZStd0Level(64, 64, pixelType, 2);
}

//! Write and read ZSt0 compressed Gray16 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd0ImageGray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteReadCompressedImageZStd0Level(63, 63, pixelType, 2);
}

//! Write and read ZSt0 compressed Brg24 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd0ImageBrg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteReadCompressedImageZStd0Level(62, 62, pixelType, 2);
}

//! Write and read ZSt0 compressed Brg48 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd0ImageBrg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteReadCompressedImageZStd0Level(61, 61, pixelType, 2);
}

//! Write and read ZSt1 compressed Gray8 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd1ImageGray8Basic)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteReadCompressedImageZStd1Basic(64, 64, pixelType);
}

//! Write and read ZSt1 compressed Gray16 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd1ImageGray16Basic)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteReadCompressedImageZStd1Basic(63, 63, pixelType);
}

//! Write and read ZSt1 compressed Brg24 image with no parameters
TEST(CziWriter, WriteReadCompressedZStd1ImageBgr24Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteReadCompressedImageZStd1Basic(62, 62, pixelType);
}

//! Write and read ZSt1 compressed Brg48 image with no parameters
TEST(CziWriter, WriteReadReadCompressedZStd1ImageBgr48Basic)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteReadCompressedImageZStd1Basic(61, 61, pixelType);
}

//! Write and read ZSt1 compressed Gray8 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageGray8Level2)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteReadCompressedImageZStd1Level(64, 64, pixelType, 2);
}

//! Write and read ZSt1 compressed Gray16 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageGray16Level2)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteReadCompressedImageZStd1Level(63, 63, pixelType, 2);
}

//! Write and read ZSt1 compressed Brg24 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageBrg24Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteReadCompressedImageZStd1Level(62, 62, pixelType, 2);
}

//! Write and read ZSt1 compressed Brg48 image with compression level 2 parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageBrg48Level2)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteReadCompressedImageZStd1Level(61, 61, pixelType, 2);
}

//! Write and read ZSt1 compressed Gray8 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageGray8LowPacking)
{
    constexpr PixelType pixelType = PixelType::Gray8;
    _testWriteReadCompressedImageZStd1LowPacking(64, 64, pixelType, 2, true);
}

//! Write and read ZSt1 compressed Gray16 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageGray16LowPacking)
{
    constexpr PixelType pixelType = PixelType::Gray16;
    _testWriteReadCompressedImageZStd1LowPacking(63, 63, pixelType, 2, true);
}

//! Write and read ZSt1 compressed Bgr24 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageBrg24LowPacking)
{
    constexpr PixelType pixelType = PixelType::Bgr24;
    _testWriteReadCompressedImageZStd1LowPacking(62, 62, pixelType, 2, true);
}

//! Write and read ZSt1 compressed Bgr48 image with compression level 2 
//! and low-high byte packing enable flag parameter
TEST(CziWriter, WriteReadCompressedZStd1ImageBrg48LowPacking)
{
    constexpr PixelType pixelType = PixelType::Bgr48;
    _testWriteReadCompressedImageZStd1LowPacking(61, 61, pixelType, 2, true);
}
