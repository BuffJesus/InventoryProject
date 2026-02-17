// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "INV_GridPopupActions.generated.h"

class UINV_GridSlot;
class UINV_SlottedItem;
class UINV_InventoryItem;
class UINV_HoverItem;
class APlayerController;

/**
 * Utility class for popup menu action handlers.
 * Handles split, drop, consume, and inspect actions from popup menus.
 */
UCLASS()
class INVENTORY_API UINV_GridPopupActions : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Execute split action - split stack and assign portion to hover item.
	 *
	 * @param GridSlots All grid slots in the inventory
	 * @param SlottedItems Map of slotted item widgets
	 * @param HoverItem Reference to hover item to populate
	 * @param Item Item being split
	 * @param Index Grid index where item is located
	 * @param SplitAmount Amount to split off
	 * @param GridWidth Width of the grid
	 * @param AssignHoverCallback Callback to assign hover item
	 */
	static void ExecuteSplit(
		const TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
		UINV_InventoryItem* Item,
		int32 Index,
		int32 SplitAmount,
		TFunction<void(UINV_InventoryItem*, int32, int32)> AssignHoverCallback,
		TFunction<void(UINV_HoverItem*, int32)> UpdateHoverStackCallback);

	/**
	 * Execute consume action - reduce stack count by 1.
	 *
	 * @param GridSlots All grid slots in the inventory
	 * @param SlottedItems Map of slotted item widgets
	 * @param Item Item being consumed
	 * @param Index Grid index where item is located
	 * @return New stack count after consuming
	 */
	static int32 ExecuteConsume(
		TArray<TObjectPtr<UINV_GridSlot>>& GridSlots,
		const TMap<int32, TObjectPtr<UINV_SlottedItem>>& SlottedItems,
		UINV_InventoryItem* Item,
		int32 Index);
};
