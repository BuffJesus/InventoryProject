// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UINV_HoverItem;
class UINV_InventoryItem;
class UINV_GridSlot;
class APlayerController;
class UUserWidget;
struct FINV_GridFragment;
struct FINV_ImageFragment;

/**
 * Manages hover item lifecycle, creation, and cursor visibility.
 * Handles pickup, assignment, and cleanup of dragged inventory items.
 */
class INVENTORYUI_API FINV_HoverItemManager
{
public:
	/**
	 * Create or update hover item widget with inventory item data.
	 *
	 * @param HoverItem Hover item widget to update (created if null)
	 * @param HoverItemClass Class to instantiate if creating new hover item
	 * @param InventoryItem Item to display in hover
	 * @param GridFragment Grid size data for the item
	 * @param ImageFragment Icon data for the item
	 * @param TileSize Size of grid tiles for scaling
	 * @param OwningPlayer Player controller for widget creation
	 * @param ContextWidget Widget context for viewport scale
	 * @return Updated or created hover item widget
	 */
	static UINV_HoverItem* AssignHoverItem(
		UINV_HoverItem* HoverItem,
		TSubclassOf<UINV_HoverItem> HoverItemClass,
		UINV_InventoryItem* InventoryItem,
		const FINV_GridFragment* GridFragment,
		const FINV_ImageFragment* ImageFragment,
		float TileSize,
		APlayerController* OwningPlayer,
		UUserWidget* ContextWidget,
		const TOptional<FVector2D>& InitialViewportCenter = TOptional<FVector2D>());

	/**
	 * Set additional hover item properties (previous index, stack count).
	 *
	 * @param HoverItem Hover item to configure
	 * @param GridSlot Grid slot for stack count
	 * @param InventoryItem Item for stackable check
	 * @param PreviousGridIndex Index to remember for returning item
	 */
	static void ConfigureHoverItemProperties(
		UINV_HoverItem* HoverItem,
		const UINV_GridSlot* GridSlot,
		const UINV_InventoryItem* InventoryItem,
		int32 PreviousGridIndex);

	/**
	 * Clear and remove hover item from viewport.
	 *
	 * @param HoverItem Hover item to clear
	 */
	static void ClearHoverItem(TObjectPtr<UINV_HoverItem>& HoverItem);

	/**
	 * Get or create a cursor widget.
	 *
	 * @param CachedWidget Cached widget reference
	 * @param WidgetClass Class to instantiate if creating
	 * @param OwningPlayer Player controller for widget creation
	 * @return Cursor widget instance
	 */
	static UUserWidget* GetOrCreateCursorWidget(
		TObjectPtr<UUserWidget>& CachedWidget,
		const TSubclassOf<UUserWidget>& WidgetClass,
		APlayerController* OwningPlayer);

	/**
	 * Set the mouse cursor widget.
	 *
	 * @param PlayerController Player controller to update
	 * @param CursorWidget Widget to use as cursor
	 */
	static void SetCursorWidget(
		APlayerController* PlayerController,
		UUserWidget* CursorWidget);
};
