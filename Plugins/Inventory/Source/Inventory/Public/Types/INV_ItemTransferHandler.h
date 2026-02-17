// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

class UINV_InventoryItem;
class UINV_GridSlot;
class UINV_SlottedItem;
struct FINV_GridFragment;
struct FINV_ImageFragment;
struct FINV_ItemManifest;

/**
 * Result of attempting to swap a hover item with items occupying target space
 */
struct FINV_SwapResult
{
	/** Whether the swap was successful */
	bool bSuccess { false };

	/** The index where the hover item should be placed */
	int32 PlacementIndex { INDEX_NONE };

	/** Stack count to apply to placed hover item */
	int32 PlacementStackCount { 0 };

	/** Whether the placed hover item is stackable */
	bool bPlacementStackable { false };

	/** Items that need to be removed from the grid */
	TArray<TPair<UINV_InventoryItem*, int32>> ItemsToRemove;

	/** Items that need to be added back to the grid at new locations */
	TArray<TPair<UINV_InventoryItem*, int32>> ItemsToRelocate;

	/** Stack counts for relocated items */
	TArray<int32> RelocationStackCounts;

	/** Stackable flags for relocated items */
	TArray<bool> RelocationStackableFlags;
};

/**
 * Pure-logic handler for item transfers, swaps, and stack operations.
 * Separated from UI concerns to enable testability and reusability.
 */
class INVENTORY_API FINV_ItemTransferHandler
{
public:
	/**
	 * Plan a swap operation between a hover item and items at target location.
	 * Does not mutate any state - returns a plan that can be validated and applied.
	 *
	 * @param GridSlots All grid slots
	 * @param GridSize Dimensions of the grid
	 * @param HoverItemDimensions Size of the item being placed
	 * @param TargetDropIndex Where the hover item should be placed
	 * @param ClickedIndex The index that was clicked to initiate swap
	 * @return Swap plan with all necessary relocations, or bSuccess=false if impossible
	 */
	static FINV_SwapResult PlanSwapOperation(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		const FIntPoint& HoverItemDimensions,
		int32 TargetDropIndex,
		int32 ClickedIndex);

	/**
	 * Calculate stack transfer amounts for merging stacks.
	 *
	 * @param SourceStackCount Stack count of item being transferred
	 * @param TargetStackCount Current stack count at target location
	 * @param MaxStackSize Maximum stack size for this item type
	 * @param OutAmountToTransfer How many items to transfer from source to target
	 * @param OutSourceRemaining How many items remain in source stack
	 * @param OutTargetFinal Final stack count at target
	 */
	static void CalculateStackTransfer(
		int32 SourceStackCount,
		int32 TargetStackCount,
		int32 MaxStackSize,
		int32& OutAmountToTransfer,
		int32& OutSourceRemaining,
		int32& OutTargetFinal);

	/**
	 * Check if two items can have their stacks merged.
	 *
	 * @param SourceItem Item being transferred
	 * @param TargetItem Item at destination
	 * @return True if items are stack-compatible
	 */
	static bool AreItemsStackCompatible(
		const UINV_InventoryItem* SourceItem,
		const UINV_InventoryItem* TargetItem);

private:
	/**
	 * Simulate footprint occupancy to validate placement before applying changes.
	 */
	static TArray<bool> BuildOccupancyMap(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots);

	/**
	 * Mark a rectangular footprint in the occupancy map.
	 */
	static void MarkFootprint(
		TArray<bool>& OccupancyMap,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		int32 StartIndex,
		const FIntPoint& Dimensions,
		bool bOccupied);

	/**
	 * Check if an item can fit at the given index using the occupancy map.
	 */
	static bool CanFitAtIndex(
		const TArray<bool>& OccupancyMap,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		int32 StartIndex,
		const FIntPoint& Dimensions);

	/**
	 * Find first available placement for an item using first-fit algorithm.
	 *
	 * @param OccupancyMap Current grid occupancy state
	 * @param GridSlots All grid slots
	 * @param GridSize Grid dimensions
	 * @param Dimensions Item dimensions
	 * @param OutIndex Found index, or INDEX_NONE if no space
	 * @return True if space was found
	 */
	static bool FindFirstFitPlacement(
		const TArray<bool>& OccupancyMap,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		const FIntPoint& Dimensions,
		int32& OutIndex);
};
