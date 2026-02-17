// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/INV_GridTypes.h"

class UINV_GridSlot;
class UINV_InventoryItem;
struct FINV_ItemManifest;
struct FGameplayTag;

/**
 * Pure logic engine for spatial inventory grid placement calculations.
 *
 * This class contains no state and no widget dependencies, making it:
 * - Easy to unit test
 * - Reusable across different grid types
 * - Performance optimized through static methods
 *
 * Responsibilities:
 * - Spatial availability queries (can item fit?)
 * - Coordinate calculations (where should item snap?)
 * - Stack compatibility checks
 * - Grid bounds validation
 */
class INVENTORY_API FINV_GridPlacementEngine
{
public:
	/**
	 * Check if an item manifest can fit in the grid with stack consideration.
	 *
	 * @param GridSlots - Array of all grid slots
	 * @param GridSize - Dimensions of the grid (width, height)
	 * @param Manifest - Item manifest to check
	 * @param bUseItemRarity - Whether rarity should be considered for stacking
	 * @param ItemRarityTag - The rarity tag if used
	 * @return Result containing available slots, room to fill, and remainder
	 */
	static FINV_SlotAvailabilityResult HasRoomForItem(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		const FINV_ItemManifest& Manifest,
		bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag);

	/**
	 * Check if space at a position is free or contains a single swappable item.
	 *
	 * @param GridSlots - Array of all grid slots
	 * @param GridSize - Dimensions of the grid
	 * @param Position - Upper-left coordinate to check
	 * @param Dimensions - Size of the item footprint
	 * @return Result indicating if space is free, or which item(s) block it
	 */
	static FINV_SpaceQueryResult CheckHoverPosition(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		const FIntPoint& Position,
		const FIntPoint& Dimensions);

	/**
	 * Calculate the upper-left starting coordinate for centering an item on cursor.
	 *
	 * Handles even/odd dimension adjustments to provide intuitive item placement.
	 *
	 * @param CursorCoord - Current tile coordinate of the cursor
	 * @param ItemDimensions - Size of the item being placed
	 * @param CursorQuadrant - Which quadrant of the tile the cursor is in
	 * @return Upper-left coordinate where item should be placed
	 */
	static FIntPoint CalculateStartingCoordinate(
		const FIntPoint& CursorCoord,
		const FIntPoint& ItemDimensions,
		EINV_TileQuadrant CursorQuadrant);

	/**
	 * Validate if an item footprint fits within grid boundaries.
	 *
	 * @param StartIndex - Linear index of upper-left position
	 * @param ItemDimensions - Size of item footprint
	 * @param GridSize - Dimensions of the grid
	 * @return True if entire footprint is within bounds
	 */
	static bool IsInGridBounds(
		int32 StartIndex,
		const FIntPoint& ItemDimensions,
		const FIntPoint& GridSize);

	/**
	 * Check if two items can stack together based on type and rarity.
	 *
	 * @param ExistingItem - Item already in the grid
	 * @param ItemType - Type tag of the item being added
	 * @param bUseItemRarity - Whether rarity matters for stacking
	 * @param ItemRarityTag - Rarity tag to compare
	 * @return True if items are compatible for stacking
	 */
	static bool IsStackCompatible(
		const UINV_InventoryItem* ExistingItem,
		const FGameplayTag& ItemType,
		bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag);

	/**
	 * Get the dimensions of an item from its manifest.
	 * Defaults to 1x1 if no grid fragment exists.
	 */
	static FIntPoint GetItemDimensions(const FINV_ItemManifest& Manifest);

	/**
	 * Determine how many stacks can fit in a slot.
	 *
	 * @param bStackable - Whether the item is stackable
	 * @param MaxStackSize - Maximum stack size for this item
	 * @param AmountToFill - How many we're trying to add
	 * @param GridSlot - The slot we're checking
	 * @return Number of stacks that can fit
	 */
	static int32 DetermineFillAmountForSlot(
		bool bStackable,
		int32 MaxStackSize,
		int32 AmountToFill,
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const UINV_GridSlot* GridSlot);

	/**
	 * Get the current stack count at a grid slot (handles upper-left reference).
	 */
	static int32 GetStackAmount(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const UINV_GridSlot* GridSlot);

private:
	/**
	 * Check if a specific index can accommodate an item of given dimensions.
	 *
	 * @param GridSlots - All grid slots
	 * @param GridSize - Grid dimensions
	 * @param GridSlot - Starting slot to check
	 * @param Dimensions - Item footprint size
	 * @param CheckedIndices - Indices already claimed
	 * @param OutTentativelyClaimed - Output of newly claimed indices
	 * @param ItemType - Type for stack compatibility
	 * @param bUseItemRarity - Rarity check flag
	 * @param ItemRarityTag - Rarity for comparison
	 * @param MaxStackSize - Max stacks for this item
	 * @return True if item can fit at this position
	 */
	static bool HasRoomAtIndex(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const FIntPoint& GridSize,
		const UINV_GridSlot* GridSlot,
		const FIntPoint& Dimensions,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag,
		int32 MaxStackSize);

	/**
	 * Check if a sub-slot satisfies constraints for item placement.
	 *
	 * Validates:
	 * - Index not already claimed
	 * - Empty OR contains stackable compatible item with room
	 */
	static bool CheckSlotConstraints(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const UINV_GridSlot* GridSlot,
		const UINV_GridSlot* SubGridSlot,
		const TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		bool bUseItemRarity,
		const FGameplayTag& ItemRarityTag,
		int32 MaxStackSize);

	// Helper predicates
	static bool IsIndexClaimed(const TSet<int32>& CheckedIndices, int32 Index);
	static bool HasValidItem(const UINV_GridSlot* GridSlot);
	static bool IsUpperLeftSlot(const UINV_GridSlot* GridSlot, const UINV_GridSlot* SubGridSlot);
};
