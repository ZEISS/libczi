// SPDX-FileCopyrightText: 2017-2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "executeReEncodeCzi.h"
#include "executeBase.h"
#include "inc_libCZI.h"
#include "libCZI_compress.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

using namespace libCZI;
using namespace std;

namespace
{
    shared_ptr<IMemoryBlock> EncodeTile(
        CompressionMode target,
        PixelType pixelType,
        uint32_t width,
        uint32_t height,
        uint32_t stride,
        const void* pixels,
        const ICompressParameters* compressParams)
    {
        switch (target)
        {
        case CompressionMode::Zstd0:
            return ZstdCompress::CompressZStd0Alloc(width, height, stride, pixelType, pixels, compressParams);
        case CompressionMode::Zstd1:
            return ZstdCompress::CompressZStd1Alloc(width, height, stride, pixelType, pixels, compressParams);
        case CompressionMode::JpgXr:
            return JxrLibCompress::Compress(pixelType, width, height, stride, pixels, compressParams);
#if LIBCZI_HAVE_LIBJXL
        case CompressionMode::Jxl:
            return JxlLibCompress::Compress(pixelType, width, height, stride, pixels, compressParams);
#else
        case CompressionMode::Jxl:
            throw runtime_error("ReEncodeCZI: JPEG XL output requires LIBCZI_BUILD_WITH_LIBJXL=ON.");
#endif
        default:
            break;
        }

        stringstream ss;
        ss << "ReEncodeCZI: unsupported target compression \""
           << Utils::CompressionModeToInformalString(target)
           << "\" (supported: zstd0, zstd1, jpgxr, jxl";
#if !LIBCZI_HAVE_LIBJXL
        ss << " (jxl not built)";
#endif
        ss << ", uncompressed).";
        throw runtime_error(ss.str());
    }

    string FormatLocalTime(const chrono::system_clock::time_point tp)
    {
        const time_t tt = chrono::system_clock::to_time_t(tp);
        tm tmBuf{};
#if defined(_MSC_VER)
        if (localtime_s(&tmBuf, &tt) != 0)
        {
            return "(invalid time)";
        }
#elif defined(_WIN32)
        const tm* p = localtime(&tt);
        if (p == nullptr)
        {
            return "(invalid time)";
        }

        tmBuf = *p;
#else
        if (localtime_r(&tt, &tmBuf) == nullptr)
        {
            return "(invalid time)";
        }
#endif
        stringstream ss;
        ss << put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    string FormatElapsedSeconds(chrono::steady_clock::duration d)
    {
        const double secs = chrono::duration<double>(d).count();
        stringstream ss;
        ss << fixed << setprecision(2) << secs << " s";
        return ss.str();
    }

    bool TryGetFileSizeBytes(const wstring& path, uint64_t* outSize)
    {
        if (outSize == nullptr)
        {
            return false;
        }

#if defined(_WIN32)
        struct __stat64 st{};
        if (_wstat64(path.c_str(), &st) != 0)
        {
            return false;
        }

        *outSize = static_cast<uint64_t>(st.st_size);
        return true;
#else
        struct stat st{};
        if (stat(convertToUtf8(path).c_str(), &st) != 0)
        {
            return false;
        }

        *outSize = static_cast<uint64_t>(st.st_size);
        return true;
#endif
    }

    void WriteReencodeProgressBar(ILog* log, int completedSteps, int totalSteps, const char* label)
    {
        if (log == nullptr || totalSteps <= 0)
        {
            return;
        }

        const int pct = min(100, max(0, (completedSteps * 100) / totalSteps));
        constexpr int barWidth = 40;
        const int filled = min(barWidth, max(0, (completedSteps * barWidth) / totalSteps));

        stringstream ss;
        ss << "\r" << label << " [";
        for (int i = 0; i < barWidth; ++i)
        {
            ss << (i < filled ? '#' : '-');
        }

        ss << "] " << completedSteps << "/" << totalSteps << " (" << pct << "%)";
        log->WriteStdOut(ss.str());
        cout.flush();
    }
} // namespace

class CExecuteReEncodeCzi : CExecuteBase
{
    struct PreparedReencodeSubblock
    {
        bool valid = false;
        bool isUncompressed = false;
        SubBlockInfo sbi{};
        vector<uint8_t> metaCopy;
        vector<uint8_t> attCopy;
        shared_ptr<IMemoryBlock> compressed;
        vector<uint8_t> pixelCopy;
        uint32_t pixelStrideBytes = 0;
    };

    static PreparedReencodeSubblock PrepareSubblockForReencode(
        const shared_ptr<ICZIReader>& reader,
        int index,
        CompressionMode targetCompression,
        const ICompressParameters* compressParams);

    static void ReencodeSubblockWorker(
        const CCmdLineOptions* options,
        CompressionMode targetCompression,
        const ICompressParameters* compressParams,
        int subBlockCount,
        atomic<int>* nextIndex,
        atomic<bool>* cancelFlag,
        vector<PreparedReencodeSubblock>* preparedOut,
        mutex* slotMutex,
        mutex* errorMutex,
        exception_ptr* firstError,
        shared_ptr<ILog> progressLog,
        atomic<int>* progressDone,
        mutex* progressMx,
        int progressTotalSteps);

public:
    static bool execute(const CCmdLineOptions& options);
};

/*static*/ CExecuteReEncodeCzi::PreparedReencodeSubblock CExecuteReEncodeCzi::PrepareSubblockForReencode(
    const shared_ptr<ICZIReader>& reader,
    int index,
    CompressionMode targetCompression,
    const ICompressParameters* compressParams)
{
    const auto sb = reader->ReadSubBlock(index);
    if (sb == nullptr)
    {
        stringstream ss;
        ss << "ReEncodeCZI: ReadSubBlock(" << index << ") failed.";
        throw runtime_error(ss.str());
    }

    PreparedReencodeSubblock out;
    out.sbi = sb->GetSubBlockInfo();

    size_t metaSize = 0;
    const auto metaBlob = sb->GetRawData(ISubBlock::MemBlkType::Metadata, &metaSize);
    if (metaSize > 0 && metaBlob != nullptr)
    {
        const auto* p = static_cast<const uint8_t*>(metaBlob.get());
        out.metaCopy.assign(p, p + metaSize);
    }

    size_t attSize = 0;
    const auto attBlob = sb->GetRawData(ISubBlock::MemBlkType::Attachment, &attSize);
    if (attSize > 0 && attBlob != nullptr)
    {
        const auto* p = static_cast<const uint8_t*>(attBlob.get());
        out.attCopy.assign(p, p + attSize);
    }

    const shared_ptr<IBitmapData> bitmap = sb->CreateBitmap(nullptr);
    const ScopedBitmapLockerSP locker(bitmap);

    if (targetCompression == CompressionMode::UnCompressed)
    {
        const uint32_t w = bitmap->GetWidth();
        const uint32_t h = bitmap->GetHeight();
        const PixelType pt = out.sbi.pixelType;
        const size_t bpp = Utils::GetBytesPerPixel(pt);
        const size_t rowBytes = static_cast<size_t>(w) * bpp;
        if (locker.stride < rowBytes)
        {
            throw runtime_error("ReEncodeCZI: decoded bitmap stride too small for uncompressed copy.");
        }

        out.pixelCopy.resize(rowBytes * static_cast<size_t>(h));
        const auto* srcBase = static_cast<const uint8_t*>(locker.ptrDataRoi);
        for (uint32_t y = 0; y < h; ++y)
        {
            memcpy(
                out.pixelCopy.data() + static_cast<size_t>(y) * rowBytes,
                srcBase + static_cast<size_t>(y) * locker.stride,
                rowBytes);
        }

        out.pixelStrideBytes = static_cast<uint32_t>(rowBytes);
        out.isUncompressed = true;
    }
    else
    {
        out.compressed = EncodeTile(
            targetCompression,
            out.sbi.pixelType,
            bitmap->GetWidth(),
            bitmap->GetHeight(),
            locker.stride,
            locker.ptrDataRoi,
            compressParams);
    }

    out.valid = true;
    return out;
}

/*static*/ void CExecuteReEncodeCzi::ReencodeSubblockWorker(
    const CCmdLineOptions* options,
    CompressionMode targetCompression,
    const ICompressParameters* compressParams,
    int subBlockCount,
    atomic<int>* nextIndex,
    atomic<bool>* cancelFlag,
    vector<PreparedReencodeSubblock>* preparedOut,
    mutex* slotMutex,
    mutex* errorMutex,
    exception_ptr* firstError,
    shared_ptr<ILog> progressLog,
    atomic<int>* progressDone,
    mutex* progressMx,
    int progressTotalSteps)
{
    shared_ptr<ICZIReader> reader = CreateAndOpenCziReader(*options);

    for (;;)
    {
        if (cancelFlag->load(memory_order_relaxed))
        {
            break;
        }

        const int index = nextIndex->fetch_add(1, memory_order_relaxed);
        if (index >= subBlockCount)
        {
            break;
        }

        try
        {
            PreparedReencodeSubblock prepared = PrepareSubblockForReencode(reader, index, targetCompression, compressParams);
            {
                lock_guard<mutex> lock(*slotMutex);
                (*preparedOut)[static_cast<size_t>(index)] = move(prepared);
            }

            const int step = progressDone->fetch_add(1, memory_order_relaxed) + 1;
            {
                lock_guard<mutex> pg(*progressMx);
                WriteReencodeProgressBar(progressLog.get(), step, progressTotalSteps, "ReEncodeCZI");
            }
        }
        catch (...)
        {
            lock_guard<mutex> el(*errorMutex);
            if (!firstError)
            {
                *firstError = current_exception();
            }

            cancelFlag->store(true, memory_order_relaxed);
            break;
        }
    }
}

bool CExecuteReEncodeCzi::execute(const CCmdLineOptions& options)
{
    const CompressionMode targetCompression = options.GetCompressionMode();
    if (targetCompression == CompressionMode::Invalid)
    {
        throw invalid_argument("ReEncodeCZI requires --compressionopts (e.g. zstd1:ExplicitLevel=3 or jxl:).");
    }

    auto reader = CreateAndOpenCziReader(options);
    const FileHeaderInfo hdr = reader->GetFileHeaderInfo();
    SubBlockStatistics stats = reader->GetStatistics();
    if (stats.subBlockCount <= 0)
    {
        throw runtime_error("ReEncodeCZI: source has no subblocks.");
    }

    const wstring outPath = options.MakeOutputFilename(nullptr, L"czi");
    auto outStream = CreateOutputStreamForFile(outPath.c_str(), true);
    auto writer = CreateCZIWriter();

    auto writerInfo = make_shared<CCziWriterInfo>(hdr.fileGuid);
    if (!stats.dimBounds.IsEmpty())
    {
        writerInfo->SetDimBounds(&stats.dimBounds);
    }

    if (stats.IsMIndexValid())
    {
        writerInfo->SetMIndexBounds(stats.minMindex, stats.maxMindex);
    }

    writerInfo->SetReservedSizeForSubBlockDirectory(true, static_cast<size_t>(max(1, stats.subBlockCount)));

    int attachmentCount = 0;
    reader->EnumerateAttachments(
        [&](int, const AttachmentInfo&)->bool
        {
            ++attachmentCount;
            return true;
        });
    writerInfo->SetReservedSizeForAttachmentsDirectory(true, static_cast<size_t>(max(1, attachmentCount)));

    writer->Create(outStream, writerInfo);

    const ICompressParameters* compressParams = options.GetCompressionParameters().get();

    const int subBlockCount = stats.subBlockCount;
    vector<PreparedReencodeSubblock> prepared(static_cast<size_t>(subBlockCount));

    const int progressTotalSteps = max(1, subBlockCount * 2);
    atomic<int> progressDone{ 0 };
    mutex progressMx;
    const shared_ptr<ILog> log = options.GetLog();
    const auto wallStartSubblocks = chrono::system_clock::now();
    const auto steadyStartSubblocks = chrono::steady_clock::now();
    {
        stringstream ss;
        ss << "ReEncodeCZI: subblock recompression started at " << FormatLocalTime(wallStartSubblocks);
        log->WriteLineStdOut(ss.str());
    }

    unsigned workerCount = thread::hardware_concurrency();
    if (workerCount == 0)
    {
        workerCount = 1;
    }

    if (static_cast<int>(workerCount) > subBlockCount)
    {
        workerCount = static_cast<unsigned>(subBlockCount);
    }

    if (workerCount > 1U)
    {
        wstringstream parMsg;
        parMsg << L"ReEncodeCZI: preparing subblocks with " << workerCount << L" threads.";
        options.GetLog()->WriteLineStdOut(parMsg.str().c_str());
    }

    exception_ptr firstError;

    try
    {
        if (workerCount <= 1U)
        {
            for (int i = 0; i < subBlockCount; ++i)
            {
                prepared[static_cast<size_t>(i)] = PrepareSubblockForReencode(reader, i, targetCompression, compressParams);
                const int step = progressDone.fetch_add(1, memory_order_relaxed) + 1;
                {
                    lock_guard<mutex> pg(progressMx);
                    WriteReencodeProgressBar(log.get(), step, progressTotalSteps, "ReEncodeCZI");
                }
            }
        }
        else
        {
            atomic<int> nextIndex{ 0 };
            atomic<bool> cancelFlag{ false };
            mutex slotMutex;
            mutex errorMutex;

            vector<thread> workers;
            workers.reserve(workerCount);
            for (unsigned t = 0; t < workerCount; ++t)
            {
                workers.emplace_back(
                    ReencodeSubblockWorker,
                    &options,
                    targetCompression,
                    compressParams,
                    subBlockCount,
                    &nextIndex,
                    &cancelFlag,
                    &prepared,
                    &slotMutex,
                    &errorMutex,
                    &firstError,
                    log,
                    &progressDone,
                    &progressMx,
                    progressTotalSteps);
            }

            for (thread& w : workers)
            {
                w.join();
            }

            if (firstError)
            {
                log->WriteStdOut("\n");
                cout.flush();
                rethrow_exception(firstError);
            }
        }

        for (int i = 0; i < subBlockCount; ++i)
        {
            PreparedReencodeSubblock& p = prepared[static_cast<size_t>(i)];
            if (!p.valid)
            {
                throw runtime_error("ReEncodeCZI: internal error, missing prepared subblock.");
            }

            if (p.isUncompressed)
            {
                AddSubBlockInfoStridedBitmap addStrided;
                addStrided.Clear();
                addStrided.coordinate = p.sbi.coordinate;
                addStrided.mIndexValid = p.sbi.IsMindexValid();
                addStrided.mIndex = p.sbi.mIndex;
                addStrided.x = p.sbi.logicalRect.x;
                addStrided.y = p.sbi.logicalRect.y;
                addStrided.logicalWidth = p.sbi.logicalRect.w;
                addStrided.logicalHeight = p.sbi.logicalRect.h;
                addStrided.physicalWidth = p.sbi.physicalSize.w;
                addStrided.physicalHeight = p.sbi.physicalSize.h;
                addStrided.PixelType = p.sbi.pixelType;
                addStrided.pyramid_type = p.sbi.pyramidType;
                addStrided.ptrBitmap = p.pixelCopy.data();
                addStrided.strideBitmap = p.pixelStrideBytes;
                addStrided.ptrSbBlkMetadata = p.metaCopy.empty() ? nullptr : p.metaCopy.data();
                addStrided.sbBlkMetadataSize = static_cast<uint32_t>(p.metaCopy.size());
                addStrided.ptrSbBlkAttachment = p.attCopy.empty() ? nullptr : p.attCopy.data();
                addStrided.sbBlkAttachmentSize = static_cast<uint32_t>(p.attCopy.size());
                writer->SyncAddSubBlock(addStrided);
            }
            else
            {
                AddSubBlockInfoMemPtr addMem;
                addMem.Clear();
                addMem.coordinate = p.sbi.coordinate;
                addMem.mIndexValid = p.sbi.IsMindexValid();
                addMem.mIndex = p.sbi.mIndex;
                addMem.x = p.sbi.logicalRect.x;
                addMem.y = p.sbi.logicalRect.y;
                addMem.logicalWidth = p.sbi.logicalRect.w;
                addMem.logicalHeight = p.sbi.logicalRect.h;
                addMem.physicalWidth = p.sbi.physicalSize.w;
                addMem.physicalHeight = p.sbi.physicalSize.h;
                addMem.PixelType = p.sbi.pixelType;
                addMem.pyramid_type = p.sbi.pyramidType;
                addMem.SetCompressionMode(targetCompression);
                addMem.ptrData = p.compressed->GetPtr();
                addMem.dataSize = static_cast<uint32_t>(p.compressed->GetSizeOfData());
                addMem.ptrSbBlkMetadata = p.metaCopy.empty() ? nullptr : p.metaCopy.data();
                addMem.sbBlkMetadataSize = static_cast<uint32_t>(p.metaCopy.size());
                addMem.ptrSbBlkAttachment = p.attCopy.empty() ? nullptr : p.attCopy.data();
                addMem.sbBlkAttachmentSize = static_cast<uint32_t>(p.attCopy.size());
                writer->SyncAddSubBlock(addMem);
            }

            p = {};

            const int step = progressDone.fetch_add(1, memory_order_relaxed) + 1;
            {
                lock_guard<mutex> pg(progressMx);
                WriteReencodeProgressBar(log.get(), step, progressTotalSteps, "ReEncodeCZI");
            }
        }
    }
    catch (...)
    {
        log->WriteStdOut("\n");
        cout.flush();
        throw;
    }

    {
        lock_guard<mutex> pg(progressMx);
        WriteReencodeProgressBar(log.get(), progressTotalSteps, progressTotalSteps, "ReEncodeCZI");
    }

    log->WriteStdOut("\n");
    cout.flush();

    const auto steadyEndSubblocks = chrono::steady_clock::now();
    const auto wallEndSubblocks = chrono::system_clock::now();
    {
        stringstream ss;
        ss << "ReEncodeCZI: subblock recompression finished at " << FormatLocalTime(wallEndSubblocks)
           << " (elapsed " << FormatElapsedSeconds(steadyEndSubblocks - steadyStartSubblocks) << ").";
        log->WriteLineStdOut(ss.str());
    }

    reader->EnumerateAttachments(
        [&](int index, const AttachmentInfo&)->bool
        {
            auto att = reader->ReadAttachment(index);
            if (att == nullptr)
            {
                stringstream ss;
                ss << "ReEncodeCZI: ReadAttachment(" << index << ") failed.";
                throw runtime_error(ss.str());
            }

            size_t dataSize = 0;
            const auto blob = att->GetRawData(&dataSize);
            vector<uint8_t> dataCopy;
            if (dataSize > 0 && blob != nullptr)
            {
                const auto* p = static_cast<const uint8_t*>(blob.get());
                dataCopy.assign(p, p + dataSize);
            }

            const AttachmentInfo& ai = att->GetAttachmentInfo();
            AddAttachmentInfo addAtt;
            addAtt.Clear();
            addAtt.contentGuid = ai.contentGuid;
            addAtt.SetContentFileType(ai.contentFileType);
            addAtt.SetName(ai.name.c_str());
            addAtt.ptrData = dataCopy.empty() ? nullptr : dataCopy.data();
            addAtt.dataSize = static_cast<uint32_t>(dataCopy.size());
            writer->SyncAddAttachment(addAtt);
            return true;
        });

    shared_ptr<IMetadataSegment> mdSeg = reader->ReadMetadataSegment();
    if (mdSeg == nullptr)
    {
        throw runtime_error("ReEncodeCZI: source has no metadata segment.");
    }

    size_t xmlSize = 0;
    const shared_ptr<const void> xmlBlob = mdSeg->GetRawData(IMetadataSegment::XmlMetadata, &xmlSize);
    size_t mdAttSize = 0;
    const shared_ptr<const void> mdAttBlob = mdSeg->GetRawData(IMetadataSegment::Attachment, &mdAttSize);

    string mdXmlStr;
    if (xmlSize > 0 && xmlBlob != nullptr)
    {
        mdXmlStr.assign(static_cast<const char*>(xmlBlob.get()), xmlSize);
    }

    vector<uint8_t> mdSegAttStorage;
    if (mdAttSize > 0 && mdAttBlob != nullptr)
    {
        const auto* p = static_cast<const uint8_t*>(mdAttBlob.get());
        mdSegAttStorage.assign(p, p + mdAttSize);
    }

    reader->Close();
    reader.reset();

    WriteMetadataInfo wmi;
    wmi.Clear();
    wmi.szMetadata = mdXmlStr.empty() ? "" : mdXmlStr.c_str();
    wmi.szMetadataSize = mdXmlStr.size();
    wmi.ptrAttachment = mdSegAttStorage.empty() ? nullptr : mdSegAttStorage.data();
    wmi.attachmentSize = mdSegAttStorage.size();
    writer->SyncWriteMetadata(wmi);

    writer->Close();

    const auto steadyEndTotal = chrono::steady_clock::now();
    const auto wallEndTotal = chrono::system_clock::now();

    wstringstream msg;
    msg << L"ReEncodeCZI wrote \"" << outPath << L"\" (" << stats.subBlockCount << L" subblocks, "
        << attachmentCount << L" attachments).";
    options.GetLog()->WriteLineStdOut(msg.str().c_str());

    {
        stringstream ss;
        ss << "ReEncodeCZI: full output completed at " << FormatLocalTime(wallEndTotal)
           << " (total elapsed including attachments and metadata: "
           << FormatElapsedSeconds(steadyEndTotal - steadyStartSubblocks) << ").";
        options.GetLog()->WriteLineStdOut(ss.str());
    }

    {
        uint64_t inBytes = 0;
        uint64_t outBytes = 0;
        if (!TryGetFileSizeBytes(options.GetCZIFilename(), &inBytes) || !TryGetFileSizeBytes(outPath, &outBytes))
        {
            stringstream ss;
            ss << "ReEncodeCZI: could not read source or output file size for comparison.";
            options.GetLog()->WriteLineStdOut(ss.str());
        }
        else
        {
            stringstream ss;
            ss << fixed << setprecision(2);
            ss << "ReEncodeCZI: file size " << inBytes << " -> " << outBytes << " bytes";
            if (inBytes > 0)
            {
                const double pct = 100.0 * (static_cast<double>(outBytes) - static_cast<double>(inBytes))
                    / static_cast<double>(inBytes);
                ss << " (" << (pct >= 0 ? "+" : "") << pct << "% vs source).";
            }
            else
            {
                ss << " (source size 0; percentage N/A).";
            }

            options.GetLog()->WriteLineStdOut(ss.str());
        }
    }

    return true;
}

bool executeReEncodeCzi(const CCmdLineOptions& options)
{
    return CExecuteReEncodeCzi::execute(options);
}
