#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

enum class EINV_AddItemAction : uint8
{
	None,
	NoRoom,
	AddStacks,
	AddNewItem
};

struct FINV_AddItemDecision
{
	EINV_AddItemAction Action { EINV_AddItemAction::None };
	int32 StackCountToAdd { 0 };
	int32 Remainder { 0 };
};

class INVENTORYCORE_API FINV_InventoryAddResolver
{
public:
	static FINV_AddItemDecision Resolve(const FINV_SlotAvailabilityResult& SpaceResult, bool bHasExistingStackTarget);
};
