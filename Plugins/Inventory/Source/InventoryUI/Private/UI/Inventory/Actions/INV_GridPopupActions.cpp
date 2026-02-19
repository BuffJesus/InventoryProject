// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Actions/INV_GridPopupActions.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "Items/INV_InventoryItem.h"

void FINV_GridPopupActions::ExecuteSplit(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
	UINV_InventoryItem* Item,
	const int32 Index,
	const int32 SplitAmount,
	TFunction<void(UINV_InventoryItem*, int32, int32)> AssignHoverCallback,
	TFunction<void(UINV_HoverItem*, int32)> UpdateHoverStackCallback)
{
	if (!IsValid(Item))
	{
		return;
	}

	if (!GridSlots.IsValidIndex(Index))
	{
		return;
	}

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return;
	}

	UINV_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;

	UpperLeftGridSlot->SetStackCount(NewStackCount);

	const TObjectPtr<UINV_SlottedItem>* SlottedItemPtr = SlottedItems.Find(UpperLeftIndex);
	if (SlottedItemPtr && IsValid(*SlottedItemPtr))
	{
		(*SlottedItemPtr)->UpdateStackCount(NewStackCount);
	}

	// Use callback to assign hover item (widget-dependent operation)
	if (AssignHoverCallback)
	{
		AssignHoverCallback(Item, UpperLeftIndex, UpperLeftIndex);
	}

	// Use callback to update hover stack (widget-dependent operation)
	if (UpdateHoverStackCallback)
	{
		// Note: HoverItem is managed by caller, callback will handle it
		UpdateHoverStackCallback(nullptr, SplitAmount);
	}
}

int32 FINV_GridPopupActions::ExecuteConsume(
	TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
	UINV_InventoryItem* Item,
	const int32 Index)
{
	if (!IsValid(Item))
	{
		return 0;
	}

	if (!GridSlots.IsValidIndex(Index))
	{
		return 0;
	}

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return 0;
	}

	UINV_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;

	UpperLeftGridSlot->SetStackCount(NewStackCount);

	const TObjectPtr<UINV_SlottedItem>* SlottedItemPtr = SlottedItems.Find(UpperLeftIndex);
	if (SlottedItemPtr && IsValid(*SlottedItemPtr))
	{
		(*SlottedItemPtr)->UpdateStackCount(NewStackCount);
	}

	return NewStackCount;
}
