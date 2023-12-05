// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "executePlaneScan.h"
#include "executeBase.h"

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
        auto reader = CreateAndOpenCziReader(options);
        const auto subblock_statistics = reader->GetStatistics();
        auto accessor = reader->CreateSingleChannelScalingTileAccessor();

        auto roi = GetRoiFromOptions(options, subblock_statistics);
        CDimCoordinate coordinate = options.GetPlaneCoordinate();


        CacheContext cache_context;
        cache_context.cache = CreateSubBlockCache();
        cache_context.prune_options.maxMemoryUsage = 40 * 1024 * 1024;

        IntSize tileSize = { 512, 512 };

        for (int y = roi.y; y < roi.y + roi.h; y += tileSize.h)
        {
            for (int x = roi.x; x < roi.x + roi.w; x += tileSize.w)
            {
                IntRect tileRect = { x, y, (int)tileSize.w, (int)tileSize.h };
                WriteRoi(accessor, coordinate, tileRect, cache_context, options);
            }
        }

        return true;
        //for (int x = 0; x < statistics.boundingBoxLayer0Only.w / size_x; ++x)
    //auto re = accessor->Get(roi, &coordinate, options.GetZoom(), &scstaOptions);
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
    }
};

bool executePlaneScan(const CCmdLineOptions& options)
{
    return CExecutePlaneScan::execute(options);
}
