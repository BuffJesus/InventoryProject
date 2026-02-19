// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

class UINV_GridSlot;
class UINV_InventoryItem;
struct FINV_GridFragment;
enum class EINV_GridSlotState : uint8;

/**
 * Manages visual state of grid slots including highlighting, graying out, and texture updates.
 * Handles state transitions and ensures consistent visual feedback.
 */
class INVENTORY_API FINV_GridStateManager
{
public:
	/**
	 * Highlight a rectangular region of slots.
	 *
	 * @param GridSlots All grid slots
	 * @param GridWidth Width of the grid
	 * @param Index Starting index
	 * @param Dimensions Size of region to highlight
	 * @param bMouseWithinCanvas Whether mouse is within canvas bounds
	 */
	static void HighlightSlots(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridWidth,
		int32 Index,
		const FIntPoint& Dimensions,
		bool bMouseWithinCanvas);

	/**
	 * Remove highlighting from a rectangular region of slots.
	 *
	 * @param GridSlots All grid slots
	 * @param GridWidth Width of the grid
	 * @param Index Starting index
	 * @param Dimensions Size of region to unhighlight
	 */
	static void UnHighlightSlots(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridWidth,
		int32 Index,
		const FIntPoint& Dimensions);

	/**
	 * Change the visual state of a rectangular region.
	 *
	 * @param GridSlots All grid slots
	 * @param GridWidth Width of the grid
	 * @param Index Starting index
	 * @param Dimensions Size of region
	 * @param State Visual state to apply
	 */
	static void ChangeSlotState(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridWidth,
		int32 Index,
		const FIntPoint& Dimensions,
		EINV_GridSlotState State);

	/**
	 * Gray out blocking items to show they're in the way.
	 *
	 * @param GridSlots All grid slots
	 * @param GridWidth Width of the grid
	 * @param BlockingUpperLeftIndices Upper-left indices of blocking items
	 * @param GetItemFragment Function to get grid fragment for an item
	 * @return Indices that were grayed out (for later restoration)
	 */
	static TArray<int32> HighlightBlockingItems(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridWidth,
		const TArray<int32>& BlockingUpperLeftIndices,
		TFunction<const FINV_GridFragment*(int32)> GetItemFragment);

	/**
	 * Remove gray-out from previously highlighted blocking items.
	 *
	 * @param GridSlots All grid slots
	 * @param GridWidth Width of the grid
	 * @param GrayedOutIndices Indices that were previously grayed out
	 * @param GetItemFragment Function to get grid fragment for an item
	 */
	static void UnHighlightBlockingItems(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		int32 GridWidth,
		const TArray<int32>& GrayedOutIndices,
		TFunction<const FINV_GridFragment*(int32)> GetItemFragment);

	/**
	 * Refresh all grid slot visuals based on their availability state.
	 *
	 * @param GridSlots All grid slots
	 */
	static void RefreshAllSlotVisuals(const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots);

	/**
	 * Apply a texture state to a single slot based on its availability.
	 *
	 * @param GridSlot Slot to update
	 */
	static void ApplyStateBasedOnAvailability(UINV_GridSlot* GridSlot);
};
