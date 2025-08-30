#include "libCZI_Pixels.h"
#include "BitmapOperationsBitonal.h"

using namespace libCZI;
using namespace std;

bool BitonalBitmapOperations::GetPixelValue(const BitonalBitmapLockInfo& lockInfo, const libCZI::IntSize& extent, std::uint32_t x, std::uint32_t y)
{
	return BitmapOperationsBitonal::GetPixelFromBitonal(x, y, extent.w, extent.h, lockInfo.ptrData, lockInfo.stride);
}