// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/EventHandling/INV_GridEventHandler.h"
#include "Items/INV_InventoryItem.h"
#include "Items/Manifest/INV_ItemManifest.h"
#include "Items/Fragments/INV_ItemFragment.h"
#include "Framework/Application/SlateApplication.h"

bool FINV_GridEventHandler::IsLeftClick(const FPointerEvent& MouseEvent)
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

bool FINV_GridEventHandler::IsRightClick(const FPointerEvent& MouseEvent)
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool FINV_GridEventHandler::IsShiftClick(const FPointerEvent& MouseEvent)
{
	return MouseEvent.IsShiftDown();
}

int32 FINV_GridEventHandler::FindBestAnchorForMultiBlocker(
	const TArray<int32>& BlockingIndices,
	TFunction<FIntPoint(int32)> GetItemDimensions,
	TFunction<FGameplayTag(int32)> GetItemType,
	const FGameplayTag& HoverItemType)
{
	int32 BestAnchorIndex = INDEX_NONE;
	int32 BestScore = TNumericLimits<int32>::Lowest();

	for (const int32 BlockingIndex : BlockingIndices)
	{
		const FIntPoint Dimensions = GetItemDimensions(BlockingIndex);
		const int32 Area = Dimensions.X * Dimensions.Y;

		// Prefer larger anchors; bias toward same-type so "same item" swaps behave intuitively
		const FGameplayTag BlockingType = GetItemType(BlockingIndex);
		const bool bSameTypeAsHover = HoverItemType.IsValid() && BlockingType.MatchesTagExact(HoverItemType);
		const int32 Score = Area + (bSameTypeAsHover ? 1000 : 0);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAnchorIndex = BlockingIndex;
		}
	}

	return BestAnchorIndex;
}

bool FINV_GridEventHandler::CanMergeStacks(
	const UINV_InventoryItem* Item1,
	const UINV_InventoryItem* Item2)
{
	if (!IsValid(Item1) || !IsValid(Item2))
	{
		return false;
	}

	if (!Item1->IsStackable() || !Item2->IsStackable())
	{
		return false;
	}

	// Items must have same type
	const FINV_ItemManifest& Manifest1 = Item1->GetItemManifest();
	const FINV_ItemManifest& Manifest2 = Item2->GetItemManifest();

	if (Manifest1.GetItemType() != Manifest2.GetItemType())
	{
		return false;
	}

	// If rarity is enabled, items must have same rarity
	if (Item1->IsItemRarityEnabled() && Item2->IsItemRarityEnabled())
	{
		if (Item1->GetItemRarityTag() != Item2->GetItemRarityTag())
		{
			return false;
		}
	}

	return true;
}

void FINV_GridEventHandler::CalculateStackMerge(
	const int32 SourceStackCount,
	const int32 TargetStackCount,
	const int32 MaxStackSize,
	int32& OutAmountToTransfer,
	int32& OutSourceRemaining,
	int32& OutTargetFinal)
{
	const int32 TargetAvailableSpace = FMath::Max(0, MaxStackSize - TargetStackCount);
	OutAmountToTransfer = FMath::Min(SourceStackCount, TargetAvailableSpace);
	OutSourceRemaining = SourceStackCount - OutAmountToTransfer;
	OutTargetFinal = TargetStackCount + OutAmountToTransfer;
}

FINV_ClickActionResult::EAction FINV_GridEventHandler::DetermineStackAction(
	const int32 ClickedStackCount,
	const int32 HoverStackCount,
	const int32 MaxStackSize,
	const int32 RoomInClickedSlot)
{
	// Should we swap stack counts? (Room in clicked slot == 0 && HoveredStackCount < MaxStackSize)
	if (ShouldSwapStackCounts(RoomInClickedSlot, HoverStackCount, MaxStackSize))
	{
		return FINV_ClickActionResult::EAction::SwapStacks;
	}

	// Should we consume hover item's stacks? (Room in clicked slot >= HoveredStackCount)
	if (ShouldConsumeHoverItem(HoverStackCount, RoomInClickedSlot))
	{
		return FINV_ClickActionResult::EAction::MergeStacks;
	}

	// Should we fill in the stacks of the clicked item? (and not consume hover item)
	if (ShouldFillStack(RoomInClickedSlot, HoverStackCount))
	{
		return FINV_ClickActionResult::EAction::FillStack;
	}

	// Clicked slot already full
	if (RoomInClickedSlot == 0)
	{
		return FINV_ClickActionResult::EAction::SwapItems;
	}

	return FINV_ClickActionResult::EAction::None;
}

bool FINV_GridEventHandler::ShouldSwapStackCounts(
	const int32 RoomInClickedSlot,
	const int32 HoverStackCount,
	const int32 MaxStackSize)
{
	return RoomInClickedSlot == 0 && HoverStackCount < MaxStackSize;
}

bool FINV_GridEventHandler::ShouldConsumeHoverItem(
	const int32 HoverStackCount,
	const int32 RoomInClickedSlot)
{
	return RoomInClickedSlot >= HoverStackCount;
}

bool FINV_GridEventHandler::ShouldFillStack(
	const int32 RoomInClickedSlot,
	const int32 HoverStackCount)
{
	return RoomInClickedSlot > 0 && RoomInClickedSlot < HoverStackCount;
}
