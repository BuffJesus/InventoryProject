#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/Click/INV_GridClickActionResolver.h"
#include "UI/Inventory/Stack/INV_GridStackOperations.h"

class UINV_InventoryItem;

struct FINV_GridClickExecutionCallbacks
{
	TFunction<void(UINV_InventoryItem* Item, int32 GridIndex)> Pickup;
	TFunction<void(int32 GridIndex)> CreatePopup;
	TFunction<void(int32 ClickedStackCount, int32 HoveredStackCount, int32 GridIndex)> SwapStackCounts;
	TFunction<void(int32 ClickedStackCount, int32 HoveredStackCount, int32 GridIndex)> ConsumeHoverStacks;
	TFunction<void(int32 FillAmount, int32 Remainder, int32 GridIndex)> FillStack;
	TFunction<void(UINV_InventoryItem* ClickedItem, int32 GridIndex)> SwapItems;
	TFunction<void(int32 GridIndex)> RouteToSlottedClick;
	TFunction<void(int32 GridIndex)> PlaceItem;
};

class INVENTORY_API FINV_GridClickExecutionService
{
public:
	static void ExecuteSlottedItemClick(
		const FINV_GridClickResult& ActionResult,
		const FINV_StackDetails& StackDetails,
		UINV_InventoryItem* ClickedInventoryItem,
		int32 GridIndex,
		const FINV_GridClickExecutionCallbacks& Callbacks);

	static void ExecuteGridSlotClick(
		const FINV_GridClickResult& ActionResult,
		const FINV_GridClickExecutionCallbacks& Callbacks);
};
