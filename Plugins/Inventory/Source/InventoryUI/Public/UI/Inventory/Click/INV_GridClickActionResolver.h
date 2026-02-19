// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/INV_GridTypes.h"
#include "INV_GridClickActionResolver.generated.h"

class UINV_HoverItem;
class UINV_InventoryItem;
class UINV_GridSlot;
struct FINV_StackDetails;

/**
 * Actions that can be performed when clicking on a grid slot.
 */
UENUM()
enum class EINV_ClickAction : uint8
{
	None,
	Pickup,              // Pick up item from grid
	CreatePopup,         // Show right-click context menu
	SwapStackCounts,     // Swap stack counts between hover and clicked
	ConsumeHoverStacks,  // Merge all hover stacks into clicked slot
	FillStack,           // Fill clicked slot and keep remainder in hover
	SwapItems,           // Swap hover item with clicked item
	PlaceItem            // Place hover item on empty slots
};

/**
 * Result of click action resolution containing the action to perform.
 */
USTRUCT()
struct FINV_GridClickResult
{
	GENERATED_BODY()

	EINV_ClickAction Action = EINV_ClickAction::None;
	int32 TargetIndex = INDEX_NONE;
	int32 AuxiliaryValue = 0; // Used for fill amounts, etc.
};

/**
 * Resolves what action should be performed when clicking on grid items.
 * Handles complex decision logic for stack operations, swaps, and placements.
 */
class INVENTORYUI_API FINV_GridClickActionResolver
{
public:
	/**
	 * Determine action to perform when clicking on an item in the grid.
	 *
	 * @param HoverItem Current hover item (or null if not dragging)
	 * @param ClickedItem Item that was clicked (or null if clicking empty)
	 * @param MouseEvent Mouse event data
	 * @param GridIndex Index where click occurred
	 * @param StackDetails Stack information for stackable operations
	 * @param ItemDropIndex Computed drop index for hover item
	 * @return Action to perform and associated data
	 */
	static FINV_GridClickResult ResolveSlottedItemClick(
		const UINV_HoverItem* HoverItem,
		const UINV_InventoryItem* ClickedItem,
		const FPointerEvent& MouseEvent,
		int32 GridIndex,
		const FINV_StackDetails& StackDetails,
		int32 ItemDropIndex);

	/**
	 * Determine action when clicking empty grid slot.
	 *
	 * @param HoverItem Current hover item
	 * @param CurrentQueryResult Space query result for placement
	 * @param GridSlots All grid slots
	 * @param GridIndex Index where click occurred
	 * @param ItemDropIndex Computed drop index for placement
	 * @return Action to perform and target index
	 */
	static FINV_GridClickResult ResolveEmptySlotClick(
		const UINV_HoverItem* HoverItem,
		const FINV_SpaceQueryResult& CurrentQueryResult,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridIndex,
		int32 ItemDropIndex);

	/**
	 * Find best anchor item for multi-blocker swap scenario.
	 *
	 * @param BlockingIndices Indices of items blocking placement
	 * @param GridSlots All grid slots
	 * @param HoverItemType Type tag of hover item for matching
	 * @return Index of best anchor item
	 */
	static int32 FindBestMultiBlockerAnchor(
		const TArray<int32>& BlockingIndices,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FGameplayTag& HoverItemType);

private:
	/**
	 * Check if two items are the same stackable item.
	 */
	static bool IsSameStackable(
		const UINV_HoverItem* HoverItem,
		const UINV_InventoryItem* ClickedItem);

	/**
	 * Check if items should swap with spatial placement logic.
	 */
	static bool ShouldUseSpatialSwap(
		const UINV_HoverItem* HoverItem,
		const UINV_InventoryItem* ClickedItem,
		int32 GridIndex,
		int32 ItemDropIndex);
};
