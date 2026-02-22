#pragma once

#include "CoreMinimal.h"

class INVENTORYUI_API FINV_SpatialCategoryService
{
public:
	static int32 ResolveNextCategoryIndex(int32 CurrentIndex, int32 Direction, int32 CategoryCount);
};
