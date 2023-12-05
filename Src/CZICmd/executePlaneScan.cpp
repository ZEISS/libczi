// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "executePlaneScan.h"
#include "executeBase.h"
#include "SaveBitmap.h"

using namespace std;
using namespace libCZI;

class CExecutePlaneScan : public CExecuteBase
{
protected:
    struct CacheContext
    {
        shared_ptr<ISubBlockCache> cache;
        ISubBlockCache::PruneOptions prune_options;
    };
public:
    static bool execute(const CCmdLineOptions& options)
    {
        const auto reader = CreateAndOpenCziReader(options);
        const auto subblock_statistics = reader->GetStatistics();
        const auto accessor = reader->CreateSingleChannelScalingTileAccessor();

        auto roi = GetRoiFromOptions(options, subblock_statistics);
        CDimCoordinate coordinate = options.GetPlaneCoordinate();


        CacheContext cache_context;
        cache_context.cache = CreateSubBlockCache();
        cache_context.prune_options.maxMemoryUsage = 40 * 1024 * 1024;

        IntSize tileSize = { 512, 512 };

        for (int y = 0; y < (roi.h + static_cast<int>(tileSize.h) - 1) / static_cast<int>(tileSize.h); ++y)
        {
            for (int x = 0; x < (roi.w + static_cast<int>(tileSize.w) - 1) / static_cast<int>(tileSize.w); ++x)
            {
                IntRect tileRect =
                {
                    roi.x + x * static_cast<int>(tileSize.w),
                    roi.y + y * static_cast<int>(tileSize.h),
                    min(static_cast<int>(tileSize.w), roi.w - x * static_cast<int>(tileSize.w)),
                    min(static_cast<int>(tileSize.h), roi.h - y * static_cast<int>(tileSize.h))
                };

                WriteRoi(accessor, coordinate, tileRect, cache_context, options);
            }
        }

        return true;
    }
protected:
    static void WriteRoi(const shared_ptr<ISingleChannelScalingTileAccessor>& accessor, const CDimCoordinate& coordinate, const IntRect& roi, const CacheContext& cache_context, const CCmdLineOptions& options)
    {
        libCZI::ISingleChannelScalingTileAccessor::Options scstaOptions; scstaOptions.Clear();
        scstaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        scstaOptions.sceneFilter = options.GetSceneIndexSet();
        scstaOptions.subBlockCache = cache_context.cache;

        auto bitmap = accessor->Get(roi, &coordinate, options.GetZoom(), &scstaOptions);

        cache_context.cache->Prune(cache_context.prune_options);

        auto filename = GetFileName(options, roi);
        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(filename.c_str(), SaveDataFormat::PNG, bitmap.get());
    }

    static wstring GetFileName(const CCmdLineOptions& options, const IntRect& roi)
    {
        wstringstream string_stream;
        string_stream << "_X" << roi.x << "_Y" << roi.y << "_W" << roi.w << "_H" << roi.h;
        wstring outputfilename = options.MakeOutputFilename(string_stream.str().c_str(), L"PNG");
        return outputfilename;
    }
};

bool executePlaneScan(const CCmdLineOptions& options)
{
    return CExecutePlaneScan::execute(options);
}
