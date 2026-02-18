// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Items/INV_GridItemOperations.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "UI/Inventory/SlottedItems/INV_SlottedItem.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "InventoryManagement/Utils/INV_GridIteration.h"

void UINV_GridItemOperations::UpdateGridSlots(
	TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	UINV_InventoryItem* NewItem,
	const int32 Index,
	const bool bStackableItem,
	const int32 StackAmount,
	const int32 GridWidth)
{
	if (!GridSlots.IsValidIndex(Index))
	{
		return;
	}

	if (!IsValid(NewItem))
	{
		return;
	}

	if (bStackableItem)
	{
		// Store stack count on the upper-left slot.
		GridSlots[Index]->SetStackCount(StackAmount);
	}

	const FINV_GridFragment* GridFragment = NewItem->GetCachedGridFragment();
	if (!GridFragment)
	{
		return;
	}

	const FIntPoint Dimensions = GridFragment->GetGridSize();

	FINV_GridIteration::ForEach2D(GridSlots, Index, Dimensions, GridWidth,
		[&](UINV_GridSlot* GridSlot)
		{
			// Mark all slots as occupied by this item.
			GridSlot->SetInventoryItem(NewItem);
			GridSlot->SetUpperLeftIndex(Index);
			GridSlot->SetOccupiedTexture();
			GridSlot->SetAvailability(false);
		});
}

void UINV_GridItemOperations::RemoveItemFromGrid(
	TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
	const UINV_InventoryItem* InventoryItem,
	const int32 GridIndex,
	const int32 GridWidth)
{
	if (!IsValid(InventoryItem))
	{
		return;
	}

	if (!GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	const FINV_GridFragment* GridFragment = InventoryItem->GetCachedGridFragment();
	if (!GridFragment)
	{
		return;
	}

	FINV_GridIteration::ForEach2D(GridSlots, GridIndex, GridFragment->GetGridSize(), GridWidth,
		[&](UINV_GridSlot* GridSlot)
		{
			// Clear slot state.
			GridSlot->SetInventoryItem(nullptr);
			GridSlot->SetUpperLeftIndex(INDEX_NONE);
			GridSlot->SetUnoccupiedTexture();
			GridSlot->SetAvailability(true);
			GridSlot->SetStackCount(0);
		});

	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UINV_SlottedItem> FoundSlottedItem;
		SlottedItems.RemoveAndCopyValue(GridIndex, FoundSlottedItem);
		if (IsValid(FoundSlottedItem))
		{
			FoundSlottedItem->RemoveFromParent();
		}
	}
}

void UINV_GridItemOperations::ApplyStackUpdates(
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
	const FINV_SlotAvailabilityResult& Result)
{
	// Apply stack changes to existing slots.
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (!Availability.bItemAtIndex)
		{
			continue;
		}

		if (!GridSlots.IsValidIndex(Availability.Index))
		{
			continue;
		}

		const TObjectPtr<UINV_SlottedItem>* SlottedItemPtr = SlottedItems.Find(Availability.Index);
		if (!SlottedItemPtr)
		{
			continue;
		}

		const TObjectPtr<UINV_SlottedItem>& SlottedItem = *SlottedItemPtr;
		if (!IsValid(SlottedItem))
		{
			continue;
		}

		const int32 NewStackCount = GridSlots[Availability.Index]->GetStackCount() + Availability.AmountToFill;
		GridSlots[Availability.Index]->SetStackCount(NewStackCount);
		SlottedItem->UpdateStackCount(NewStackCount);
	}
}

