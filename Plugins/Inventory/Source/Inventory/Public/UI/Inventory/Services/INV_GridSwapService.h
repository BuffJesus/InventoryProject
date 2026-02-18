#pragma once

#include "CoreMinimal.h"

class UINV_GridSlot;
class UINV_InventoryItem;

struct FINV_GridSwapCallbacks
{
	TFunction<void(UINV_InventoryItem* Item, int32 GridIndex, int32 PreviousGridIndex)> AssignHoverItem;
	TFunction<void(UINV_InventoryItem* Item, int32 GridIndex)> RemoveItemFromGrid;
	TFunction<void(UINV_InventoryItem* Item, int32 GridIndex, bool bStackable, int32 StackCount)> AddItemAtIndex;
	TFunction<void(UINV_InventoryItem* Item, int32 GridIndex, bool bStackable, int32 StackCount)> UpdateGridSlots;
	TFunction<void()> RefreshGridSlotVisuals;
};

class INVENTORY_API FINV_GridSwapService
{
public:
	static bool ExecuteSwapWithHoverItem(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		int32 PreferredDropIndex,
		UINV_InventoryItem* ClickedInventoryItem,
		int32 GridIndex,
		UINV_InventoryItem* HoverInventoryItem,
		const FIntPoint& HoverItemDimensions,
		int32 HoverStackCount,
		bool bHoverStackable,
		int32 HoverPreviousIndex,
		const FINV_GridSwapCallbacks& Callbacks);
};
