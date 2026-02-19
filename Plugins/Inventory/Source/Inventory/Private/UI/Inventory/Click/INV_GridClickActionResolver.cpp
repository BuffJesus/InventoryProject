// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Click/INV_GridClickActionResolver.h"
#include "UI/Inventory/HoverItem/INV_HoverItem.h"
#include "UI/Inventory/GridSlots/INV_GridSlot.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "UI/Inventory/EventHandling/INV_GridEventHandler.h"

FINV_GridClickResult FINV_GridClickActionResolver::ResolveSlottedItemClick(
	const UINV_HoverItem* HoverItem,
	const UINV_InventoryItem* ClickedItem,
	const FPointerEvent& MouseEvent,
	int32 GridIndex,
	const FINV_StackDetails& StackDetails,
	int32 ItemDropIndex)
{
	FINV_GridClickResult Result;

	if (!IsValid(ClickedItem))
	{
		return Result;
	}

	// No hover item + left click = pickup
	if (!IsValid(HoverItem) && FINV_GridEventHandler::IsLeftClick(MouseEvent))
	{
		Result.Action = EINV_ClickAction::Pickup;
		Result.TargetIndex = GridIndex;
		return Result;
	}

	// Right click = show popup
	if (FINV_GridEventHandler::IsRightClick(MouseEvent))
	{
		if (IsValid(HoverItem))
		{
			return Result; // No action if holding item
		}
		Result.Action = EINV_ClickAction::CreatePopup;
		Result.TargetIndex = GridIndex;
		return Result;
	}

	// Check if items are same stackable type
	if (IsSameStackable(HoverItem, ClickedItem))
	{
		// Determine stack action
		const FINV_ClickActionResult::EAction StackAction = FINV_GridEventHandler::DetermineStackAction(
			StackDetails.ClickedStackCount,
			StackDetails.HoveredStackCount,
			StackDetails.MaxStackSize,
			StackDetails.RoomInClickedSlot);

		switch (StackAction)
		{
		case FINV_ClickActionResult::EAction::SwapStacks:
			Result.Action = EINV_ClickAction::SwapStackCounts;
			Result.TargetIndex = GridIndex;
			return Result;

		case FINV_ClickActionResult::EAction::MergeStacks:
			Result.Action = EINV_ClickAction::ConsumeHoverStacks;
			Result.TargetIndex = GridIndex;
			return Result;

		case FINV_ClickActionResult::EAction::FillStack:
			Result.Action = EINV_ClickAction::FillStack;
			Result.TargetIndex = GridIndex;
			Result.AuxiliaryValue = StackDetails.RoomInClickedSlot;
			return Result;

		case FINV_ClickActionResult::EAction::None:
		default:
			// Check if this is returning to same slot - do nothing
			if (IsValid(HoverItem) && HoverItem->GetPreviousGridIndex() == GridIndex)
			{
				return Result;
			}
			// Otherwise fall through to swap logic
			break;
		}
	}

	// Check if should use spatial swap logic
	if (ShouldUseSpatialSwap(HoverItem, ClickedItem, GridIndex, ItemDropIndex))
	{
		Result.Action = EINV_ClickAction::SwapItems;
		Result.TargetIndex = GridIndex;
		return Result;
	}

	// Default to swap
	if (IsValid(HoverItem))
	{
		Result.Action = EINV_ClickAction::SwapItems;
		Result.TargetIndex = GridIndex;
	}

	return Result;
}

FINV_GridClickResult FINV_GridClickActionResolver::ResolveEmptySlotClick(
	const UINV_HoverItem* HoverItem,
	const FINV_SpaceQueryResult& CurrentQueryResult,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	int32 GridIndex,
	int32 ItemDropIndex)
{
	FINV_GridClickResult Result;

	if (!IsValid(HoverItem))
	{
		return Result;
	}

	// If clicking on an item that overlaps with hover placement, route to that item
	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		Result.Action = EINV_ClickAction::SwapItems;
		Result.TargetIndex = CurrentQueryResult.UpperLeftIndex;
		return Result;
	}

	// Single blocker - swap with that item
	if (CurrentQueryResult.BlockingUpperLeftIndices.Num() == 1)
	{
		const int32 BlockingIndex = CurrentQueryResult.BlockingUpperLeftIndices[0];
		if (GridSlots.IsValidIndex(BlockingIndex) && GridSlots[BlockingIndex]->GetInventoryItem().IsValid())
		{
			Result.Action = EINV_ClickAction::SwapItems;
			Result.TargetIndex = BlockingIndex;
			return Result;
		}
	}

	// Multi-blocker - find best anchor
	if (!GridSlots[GridIndex]->GetInventoryItem().IsValid() && CurrentQueryResult.BlockingUpperLeftIndices.Num() > 1)
	{
		const UINV_InventoryItem* HoverInventoryItem = HoverItem->GetInventoryItem();
		const FGameplayTag HoverItemType = IsValid(HoverInventoryItem)
			? HoverInventoryItem->GetItemManifest().GetItemType()
			: FGameplayTag();

		const int32 BestAnchor = FindBestMultiBlockerAnchor(
			CurrentQueryResult.BlockingUpperLeftIndices,
			GridSlots,
			HoverItemType);

		if (GridSlots.IsValidIndex(BestAnchor) && GridSlots[BestAnchor]->GetInventoryItem().IsValid())
		{
			Result.Action = EINV_ClickAction::SwapItems;
			Result.TargetIndex = BestAnchor;
			return Result;
		}
	}

	// Clicked slot has an item - route to that item's upper left
	if (GridSlots[GridIndex]->GetInventoryItem().IsValid())
	{
		const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex();
		if (GridSlots.IsValidIndex(UpperLeftIndex))
		{
			Result.Action = EINV_ClickAction::SwapItems;
			Result.TargetIndex = UpperLeftIndex;
			return Result;
		}
	}

	// Free space - place item
	if (CurrentQueryResult.bHasSpace)
	{
		Result.Action = EINV_ClickAction::PlaceItem;
		Result.TargetIndex = ItemDropIndex;
	}

	return Result;
}

int32 FINV_GridClickActionResolver::FindBestMultiBlockerAnchor(
	const TArray<int32>& BlockingIndices,
	const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
	const FGameplayTag& HoverItemType)
{
	int32 BestAnchorIndex = INDEX_NONE;
	int32 BestScore = TNumericLimits<int32>::Lowest();

	for (const int32 BlockingUpperLeftIndex : BlockingIndices)
	{
		if (!GridSlots.IsValidIndex(BlockingUpperLeftIndex)) continue;

		const UINV_InventoryItem* BlockingItem = GridSlots[BlockingUpperLeftIndex]->GetInventoryItem().Get();
		if (!IsValid(BlockingItem)) continue;

		const FINV_GridFragment* BlockingGridFragment = BlockingItem->GetCachedGridFragment();
		const FIntPoint BlockingDimensions = BlockingGridFragment ? BlockingGridFragment->GetGridSize() : FIntPoint(1, 1);
		const int32 BlockingArea = BlockingDimensions.X * BlockingDimensions.Y;

		// Prefer larger anchors; bias toward same-type
		const bool bSameTypeAsHover = HoverItemType.IsValid() &&
			BlockingItem->GetItemManifest().GetItemType().MatchesTagExact(HoverItemType);
		const int32 Score = BlockingArea + (bSameTypeAsHover ? 1000 : 0);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAnchorIndex = BlockingUpperLeftIndex;
		}
	}

	return BestAnchorIndex;
}

bool FINV_GridClickActionResolver::IsSameStackable(
	const UINV_HoverItem* HoverItem,
	const UINV_InventoryItem* ClickedItem)
{
	if (!IsValid(HoverItem) || !IsValid(ClickedItem))
	{
		return false;
	}

	const bool bIsSameItem = ClickedItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = HoverItem->IsStackable();
	return bIsSameItem && bIsStackable;
}

bool FINV_GridClickActionResolver::ShouldUseSpatialSwap(
	const UINV_HoverItem* HoverItem,
	const UINV_InventoryItem* ClickedItem,
	int32 GridIndex,
	int32 ItemDropIndex)
{
	if (!IsValid(HoverItem) || !IsValid(ClickedItem))
	{
		return false;
	}

	const FIntPoint HoverDimensions = HoverItem->GetGridDimensions();
	const FINV_GridFragment* ClickedGridFragment = ClickedItem->GetCachedGridFragment();
	const FIntPoint ClickedDimensions = ClickedGridFragment ? ClickedGridFragment->GetGridSize() : FIntPoint(1, 1);

	const bool bDirectSameSlotDrop = ItemDropIndex != INDEX_NONE && ItemDropIndex == GridIndex;
	const bool bIsSingleTileInteraction = HoverDimensions == FIntPoint(1, 1) && ClickedDimensions == FIntPoint(1, 1);

	// Use spatial swap if not a direct same-slot drop OR not single-tile
	return !bDirectSameSlotDrop || !bIsSingleTileInteraction;
}

