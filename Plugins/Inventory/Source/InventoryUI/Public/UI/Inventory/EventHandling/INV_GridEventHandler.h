// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameplayTagContainer.h"
#include "Templates/Function.h"
#include "INV_GridEventHandler.generated.h"

class UINV_InventoryItem;
struct FPointerEvent;

/**
 * Result of analyzing a grid slot click action
 */
USTRUCT()
struct FINV_ClickActionResult
{
	GENERATED_BODY()

	enum class EAction : uint8
	{
		None,
		PickupItem,
		PlaceHoverItem,
		SwapItems,
		SwapStacks,
		MergeStacks,
		FillStack,
		ShowItemPopup
	};

	EAction Action { EAction::None };
	int32 TargetIndex { INDEX_NONE };
	int32 SecondaryIndex { INDEX_NONE };
	int32 Amount { 0 };
	bool bConsumeHoverItem { false };
};

/**
 * Utility class for handling grid input events and determining appropriate actions.
 * Provides pure decision logic without mutating state.
 */
class INVENTORYUI_API FINV_GridEventHandler
{
public:
	/**
	 * Check if a mouse event is a left click.
	 */
	static bool IsLeftClick(const FPointerEvent& MouseEvent);

	/**
	 * Check if a mouse event is a right click.
	 */
	static bool IsRightClick(const FPointerEvent& MouseEvent);

	/**
	 * Check if a mouse event is a shift+click.
	 */
	static bool IsShiftClick(const FPointerEvent& MouseEvent);

	/**
	 * Determine the best anchor item when multiple items are blocking placement.
	 * Uses heuristic: prefer larger items, bias toward same type as hover item.
	 *
	 * @param BlockingIndices Upper-left indices of blocking items
	 * @param ItemDimensions Lookup function for item dimensions
	 * @param ItemTypes Lookup function for item types
	 * @param HoverItemType Type of the hover item
	 * @return Best anchor index, or INDEX_NONE if none found
	 */
	static int32 FindBestAnchorForMultiBlocker(
		const TArray<int32>& BlockingIndices,
		TFunction<FIntPoint(int32)> GetItemDimensions,
		TFunction<FGameplayTag(int32)> GetItemType,
		const FGameplayTag& HoverItemType);

	/**
	 * Determine if two items can have their stacks merged.
	 */
	static bool CanMergeStacks(
		const UINV_InventoryItem* Item1,
		const UINV_InventoryItem* Item2);

	/**
	 * Calculate stack operation details for merging.
	 *
	 * @param SourceStackCount Stack count of item being moved
	 * @param TargetStackCount Stack count at destination
	 * @param MaxStackSize Maximum stack size
	 * @param OutAmountToTransfer How many to move
	 * @param OutSourceRemaining How many remain in source
	 * @param OutTargetFinal Final count at destination
	 */
	static void CalculateStackMerge(
		int32 SourceStackCount,
		int32 TargetStackCount,
		int32 MaxStackSize,
		int32& OutAmountToTransfer,
		int32& OutSourceRemaining,
		int32& OutTargetFinal);

	/**
	 * Determine action for stack interaction.
	 *
	 * @param ClickedStackCount Current stack at clicked location
	 * @param HoverStackCount Stack count of hover item
	 * @param MaxStackSize Maximum stack size
	 * @param RoomInClickedSlot Available space in clicked stack
	 * @return Action to perform
	 */
	static FINV_ClickActionResult::EAction DetermineStackAction(
		int32 ClickedStackCount,
		int32 HoverStackCount,
		int32 MaxStackSize,
		int32 RoomInClickedSlot);

private:
	/**
	 * Check if stack counts should be swapped (full destination, partial source).
	 */
	static bool ShouldSwapStackCounts(
		int32 RoomInClickedSlot,
		int32 HoverStackCount,
		int32 MaxStackSize);

	/**
	 * Check if hover item should be fully consumed into destination.
	 */
	static bool ShouldConsumeHoverItem(
		int32 HoverStackCount,
		int32 RoomInClickedSlot);

	/**
	 * Check if destination should be filled and source kept.
	 */
	static bool ShouldFillStack(
		int32 RoomInClickedSlot,
		int32 HoverStackCount);
};
