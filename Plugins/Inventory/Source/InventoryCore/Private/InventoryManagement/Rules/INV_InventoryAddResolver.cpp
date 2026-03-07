#include "InventoryManagement/Rules/INV_InventoryAddResolver.h"

FINV_AddItemDecision FINV_InventoryAddResolver::Resolve(const FINV_SlotAvailabilityResult& SpaceResult,
	const bool bHasExistingStackTarget)
{
	FINV_AddItemDecision Decision;

	if (SpaceResult.TotalRoomToFill == 0)
	{
		Decision.Action = EINV_AddItemAction::NoRoom;
		return Decision;
	}

	if (bHasExistingStackTarget && SpaceResult.bStackable)
	{
		Decision.Action = EINV_AddItemAction::AddStacks;
		Decision.StackCountToAdd = SpaceResult.TotalRoomToFill;
		Decision.Remainder = SpaceResult.Remainder;
		return Decision;
	}

	if (SpaceResult.TotalRoomToFill > 0)
	{
		Decision.Action = EINV_AddItemAction::AddNewItem;
		Decision.StackCountToAdd = SpaceResult.bStackable ? SpaceResult.TotalRoomToFill : 1;
	}

	return Decision;
}
