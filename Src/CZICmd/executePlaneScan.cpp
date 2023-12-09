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
        const auto reader = CExecuteBase::CreateAndOpenCziReader(options);
        const auto accessor = reader->CreateSingleChannelScalingTileAccessor();

        const auto roi = CExecuteBase::GetRoiFromOptions(options, reader->GetStatistics());
        const auto& coordinate = options.GetPlaneCoordinate();

        CacheContext cache_context;
        const uint64_t max_cache_size = options.GetSubBlockCacheSize();
        if (max_cache_size > 0)
        {
            cache_context.cache = CreateSubBlockCache();
            cache_context.prune_options.maxMemoryUsage = max_cache_size;
        }
        const auto tile_size_for_plane_scan = options.GetTileSizeForPlaneScan();
        const IntSize tileSize = { get<0>(tile_size_for_plane_scan), get<1>(tile_size_for_plane_scan) };
        const auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);

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

                CExecutePlaneScan::WriteRoi(accessor, coordinate, tileRect, cache_context, saver, options);
            }
        }

        return true;
    }
protected:
    static void WriteRoi(
        const shared_ptr<ISingleChannelScalingTileAccessor>& accessor,
        const CDimCoordinate& plane_coordinate,
        const IntRect& roi,
        const CacheContext& cache_context,
        const shared_ptr<ISaveBitmap>& saver,
        const CCmdLineOptions& options)
    {
        libCZI::ISingleChannelScalingTileAccessor::Options scstaOptions;
        scstaOptions.Clear();
        scstaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        scstaOptions.sceneFilter = options.GetSceneIndexSet();
        scstaOptions.subBlockCache = cache_context.cache;
        scstaOptions.useVisibilityCheckOptimization = options.GetUseVisibilityCheckOptimization();

        const auto bitmap = accessor->Get(roi, &plane_coordinate, options.GetZoom(), &scstaOptions);

        if (cache_context.cache)
        {
            cache_context.cache->Prune(cache_context.prune_options);
        }

        const auto filename = GetFileName(options, roi);
        saver->Save(filename.c_str(), SaveDataFormat::PNG, bitmap.get());
    }

    static wstring GetFileName(const CCmdLineOptions& options, const IntRect& roi)
    {
        wstringstream string_stream;
        string_stream << "_X" << roi.x << "_Y" << roi.y << "_W" << roi.w << "_H" << roi.h;
        wstring output_filename = options.MakeOutputFilename(string_stream.str().c_str(), L"PNG");
        return output_filename;
    }
};

bool executePlaneScan(const CCmdLineOptions& options)
{
    return CExecutePlaneScan::execute(options);
}
