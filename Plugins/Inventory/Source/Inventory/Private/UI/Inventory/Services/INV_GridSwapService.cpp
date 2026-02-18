#include "UI/Inventory/Services/INV_GridSwapService.h"
#include "UI/Inventory/Transfer/INV_ItemTransferHandler.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FINV_GridSwapService::ExecuteSwapWithHoverItem(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FIntPoint& GridSize,
	const int32 PreferredDropIndex,
	UINV_InventoryItem* ClickedInventoryItem,
	const int32 GridIndex,
	UINV_InventoryItem* HoverInventoryItem,
	const FIntPoint& HoverItemDimensions,
	const int32 HoverStackCount,
	const bool bHoverStackable,
	const int32 HoverPreviousIndex,
	const FINV_GridSwapCallbacks& Callbacks)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(INV_GridSwapService_ExecuteSwapWithHoverItem);

	if (!IsValid(HoverInventoryItem)) return false;
	if (!IsValid(ClickedInventoryItem)) return false;
	if (!GridSlots.IsValidIndex(GridIndex)) return false;
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid()) return false;

	// Prefer the computed hover drop index, but fall back to clicked index if needed.
	const int32 TargetDropIndex = GridSlots.IsValidIndex(PreferredDropIndex) ? PreferredDropIndex : GridIndex;

	const FINV_SwapResult SwapPlan = FINV_ItemTransferHandler::PlanSwapOperation(
		GridSlots,
		GridSize,
		HoverItemDimensions,
		TargetDropIndex,
		GridIndex);
	if (!SwapPlan.bSuccess) return false;

	if (Callbacks.AssignHoverItem)
	{
		Callbacks.AssignHoverItem(ClickedInventoryItem, GridIndex, HoverPreviousIndex);
	}

	for (const TPair<UINV_InventoryItem*, int32>& ItemToRemove : SwapPlan.ItemsToRemove)
	{
		if (Callbacks.RemoveItemFromGrid && IsValid(ItemToRemove.Key))
		{
			Callbacks.RemoveItemFromGrid(ItemToRemove.Key, ItemToRemove.Value);
		}
	}

	if (Callbacks.AddItemAtIndex)
	{
		Callbacks.AddItemAtIndex(HoverInventoryItem, SwapPlan.PlacementIndex, bHoverStackable, HoverStackCount);
	}
	if (Callbacks.UpdateGridSlots)
	{
		Callbacks.UpdateGridSlots(HoverInventoryItem, SwapPlan.PlacementIndex, bHoverStackable, HoverStackCount);
	}

	for (int32 i = 0; i < SwapPlan.ItemsToRelocate.Num(); ++i)
	{
		const TPair<UINV_InventoryItem*, int32>& Relocation = SwapPlan.ItemsToRelocate[i];
		const int32 StackCount = SwapPlan.RelocationStackCounts[i];
		const bool bStackable = SwapPlan.RelocationStackableFlags[i];

		if (Callbacks.AddItemAtIndex)
		{
			Callbacks.AddItemAtIndex(Relocation.Key, Relocation.Value, bStackable, StackCount);
		}
		if (Callbacks.UpdateGridSlots)
		{
			Callbacks.UpdateGridSlots(Relocation.Key, Relocation.Value, bStackable, StackCount);
		}
	}

	if (Callbacks.RefreshGridSlotVisuals)
	{
		Callbacks.RefreshGridSlotVisuals();
	}

	return true;
}
