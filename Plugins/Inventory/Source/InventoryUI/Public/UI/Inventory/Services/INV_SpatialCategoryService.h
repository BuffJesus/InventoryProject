#pragma once

#include "CoreMinimal.h"

struct FKey;

class INVENTORYUI_API FINV_SpatialCategoryService
{
public:
	static bool IsGridNavigationInput(const FKey& PressedKey);
	static int32 ResolveNextCategoryIndex(int32 CurrentIndex, int32 Direction, int32 CategoryCount);
};
