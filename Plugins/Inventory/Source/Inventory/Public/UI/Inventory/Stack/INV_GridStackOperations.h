// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

class UINV_GridSlot;
class UINV_SlottedItem;
class UINV_HoverItem;
class UINV_InventoryItem;

/**
 * Manages stack-related operations for grid inventory system.
 * Handles stack swapping, merging, filling, and compatibility checks.
 */
class INVENTORY_API FINV_GridStackOperations
{
public:
	/**
	 * Calculate stack details for a grid slot.
	 *
	 * @param GridSlot The grid slot to analyze
	 * @param HoverItem The hover item being dragged
	 * @param ClickedItem The item in the clicked slot
	 * @return Stack details including counts and capacities
	 */
	static FINV_StackDetails CalculateStackDetails(
		const UINV_GridSlot* GridSlot,
		const UINV_HoverItem* HoverItem,
		const UINV_InventoryItem* ClickedItem);

	/**
	 * Check if clicked item and hover item are the same stackable item.
	 *
	 * @param HoverItem The hover item being dragged
	 * @param ClickedItem The item in the clicked slot
	 * @return True if both items are the same and stackable
	 */
	static bool IsSameStackable(
		const UINV_HoverItem* HoverItem,
		const UINV_InventoryItem* ClickedItem);

	/**
	 * Swap stack counts between grid slot and hover item.
	 *
	 * @param GridSlot Grid slot to update
	 * @param SlottedItem Slotted item widget to update
	 * @param HoverItem Hover item widget to update
	 * @param ClickedStackCount Stack count from clicked slot
	 * @param HoveredStackCount Stack count from hover item
	 */
	static void SwapStackCounts(
		UINV_GridSlot* GridSlot,
		UINV_SlottedItem* SlottedItem,
		UINV_HoverItem* HoverItem,
		int32 ClickedStackCount,
		int32 HoveredStackCount);

	/**
	 * Merge all hover item stacks into the clicked slot.
	 *
	 * @param GridSlot Grid slot to update
	 * @param SlottedItem Slotted item widget to update
	 * @param ClickedStackCount Current stack count in slot
	 * @param HoveredStackCount Stack count from hover item
	 */
	static void ConsumeHoverItemStacks(
		UINV_GridSlot* GridSlot,
		UINV_SlottedItem* SlottedItem,
		int32 ClickedStackCount,
		int32 HoveredStackCount);

	/**
	 * Fill clicked slot to capacity and keep remainder in hover.
	 *
	 * @param GridSlot Grid slot to update
	 * @param SlottedItem Slotted item widget to update
	 * @param HoverItem Hover item widget to update
	 * @param FillAmount Amount to transfer to slot
	 * @param Remainder Amount to keep in hover
	 */
	static void FillInStack(
		UINV_GridSlot* GridSlot,
		UINV_SlottedItem* SlottedItem,
		UINV_HoverItem* HoverItem,
		int32 FillAmount,
		int32 Remainder);
};
