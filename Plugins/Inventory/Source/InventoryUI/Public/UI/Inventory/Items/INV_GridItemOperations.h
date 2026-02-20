// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UINV_GridSlot;
class UINV_SlottedItem;
class UINV_InventoryItem;
struct FINV_GridFragment;
struct FINV_SlotAvailabilityResult;

/**
 * Utility class for grid item operations.
 * Handles updating grid slot state and managing item placement/removal.
 */
class INVENTORYUI_API FINV_GridItemOperations
{
public:
	/**
	 * Update grid slots when adding an item to the grid.
	 * Marks all occupied slots and sets item references.
	 *
	 * @param GridSlots All grid slots in the inventory
	 * @param NewItem Item being added
	 * @param Index Upper-left index where item is placed
	 * @param bStackableItem Whether this is a stackable item
	 * @param StackAmount Stack count for stackable items
	 * @param GridWidth Width of the grid for coordinate calculations
	 */
	static void UpdateGridSlots(
		TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		UINV_InventoryItem* NewItem,
		int32 Index,
		bool bStackableItem,
		int32 StackAmount,
		int32 GridWidth);

	/**
	 * Remove an item from the grid, clearing all occupied slots.
	 *
	 * @param GridSlots All grid slots in the inventory
	 * @param InventoryItem Item to remove
	 * @param GridIndex Upper-left index of the item
	 * @param GridWidth Width of the grid for coordinate calculations
	 */
	static void RemoveItemFromGrid(
		TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const UINV_InventoryItem* InventoryItem,
		int32 GridIndex,
		int32 GridWidth);

	/**
	 * Apply stack count updates to existing items in the grid.
	 *
	 * @param GridSlots All grid slots in the inventory
	 * @param SlottedItems Map of slotted item widgets
	 * @param Result Availability result containing stack updates
	 */
	static void ApplyStackUpdates(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
		const FINV_SlotAvailabilityResult& Result);
};
